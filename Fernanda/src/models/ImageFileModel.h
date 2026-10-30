/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <utility>

#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QImage>
#include <QImageReader>
#include <QPixmap>

#include <Coco/Debug.h>

#include "core/PathKey.h"
#include "models/AbstractFileModel.h"

namespace Fernanda {

// A read-only image buffer. Like PdfFileModel, it inherits every read-only
// default from the base (isModified / isUserEditable false, undo inert,
// reloadContent -> setData) and implements only data() / setData().
//
// Holds three things:
//
//   data_   — the file's bytes, RETAINED. data() stays truthful, so the
//             store's external-change no-op check (disk == data()) still
//             works; an animated image needs a live QIODevice over them for
//             its QMovie; and it costs only the compressed size. No base
//             shared with PdfFileModel: what the two have in common is one
//             member and a one-line getter.
//
//   pixmap_ — the decoded first frame, the shared resource every view shows —
//             the image analogue of PDF's one parse. QPixmap is implicitly
//             shared, so N views on one image (or a common-vault image in two
//             windows) hold one set of pixels; converting a QImage per view
//             would copy them per view. A null pixmap means the decode failed,
//             and views show their load-failure placeholder off isNull().
//
//   isAnimated_ — whether the content has more than one frame. Read from the
//             content, not the extension, so an animated WebP animates too
//             and a static GIF doesn't pay for a QMovie. The model never
//             plays anything: each view runs its own
//             QMovie over data() and pauses it while hidden (ImageFileView).
//             pixmap_ is still the first frame, shown until the movie starts
//             and whenever it isn't running.
//
// Decoding is by CONTENT: QImageReader sniffs the format, so the extension only
// dispatched us here (FileTypes). Auto-transform is on so EXIF orientation is
// applied — Qt leaves it off by default, and a browser (so Obsidian) applies
// it; without it, portrait phone photos open sideways. A multi-page TIFF shows
// its first page. Qt's default decode allocation limit (256 MB) stands: past
// it, the read fails and the view shows the placeholder.
//
// QPixmap needs the GUI application, so this model is GUI-bound in the way
// PdfFileModel (QtPdf) already is; it's built on the GUI thread by the store
class ImageFileModel : public AbstractFileModel
{
    Q_OBJECT

public:
    ImageFileModel(const PathKey& key, QObject* parentStore)
        : AbstractFileModel(key, parentStore)
    {
    }

    ~ImageFileModel() override { TRACER; }

    // The decoded image every view shows (an animation's first frame). Null
    // when the decode failed
    [[nodiscard]] QPixmap pixmap() const noexcept { return pixmap_; }

    // More than one frame, so views should play it (see class note). False
    // whenever the decode failed
    [[nodiscard]] bool isAnimated() const noexcept { return isAnimated_; }

    // --- AbstractFileModel contract ----------------------------------------

    // The raw file bytes, exactly as setData last received them. Never written
    // (nothing marks an image modified), but truthful — see class note
    [[nodiscard]] QByteArray data() const override { return data_; }

    // Load — or reload — from fresh bytes. Called on open, and again on an
    // external change via the base's reloadContent -> setData; the store then
    // emits reloaded(), which is what views re-read the pixmap on (unlike PDF,
    // whose views ride QPdfDocument::statusChanged)
    void setData(const QByteArray& data) override
    {
        data_ = data;
        decode_();
    }

private:
    QByteArray data_{};
    QPixmap pixmap_{};
    bool isAnimated_ = false;

    // The reader's buffer is a local: nothing reads from it after read()
    // returns. (An animated image's QMovie needs a live device — that's the
    // view's own buffer over data_.) The buffer COW-shares data_, so this isn't
    // a copy.
    //
    // Animation is decided before read(): supportsAnimation() says the format
    // can animate (GIF, WebP — not PNG, since Qt doesn't read APNG), and
    // imageCount() says whether this file does. A count of 0 means the handler
    // can't tell; that's treated as static, so the first frame (with EXIF
    // orientation applied, which a QMovie wouldn't do) is what shows
    void decode_()
    {
        pixmap_ = {};
        isAnimated_ = false;

        QBuffer buffer{};
        buffer.setData(data_);
        buffer.open(QIODevice::ReadOnly);

        QImageReader reader(&buffer);
        reader.setAutoTransform(true);

        auto is_animated =
            reader.supportsAnimation() && reader.imageCount() > 1;
        auto image = reader.read();
        if (image.isNull()) {
            // Includes a missing imageformats plugin (TIFF/WebP), which reads
            // as "Unsupported image format"
            WARN(
                "Image decode failed for {}: {}",
                key().path(),
                reader.errorString());
            return;
        }

        pixmap_ = QPixmap::fromImage(std::move(image));
        isAnimated_ = is_animated;
    }
};

} // namespace Fernanda
