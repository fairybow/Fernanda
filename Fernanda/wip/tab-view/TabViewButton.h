#pragma once

#include <QEnterEvent>
#include <QEvent>
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QString>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVariant>

#include "Coco/Debug.h"

// Shows flag when flagged but not under mouse
class TabViewButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit TabViewButton(QWidget* parent = nullptr)
        : QAbstractButton(parent)
    {
        initialize_();
    }

    virtual ~TabViewButton() override { COCO_TRACER; }

    // Idk about this
    QString text() const {}
    void setText(const QString& text) {}

    bool flagged() const noexcept { return flagged_; }
    QString svgPath() const noexcept { return svgPath_; }
    QString flagSvgPath() const noexcept { return flagSvgPath_; }
    QSize svgSize() const noexcept { return svgSize_; }

    void setFlagged(bool flagged)
    {
        if (flagged_ == flagged) return;
        flagged_ = flagged;
        update();
    }

    void setSvgPath(const QString& svgPath)
    {
        if (svgPath_ == svgPath) return;
        svgPath_ = svgPath;
        invalidateCache_();
        update();
    }

    void setFlagSvgPath(const QString& flagSvgPath)
    {
        if (flagSvgPath_ == flagSvgPath) return;
        flagSvgPath_ = flagSvgPath;
        invalidateCache_();
        update();
    }

    void setSvgSize(const QSize& size)
    {
        if (svgSize_ == size) return;
        svgSize_ = size;
        invalidateCache_();
        update();
    }

protected:
    virtual void enterEvent(QEnterEvent* event) override
    {
        QAbstractButton::enterEvent(event);
        update();
    }

    virtual void leaveEvent(QEvent* event) override
    {
        QAbstractButton::leaveEvent(event);
        update();
    }

    /// Review and edit this!
    virtual void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Draw background based on button state
        auto rect = this->rect();
        auto pressed = isDown();
        auto under_mouse = underMouse();

        // Transparent background for normal state
        if (pressed || under_mouse)
        {
            QPainterPath path{};

            // Shrink and round out the highlight
            auto highlight_rect = rect.adjusted(INSET_, INSET_, -INSET_, -INSET_);
            path.addRoundedRect(highlight_rect, CORNER_RADIUS_, CORNER_RADIUS_);

            // Fill
            if (pressed) painter.fillPath(path, QColor(0, 0, 0, 30));
            else if (under_mouse) painter.fillPath(path, QColor(0, 0, 0, 15));
        }

        // Determine which SVG to render
        QString svg_path = shouldShowFlag_() ? flagSvgPath_ : svgPath_;
        if (svg_path.isEmpty()) return;

        // Get or create cached pixmap
        auto pixmap = getCachedPixmap_(svg_path);
        if (pixmap.isNull()) return;

        // Paint the SVG centered in the button
        auto pixmap_rect = pixmap.rect();

        // Center the pixmap in the content area
        auto x = rect.x() + (rect.width() - pixmap_rect.width()) / 2;
        auto y = rect.y() + (rect.height() - pixmap_rect.height()) / 2;

        painter.drawPixmap(x, y, pixmap);
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        QAbstractButton::resizeEvent(event);
        invalidateCache_();
    }

private:
    static constexpr auto FLAG_PROPERTY_ = "flagged";
    static constexpr auto CORNER_RADIUS_ = 4;
    static constexpr auto INSET_ = 2;

    QString svgPath_{};
    QString flagSvgPath_{};
    QSize svgSize_{ 16, 16 };
    bool flagged_ = false;
    mutable QHash<QString, QPixmap> pixmapCache_{};

    void initialize_()
    {
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
    }

    bool shouldShowFlag_() const
    {
        return flagged_ && !flagSvgPath_.isEmpty() && !underMouse();
    }

    void invalidateCache_()
    {
        pixmapCache_.clear();
    }

    QPixmap getCachedPixmap_(const QString& svgPath) const
    {
        if (svgPath.isEmpty()) return {};

        // Create cache key including effective size
        auto target_size = getEffectiveIconSize_();
        auto cache_key = QString("%1_%2x%3")
            .arg(svgPath)
            .arg(target_size.width())
            .arg(target_size.height());

        // Check cache first
        if (pixmapCache_.contains(cache_key))
            return pixmapCache_[cache_key];

        // Render SVG to pixmap at target size
        auto pixmap = renderSvgToPixmap_(svgPath, target_size);

        // Cache the result
        if (!pixmap.isNull())
            pixmapCache_[cache_key] = pixmap;

        return pixmap;
    }

    QPixmap renderSvgToPixmap_(const QString& svgPath, const QSize& targetSize) const
    {
        QSvgRenderer renderer(svgPath);

        if (!renderer.isValid())
        {
            qWarning() << "Failed to load SVG:" << svgPath;
            return {};
        }

        // Account for high DPI displays
        auto device_pixel_ratio = devicePixelRatio();
        auto pixmap_size = targetSize * device_pixel_ratio;

        QPixmap pixmap(pixmap_size);
        pixmap.setDevicePixelRatio(device_pixel_ratio);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Render SVG directly at the target size - no post-scaling!
        renderer.render(&painter);

        return pixmap;
    }

    QSize getEffectiveIconSize_() const
    {
        auto size = contentsRect().size();

        return
        {
            qMin(svgSize_.width(), size.width()),
            qMin(svgSize_.height(), size.height())
        };
    }
};
