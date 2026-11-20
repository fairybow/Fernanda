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

#include "core/files/PathKey.h"
#include "models/AbstractFileModel.h"

namespace Fernanda {

// A read-only image buffer. Like PdfFileModel, it keeps every read-only
// default from the base and implements only data() and setData(). It holds:
//
//   data_       the file's bytes, kept so data() stays truthful for the
//               store's external-change check (disk == data()) and an
//               animated image's QMovie can read them, at only the compressed
//               size. No base shared with PdfFileModel: they share one member
//               and a getter
//   pixmap_     the decoded first frame, shared by every view: QPixmap is
//               implicitly shared, so N views hold one set of pixels. Null
//               when the decode failed, which views show as their placeholder
//   isAnimated_ whether the content has more than one frame, so an animated
//               WebP animates and a static GIF doesn't pay for a QMovie. Each
//               view plays its own movie (ImageFileView); the model never does
//
// Decoding is by content (QImageReader sniffs the format). Auto-transform is
// on, so EXIF orientation is applied, as browsers and so Obsidian do;
// otherwise portrait phone photos open sideways. A multi-page TIFF shows its
// first page. Past Qt's 256 MB decode limit the read fails, showing the
// placeholder.
//
// QPixmap needs the GUI application, so this model is GUI-bound, as
// PdfFileModel is through QtPdf; the store builds it on the GUI thread
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
