/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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
#include <QObject>
#include <QString>

#include <Coco/Debug.h>

#include "core/FileRef.h"

namespace Suzuri {

// One buffer per open file, plus that file's identity. GUI-free, and
// filesystem-free: the Vault owns all disk IO. data() and setData() are the
// boundary the Vault moves bytes across — on open it reads from disk and calls
// setData(); on flush it reads data() and writes.
//
// Polymorphic by file type: text, PDF, image. Views, tabs, and the Vault's map
// all handle the base type; only the Vault's model factory names a concrete
// one.
//
// Owned by the Vault (QObject parent). No back-pointer to any window — the
// FileRef carries the vault it belongs to, and that is enough.
class AbstractFileModel : public QObject
{
    Q_OBJECT

public:
    AbstractFileModel(const FileRef& fileRef, QObject* parentVault)
        : QObject(parentVault)
        , fileRef_(fileRef)
    {
    }

    ~AbstractFileModel() override = default;

    [[nodiscard]] FileRef fileRef() const noexcept { return fileRef_; }

    // The relative path is the buffer's identity within its vault; its stem is
    // the display title. No title-override machinery — files are always on
    // disk, so the filename is always the name
    [[nodiscard]] QString title() const
    {
        return fileRef_.relative.stemQString();
    }

    // The contract. Everything else is optional and defaulted for the
    // non-editable types (image, PDF), whose save() would be a lie
    [[nodiscard]] virtual QByteArray data() const = 0;
    virtual void setData(const QByteArray& data) = 0;

    // Replace the buffer with fresh bytes from disk after an external change.
    // The default replaces wholesale, which is right for the non-editable types
    // (image/PDF have no undo history to keep). TextFileModel overrides it to
    // make the replace UNDOABLE, so Ctrl+Z restores what the user had and the
    // next autosave rewrites it — the escape hatch that lets external changes
    // be adopted without prompting. Kept distinct from setData, which is the
    // initial-open path where clearing undo is correct
    virtual void reloadContent(const QByteArray& data) { setData(data); }

    // Re-point this buffer after its file — or an ancestor folder — was renamed
    // in place. The Vault owns identity and calls this after the on-disk rename
    // and map re-key; the vault is unchanged, only the relative path moves.
    // Emits renamed() so views retitle: a direct file rename changes title()
    // (the stem); an ancestor-folder rename leaves the stem the same, and the
    // view's windowTitle simply doesn't re-emit when the text is unchanged, so
    // a folder rename re-keys with no tab churn
    void renameTo(const FileRef& newRef)
    {
        fileRef_ = newRef;
        emit renamed();
    }

    [[nodiscard]] virtual bool isUserEditable() const { return false; }
    [[nodiscard]] virtual bool isModified() const { return false; }
    virtual void setModified(bool /*modified*/) {}

    // --- Undo / redo -------------------------------------------------------

    // Undo state is model-owned, not view-owned: the single stack lives on the
    // buffer (for text, TextFileModel's PrimeDocument) and is shared across
    // every view of the file, so undo in one view reverses the same edit
    // everywhere rather than drifting per view. The window's Undo/Redo actions
    // dispatch here on whichever model backs the active view
    // (BaseWindow::activeFileModel). That is why these live on the base and not
    // on any view: two views of one file share one undo, not one each — a view
    // forwarder would site a shared concept on the wrong object.
    //
    // The defaults are inert, so the non-editable types (image/PDF) inherit a
    // no-op undo and permanently-unavailable state with zero code. An editable
    // type overrides the four and emits the two availability signals below —
    // text does. undo()/redo() no-op safely when the stack is empty, so callers
    // need not pre-check availability
    [[nodiscard]] virtual bool isUndoAvailable() const { return false; }
    [[nodiscard]] virtual bool isRedoAvailable() const { return false; }
    virtual void undo() {}
    virtual void redo() {}

    // --- Views / eviction ----------------------------------------

    // Count the views borrowing this model, so the Vault can evict the buffer
    // once the last one closes. This lives on the base — not on TextFileModel's
    // PrimeDocument — because every file type has views but only text has a
    // prime: image/PDF models reuse this untouched. Kept GUI-free by taking a
    // QObject* the model only connects to and never dereferences, so no view
    // header leaks into the model layer.
    //
    // The model observes each view's destroyed rather than views calling back
    // on teardown, so a view dying first can never touch a dead model. Called
    // exactly once per view, from the AbstractFileView base constructor, so the
    // count needs no membership set — one call in, one destroyed out
    void addView(QObject* view)
    {
        ++viewCount_;

        DEBUG(
            this,
            "View added to model [{}] — total views: {}",
            fileRef_.relative,
            viewCount_);

        connect(view, &QObject::destroyed, this, [this] {
            DEBUG(
                this,
                "View removed from model [{}] — total views: {}",
                fileRef_.relative,
                viewCount_ - 1);

            if (--viewCount_ == 0) {
                emit lastViewClosed();
            }
        });
    }

    [[nodiscard]] int viewCount() const noexcept { return viewCount_; }

    // --- External-change notifications ----------------------

    // The Vault owns IO, so it decides what happened on disk and makes these
    // calls; keeping the emits here leaves each signal owned by the model that
    // declares it. Views borrowing this model react, and because a shared
    // common-vault file has ONE model, every window's views react together —
    // pop-outs included.

    // The file vanished from disk while open — deleted or moved. Views
    // self-close: the buffer they showed no longer has a home on disk (no
    // off-disk buffers). The Vault frees the model right after
    void notifyRemovedFromDisk() { emit removedFromDisk(); }

    // The buffer was replaced from disk after an external change. Text views
    // need nothing here — the prime already routed the new content into their
    // documents; ImageFileView re-reads the model on this
    void notifyReloaded() { emit reloaded(); }

signals:
    // TODO: Will we use this?
    void modificationChanged(bool modified);

    // Every edit, not just the clean->dirty transition that modificationChanged
    // reports. The Vault restarts its autosave debounce on this, so a write
    // lands after typing pauses rather than mid-burst. Editable models only —
    // image/PDF never mutate, so they never emit it
    void contentChanged();

    // The file vanished from disk while open — deleted or moved externally
    void removedFromDisk();

    // The buffer was replaced from disk after an external change
    void reloaded();

    // The file was renamed in place, or an ancestor folder was — fileRef() and
    // title() now reflect the new path. Views retitle their tab; a shared
    // common-vault file has one model, so every window's views react together
    void renamed();

    // The last view borrowing this model closed. The Vault flushes any unsaved
    // bytes and frees the buffer on this, so models_ doesn't grow without bound
    // as tabs open and close. Emitted from addView's per-view destroyed handler
    // when the count returns to zero
    void lastViewClosed();

    // Undo/redo availability changed on this model's stack. Declared on the
    // base so every undoable model emits them uniformly and a later live
    // surface (toolbar, command palette) can drive enabled-state off them.
    // Nothing consumes them yet: Undo and Redo stay enabled and no-op when
    // unavailable. Non-editable models never emit them
    void undoAvailable(bool available);
    void redoAvailable(bool available);

private:
    FileRef fileRef_;

    // Views borrowing this model. Not the view objects themselves — just how
    // many — since the only consumer is the last-view-close eviction edge
    int viewCount_ = 0;
};

} // namespace Suzuri
