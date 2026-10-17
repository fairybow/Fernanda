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

#include <QAbstractButton>
#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/widgets/Glyph.h"
#include "views/ViewConstants.h"

namespace Suzuri {

using namespace Qt::StringLiterals;

namespace Internal {

// A flat glyph button for the zoom pill. Deliberately NOT a GlyphButton: that
// draws the style's chrome auto-raise frame and tints from palette(WindowText),
// both right on the tab row and both wrong floating over PDF content. This one
// is flat with a faint translucent-white hover and a fixed light glyph, legible
// on the dark pill regardless of app theme. It shares only the SVG render and
// its cache (Glyph.h) with the tab buttons. Behavior-free like GlyphButton — it
// emits clicked() and the ZoomControl decides what that means
class ZoomButton_ : public QAbstractButton
{
    Q_OBJECT

public:
    // Always one of ZoomControl's own buttons, hence the parent's name
    ZoomButton_(const Coco::Path& glyphPath, QWidget* parentControl)
        : QAbstractButton(parentControl)
        , glyphPath_(glyphPath)
    {
        setAttribute(Qt::WA_Hover, true);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setFixedSize(ZOOM_CONTROL_BUTTON_EXTENT, ZOOM_CONTROL_BUTTON_EXTENT);
    }

protected:
    void enterEvent(QEnterEvent* event) override
    {
        QAbstractButton::enterEvent(event);
        update();
    }

    void leaveEvent(QEvent* event) override
    {
        QAbstractButton::leaveEvent(event);
        update();
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Faint wash on hover/press — the pill's own feel, not the style's
        // chrome frame (see class note)
        if (underMouse() || isDown()) {
            painter.setPen(Qt::NoPen);
            // TODO: Do these belong in ViewConstants?:
            painter.setBrush(QColor(255, 255, 255, isDown() ? 45 : 25));
            painter.drawRoundedRect(rect(), 4.0, 4.0);
        }

        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto extent = qMin(ZOOM_CONTROL_ICON_EXTENT, qMin(width(), height()));
        if (extent <= 0) {
            return;
        }

        // Fixed light glyph — over document content, so it can't follow the app
        // palette and stay legible
        auto pixmap = glyph_.pixmap(
            glyphPath_,
            extent,
            ZOOM_CONTROL_ICON_COLOR,
            devicePixelRatioF());

        painter.drawPixmap(
            (width() - extent) / 2,
            (height() - extent) / 2,
            pixmap);
    }

private:
    Coco::Path glyphPath_;
    Ui::Glyph::Cache glyph_{};
};

// The pill's clickable readout ("Fit" / "N%"). Painted, not a QLabel: a QLabel
// draws its text in its palette's WindowText, and even a palette set on it
// explicitly comes out black over the image view — something in the
// QGraphicsView's ancestry re-resolves it. So the readout paints a fixed color,
// as the glyph buttons do, and stays legible on the dark pill regardless of
// palette or style. Behavior-free like the buttons: it emits clicked() and the
// ZoomControl decides what that means
class ZoomReadout_ : public QWidget
{
    Q_OBJECT

public:
    // Always the ZoomControl's own readout, hence the parent's name
    explicit ZoomReadout_(QWidget* parentControl)
        : QWidget(parentControl)
    {
        setCursor(Qt::PointingHandCursor); // it's clickable
        setFixedSize(ZOOM_CONTROL_READOUT_WIDTH, ZOOM_CONTROL_BUTTON_EXTENT);
    }

    void setText(const QString& text)
    {
        if (text_ == text) {
            return;
        }

        text_ = text;
        update();
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
            event->accept();
            return;
        }

        QWidget::mousePressEvent(event);
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setPen(ZOOM_CONTROL_READOUT_COLOR);
        painter.drawText(rect(), Qt::AlignCenter, text_);
    }

private:
    QString text_{};
};

} // namespace Internal

// The floating zoom pill: [-] [readout] [+], bottom-right over a scaled
// document view (PDF and image). Behavior-free in the same sense as the
// buttons: it reports what the user asked for and shows whatever text the
// owning view pushes back. The view owns the zoom state and is the single
// source of truth; this only emits intents and displays a string.
//
// The readout doubles as the Fit toggle (a visible, discoverable affordance) —
// clicking it flips Fit<->Fixed. Reset stays on Ctrl+0 / the View menu, not a
// hidden right-click. Dark fixed pill + light glyphs + a hairline border, so it
// reads on a white OR a black page and can't inherit an invisible light-theme
// palette
class ZoomControl : public QWidget
{
    Q_OBJECT

public:
    // Always the scaled view it floats over (a QPdfView or an image view's
    // QGraphicsView) — hence the parent's name. It overlays that view and hides
    // with it in a load-failure state
    explicit ZoomControl(QWidget* parentView)
        : QWidget(parentView)
    {
        setup_();
    }

    ~ZoomControl() override { TRACER; }

    // The owning view pushes the current zoom string here after every change,
    // from whatever entry point drove it (buttons, wheel, keyboard, menu), so
    // the readout is always truthful
    void setDisplayText(const QString& text) { readout_->setText(text); }

signals:
    void zoomInRequested();
    void zoomOutRequested();
    void toggleFitRequested(); // readout click

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        // Track the parent view's size so the pill stays pinned bottom-right
        if (watched == parent() && event->type() == QEvent::Resize) {
            reposition_();
        }

        return QWidget::eventFilter(watched, event);
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        auto r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        // Dark translucent fill + a hairline light border. The border defines
        // the pill edge on any backdrop and keeps it visible over an all-black
        // PDF
        painter.setPen(QColor(255, 255, 255, 40));
        painter.setBrush(QColor(30, 30, 30, 205));
        painter.drawRoundedRect(r, ZOOM_CONTROL_RADIUS, ZOOM_CONTROL_RADIUS);
    }

private:
    Internal::ZoomButton_* minusButton_ =
        new Internal::ZoomButton_(u":/lucide/Minus.svg"_s, this);
    Internal::ZoomReadout_* readout_ = new Internal::ZoomReadout_(this);
    Internal::ZoomButton_* plusButton_ =
        new Internal::ZoomButton_(u":/lucide/Plus.svg"_s, this);

    void setup_()
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // The owning view pushes the real text on its first applyZoom_; this
        // is only what shows until then
        readout_->setText(tr("Fit"));

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(0);
        layout->addWidget(minusButton_);
        layout->addWidget(readout_);
        layout->addWidget(plusButton_);

        connect(minusButton_, &QAbstractButton::clicked, this, [this] {
            emit zoomOutRequested();
        });
        connect(plusButton_, &QAbstractButton::clicked, this, [this] {
            emit zoomInRequested();
        });
        connect(readout_, &Internal::ZoomReadout_::clicked, this, [this] {
            emit toggleFitRequested(); // Fit<->Fixed
        });

        // Follow the parent view's geometry (an explicit parent is always
        // passed — the scaled view)
        if (auto* p = parent()) {
            p->installEventFilter(this);
        }

        reposition_();
    }

    void reposition_()
    {
        auto* p = parentWidget();
        if (!p) {
            return;
        }

        adjustSize();
        move(
            p->width() - width() - ZOOM_CONTROL_MARGIN,
            p->height() - height() - ZOOM_CONTROL_MARGIN);
        raise(); // stay above the scroll area's viewport
    }
};

} // namespace Suzuri
