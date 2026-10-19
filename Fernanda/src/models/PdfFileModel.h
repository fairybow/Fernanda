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

#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QPdfDocument>

#include <Coco/Debug.h>

#include "core/PathKey.h"
#include "models/AbstractFileModel.h"

namespace Fernanda {

// A read-only PDF buffer. No prime, no undo, no per-view documents: a PDF is
// never edited in Fernanda, so the base's inert defaults (isModified /
// isUserEditable false, undo no-op, reloadContent -> setData) are exactly
// right, and only data() / setData() need implementing.
//
// The model owns the parsed QPdfDocument — the shared, one-per-file resource,
// the PDF analogue of TextFileModel's prime. Every view builds its own QPdfView
// pointing at THIS document, so N views share one parse (and, for a
// common-vault PDF, one across every window). QPdfDocument reads lazily from
// the QBuffer as pages are rendered, so the bytes and the buffer must outlive
// the document: all three are members. data_ and the buffer COW-share one
// QByteArray, so holding both is not a second copy.
//
// No base shared with ImageFileModel beyond AbstractFileModel, which already
// centralizes every read-only default: what the two have in common is one byte
// array and a one-line getter.
class PdfFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    PdfFileModel(const PathKey& key, QObject* parentVault)
        : AbstractFileModel(key, parentVault)
    {
        setup_();
    }

    ~PdfFileModel() override
    {
        TRACER;

        // The document reads from the buffer, so it goes first. Not strictly
        // needed (Qt holds the device as a QPointer, which just nulls), but
        // explicit
        delete document_;
    }

    // The shared parsed document the views' QPdfViews point at. Views also read
    // its status/statusChanged to swap in their error placeholder
    [[nodiscard]] QPdfDocument* document() const noexcept { return document_; }

    // --- AbstractFileModel contract ----------------------------------------

    // The raw file bytes, exactly as setData last received them. The Vault
    // never writes a PDF (nothing marks one modified), so this is read back
    // only by the Vault's own machinery, but it stays truthful
    [[nodiscard]] QByteArray data() const override { return data_; }

    // Load — or reload — the document from fresh bytes. Re-seat the buffer over
    // the new data and reopen it BEFORE load(), since QPdfDocument keeps
    // reading from the buffer as pages render and must point at the current
    // bytes for the model's life. Called on open, and again on an external
    // change via the base's reloadContent -> setData, which reloads this SAME
    // document in place — so every view repaints with no extra wiring
    void setData(const QByteArray& data) override
    {
        data_ = data;

        buffer_.close();
        buffer_.setData(data_);
        buffer_.open(QIODevice::ReadOnly);

        document_->load(&buffer_);
    }

private:
    QByteArray data_{};
    QBuffer buffer_{};
    QPdfDocument* document_ = new QPdfDocument(this);

    void setup_()
    {
        // The Vault owns IO and never surfaces a read failure here — data() is
        // whatever it handed us — so a load failure is a corrupt, locked, or
        // password-protected PDF. Log it; the view swaps to its error
        // placeholder off this same statusChanged (PdfFileView). There is no
        // password prompt
        connect(
            document_,
            &QPdfDocument::statusChanged,
            this,
            [this](QPdfDocument::Status status) {
                if (status == QPdfDocument::Status::Error) {
                    WARN("PDF load failed for {}!", key().path());
                }
            });
    }
};

} // namespace Fernanda
