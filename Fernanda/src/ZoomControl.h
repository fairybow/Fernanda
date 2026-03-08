/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

// Floating zoom control overlay. Anchors itself to the bottom-right
// corner of its parent, repositioning on resize via event filter. The overlay
// owns no zoom state, but rather emits requests and displays whatever
// percentage the parent view provides via setZoomPercent().
//
// TODO: Hide/fade out after a set linger time. Reappear on hover in its general
// area (bottom-right). Begin visible, though, so it's obvious the widget is
// there to users
// TODO: Buttons are a little too far from left and right edges respectively
// TODO: Widget itself is too far right (overlaps with scroll bar slightly and
// also over edge of fitted PDFs
// TODO: Button text (and perhaps % label) are slightly lower than centered
// TODO: Display button too wide
// TODO: Should pressing display reset to 100% or fit to view? Maybe cycle the
// two? On fit to view, we'd probably just want to hide the percent and replace
// with something else that indicates we're in fit mode. Additionally, when
// pressing zoom in or out in fit mode, how should the views handle this?
class ZoomControl : public QWidget
{
    Q_OBJECT

public:
    explicit ZoomControl(QWidget* parent)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~ZoomControl() override { TRACER; }

    void setZoomPercent(int percent)
    {
        display_->setText(QString("%1%").arg(percent));
    }

signals:
    void zoomOutRequested();
    void zoomResetRequested();
    void zoomInRequested();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == parent() && event->type() == QEvent::Resize)
            reposition_();

        return false;
    }

    virtual void paintEvent(QPaintEvent* event) override
    {
        (void)event;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(30, 30, 30, 200));
        painter.drawRoundedRect(rect(), 6.0, 6.0);
    }

private:
    static constexpr auto MARGIN_ = 12;
    static constexpr auto HEIGHT_ = 28;
    static constexpr auto DISPLAY_WIDTH_ = 46;

    QPushButton* minusButton_ = new QPushButton("-", this);
    QPushButton* display_ = new QPushButton("100%", this);
    QPushButton* plusButton_ = new QPushButton("+", this);

    void setup_()
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setAttribute(Qt::WA_StyledBackground, true);
        setupButtons_();

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(4, 2, 4, 2);
        layout->setSpacing(0);
        layout->addWidget(minusButton_);
        layout->addWidget(display_);
        layout->addWidget(plusButton_);

        connect(minusButton_, &QPushButton::clicked, this, [&] {
            emit zoomOutRequested();
        });

        connect(display_, &QPushButton::clicked, this, [&] {
            emit zoomResetRequested();
        });

        connect(plusButton_, &QPushButton::clicked, this, [&] {
            emit zoomInRequested();
        });

        if (auto p = parent()) p->installEventFilter(this);
        reposition_();
    }

    void setupButtons_()
    {
        // TODO: Refine button hover/idle contrast

        auto text_color = QColor(255, 255, 255, 210);
        auto button_palette = QPalette{};
        button_palette.setColor(QPalette::Button, QColor(255, 255, 255, 30));
        button_palette.setColor(QPalette::ButtonText, text_color);

        auto zoom_font = QFont{};
        zoom_font.setPixelSize(16);

        for (auto button : { minusButton_, plusButton_ }) {
            button->setFixedSize(HEIGHT_, HEIGHT_);
            button->setFocusPolicy(Qt::NoFocus);
            button->setFlat(true);
            button->setPalette(button_palette);
            button->setFont(zoom_font);
        }

        auto display_font = QFont{};
        display_font.setPixelSize(12);

        display_->setFixedSize(DISPLAY_WIDTH_, HEIGHT_);
        display_->setFlat(true);
        display_->setPalette(button_palette);
        display_->setFont(display_font);
    }

    void reposition_()
    {
        auto parent_widget = parentWidget();
        if (!parent_widget) return;

        adjustSize();

        auto x = parent_widget->width() - width() - MARGIN_;
        auto y = parent_widget->height() - height() - MARGIN_;
        move(x, y);
    }
};

} // namespace Fernanda
