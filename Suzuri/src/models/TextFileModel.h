/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QObject>
#include <QString>
#include <QStringDecoder>
#include <QTextDocument>

#include <Coco/Debug.h>

#include "core/FileRef.h"
#include "models/AbstractFileModel.h"
#include "models/PrimeDocument.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// A text buffer, backed by a PrimeDocument. The prime holds the canonical
// content and the single shared undo stack; each view registers its own
// QTextDocument (for independent layout) and the prime keeps them in sync.
//
// One per open file. Because the common Vault is shared by every window, a
// common file's single model gives cross-window live sync for free — no
// propagation protocol.
//
// This is a thin adapter: it *is* an AbstractFileModel (so views, tabs, and the
// Vault map handle it polymorphically) and *has* a PrimeDocument (which is not
// itself a model). All the document work lives in the prime; this forwards.
class TextFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    TextFileModel(const FileRef& fileRef, QObject* parentVault)
        : AbstractFileModel(fileRef, parentVault)
    {
        setup_();
    }

    ~TextFileModel() override { TRACER; }

    // --- Views (delegate to the prime) -------------------------------------

    // The view keeps ownership of viewDoc; register it so its edits route
    // through the prime and out to the other views. See TextFileView
    void registerView(QTextDocument* viewDoc) { prime_->registerView(viewDoc); }

    void unregisterView(QTextDocument* viewDoc)
    {
        prime_->unregisterView(viewDoc);
    }

    [[nodiscard]] int viewCount() const { return prime_->viewCount(); }

    // --- Undo / redo (one stack, on the prime) -------------------------------

    [[nodiscard]] bool isUndoAvailable() const override
    {
        return prime_->isUndoAvailable();
    }

    [[nodiscard]] bool isRedoAvailable() const override
    {
        return prime_->isRedoAvailable();
    }

    void undo() override { prime_->undo(); }
    void redo() override { prime_->redo(); }

    // --- AbstractFileModel contract ----------------------------------------

    // Do these bytes decode cleanly as UTF-8? Vault::openModel asks before
    // building a text buffer, because QString::fromUtf8 (in
    // adoptFormatAndDecode_) silently turns every invalid sequence into U+FFFD,
    // and a buffer built that way isn't faithful to disk. It sits here because
    // this class is the one place bytes become text.
    //
    // Asks the decoder, not the decoded text: a file may legitimately contain
    // U+FFFD — every lossy open Suzuri accepts saves some — so searching for it
    // would flag such a file forever. Stateless, because a stateful decoder
    // holds a truncated final sequence as pending state instead of counting it
    // invalid. A BOM is valid UTF-8, so it needs no special case
    [[nodiscard]] static bool isValidUtf8(QByteArrayView bytes)
    {
        QStringDecoder decoder(
            QStringDecoder::Utf8,
            QStringDecoder::Flag::Stateless);

        // The decode runs on conversion to QString; the text itself is unused
        [[maybe_unused]] QString decoded = decoder(bytes);
        return !decoder.hasError();
    }

    [[nodiscard]] bool isUserEditable() const override { return true; }

    // UTF-8, TXT-first. Encoding detection is a later concern. The prime speaks
    // '\n' and never holds a BOM; restore the file's own line ending and BOM on
    // the way out, so an edit doesn't change how the file is stored (a
    // deliberate divergence from Obsidian, which normalizes to LF)
    [[nodiscard]] QByteArray data() const override
    {
        auto text = prime_->text();

        if (lineEnding_ == LineEnding_::CrLf) {
            text.replace(QChar('\n'), u"\r\n"_s);
        }

        auto bytes = text.toUtf8();

        if (hasUtf8Bom_) {
            bytes.prepend(UTF8_BOM_);
        }

        return bytes;
    }

    void setData(const QByteArray& data) override
    {
        prime_->setText(adoptFormatAndDecode_(data));

        // A freshly loaded buffer matches disk. setText() marks the document
        // modified, so clear it — this also emits modificationChanged(false)
        prime_->setModified(false);
    }

    // Reload from disk as an UNDOABLE replace. Every external change the Vault
    // adopts lands here; setData (the initial-open path) would clear undo via
    // setPlainText, so this uses the prime's undoable whole-document replace
    // instead, then marks clean — the buffer now matches disk, and Ctrl+Z
    // reverts to the prior content (which autosave then rewrites).
    //
    // Re-detects the line ending and BOM: an external tool that converted the
    // file has its choice adopted like any other external change. Undo restores
    // the prior TEXT only — neither is on the undo stack, so reverting a reload
    // saves the restored text in the external tool's format (accepted)
    void reloadContent(const QByteArray& data) override
    {
        prime_->replaceAllUndoable(adoptFormatAndDecode_(data));

        // The buffer matches disk again. Clearing the flag also keeps what the
        // user types next out of the reload's undo step: a text document
        // folds an insertion into the one before it when the two touch and
        // the document is modified, so text typed at the end of the reloaded
        // text would otherwise undo together with the reload
        prime_->setModified(false);
    }

    [[nodiscard]] bool isModified() const override
    {
        return prime_->isModified();
    }

    void setModified(bool modified) override { prime_->setModified(modified); }

signals:
    // Forwarded from the prime. After an undo/redo a view didn't originate, the
    // focused view repositions its cursor to the change site
    void cursorPositionHint(int position);

private:
    // The line ending the file uses on disk, restored by data() on write. The
    // prime always holds '\n' (QTextDocument folds every break into a block).
    // Bare '\r' isn't preserved: it loads as a break and saves as LF
    enum class LineEnding_
    {
        Lf,
        CrLf
    };

    LineEnding_ lineEnding_ = LineEnding_::Lf;

    // The UTF-8 byte-order mark. A file that has one keeps it: setData strips
    // it before decoding and data() re-attaches it on write, so the prime holds
    // content only. It has to be stripped explicitly — QString::fromUtf8 uses
    // the STATELESS QUtf8::convertToUnicode, which has no BOM handling (that
    // lives in the QStringDecoder overload), and QTextCursor::insertText
    // filters only line and frame separators, so a U+FEFF would survive into
    // the buffer and be written twice
    static constexpr QByteArrayView UTF8_BOM_ = "\xEF\xBB\xBF";
    bool hasUtf8Bom_ = false;

    PrimeDocument* prime_ = new PrimeDocument(this);

    // The FIRST line break decides. A mixed-ending file is therefore saved
    // uniformly in its first break's style — a normalization confined to files
    // already inconsistent. No break at all (every new file from createFile,
    // which starts empty) means LF, matching Obsidian's default
    static LineEnding_ detectLineEnding_(const QByteArray& data)
    {
        auto first_lf = data.indexOf('\n');

        if (first_lf > 0 && data.at(first_lf - 1) == '\r') {
            return LineEnding_::CrLf;
        }

        return LineEnding_::Lf;
    }

    void setup_()
    {
        // Bubble the prime's state up as this model's own signals
        connect(
            prime_,
            &PrimeDocument::modificationChanged,
            this,
            &AbstractFileModel::modificationChanged);

        // Every edit bubbles up as contentChanged (the base signal drops the
        // delta args we don't need here). The Vault hangs its autosave debounce
        // on this
        connect(
            prime_,
            &PrimeDocument::contentsChange,
            this,
            &AbstractFileModel::contentChanged);

        connect(
            prime_,
            &PrimeDocument::cursorPositionHint,
            this,
            &TextFileModel::cursorPositionHint);

        connect(
            prime_,
            &PrimeDocument::undoAvailable,
            this,
            &AbstractFileModel::undoAvailable);

        connect(
            prime_,
            &PrimeDocument::redoAvailable,
            this,
            &AbstractFileModel::redoAvailable);
    }

    // Adopt the file's on-disk format (line ending, BOM) so data() can restore
    // it on write, and return the decoded text WITHOUT the BOM. Detection runs
    // on the original bytes; only the decode sees the sliced view. The single
    // place bytes become text, so the open path and the external-reload path
    // can't disagree about what the prime holds
    [[nodiscard]] QString adoptFormatAndDecode_(const QByteArray& data)
    {
        lineEnding_ = detectLineEnding_(data);
        hasUtf8Bom_ = data.startsWith(UTF8_BOM_);

        return QString::fromUtf8(
            hasUtf8Bom_ ? data.sliced(UTF8_BOM_.size()) : data);
    }
};

} // namespace Fernanda
