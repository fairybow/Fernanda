/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

// Floating zoom control overlay. Anchors itself to the bottom-right corner of
// its parent, repositioning on resize via event filter. Owns zoom state (mode
// and factor) and emits a single zoomChanged() signal for relevant views to
// obey.
//
// Left-click display to toggle between Fit and last used fixed zoom;
// right-click display to set to 100% (also resets last used zoom factor)
//
// TODO: (Maybe) hide/fade out after a set linger time. Reappear on hover in its
// general area (bottom-right). Begin visible, though, so it's obvious the
// widget is there to users
// TODO: Allow typing percentage into display
// TODO: Buttons are a little too far from left and right edges respectively
// TODO: Widget itself is too far right (overlaps with scroll bar slightly and
// also over edge of fitted PDFs
// TODO: Button text (and perhaps % label) are slightly lower than centered
// TODO: Need to realign views' scrolls / content position on zoom. We want the
// relatively same area to be at the same spot on the screen so as not to
// disorient. We may also want to return to this position or another position
// when user toggles between factor and fit, which may mean changing ZoomControl
// a little
// TODO: Views also need to implement ability to pan
// TODO: Corners look too sharp
// TODO: Double click to change mode
class ZoomControl : public QWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        Fit,
        Fixed
    };

    ZoomControl(Mode mode, qreal factor, QWidget* parent = nullptr)
        : QWidget(parent)
        , mode_(mode)
        , factor_(factor)
    {
        setup_();
    }

    explicit ZoomControl(QWidget* parent = nullptr)
        : ZoomControl(Fit, 1.0, parent)
    {
    }

    virtual ~ZoomControl() override { TRACER; }

    Mode mode() const noexcept { return mode_; }
    qreal factor() const noexcept { return factor_; }

signals:
    void zoomChanged(Mode mode, qreal factor);

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        // Resize with parent
        if (watched == parent() && event->type() == QEvent::Resize)
            reposition_();

        // Reset on right-click
        if (watched == display_ && event->type() == QEvent::MouseButtonPress) {
            auto mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event->button() == Qt::RightButton) {
                goTo100_();
                return true;
            }
        }

        return false;
    }

    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(30, 30, 30, 200));
        painter.drawRoundedRect(rect(), 6.0, 6.0);
    }

private:
    Mode mode_;
    qreal factor_;

    static constexpr auto STEP_ = 0.1;
    static constexpr auto MIN_FACTOR_ = 0.1;
    static constexpr auto MAX_FACTOR_ = 3.0;

    QPushButton* minusButton_ = new QPushButton("-", this);
    QPushButton* display_ = new QPushButton("100%", this);
    QPushButton* plusButton_ = new QPushButton("+", this);

    static constexpr auto RIGHT_PADDING_ = 22;
    static constexpr auto BOTTOM_PADDING_ = 22;
    static constexpr auto CONTENT_HEIGHT_ = 28;
    static constexpr auto DISPLAY_WIDTH_ = 46;

    void setup_()
    {
        // setAttribute(Qt::WA_StyledBackground, true); // TODO: For QSS
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setupButtons_();

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(0);
        layout->addWidget(minusButton_);
        layout->addWidget(display_);
        layout->addWidget(plusButton_);

        connect(minusButton_, &QPushButton::clicked, this, [this] {
            step_(-1);
        });

        connect(display_, &QPushButton::clicked, this, [this] {
            toggleMode_();
        });

        connect(plusButton_, &QPushButton::clicked, this, [this] { step_(1); });

        display_->installEventFilter(this);
        if (auto p = parent()) p->installEventFilter(this);

        reposition_();
        updateDisplay_();
    }

    void setupButtons_()
    {
        // TODO: Refine button hover/idle contrast maybe

        auto text_color = QColor(255, 255, 255, 210);
        auto button_palette = QPalette{};
        button_palette.setColor(QPalette::Button, QColor(255, 255, 255, 30));
        button_palette.setColor(QPalette::ButtonText, text_color);

        auto zoom_font = QFont{};
        zoom_font.setPixelSize(16);
        zoom_font.setBold(true);

        for (auto button : { minusButton_, plusButton_ }) {
            button->setFixedSize(CONTENT_HEIGHT_, CONTENT_HEIGHT_);
            button->setFocusPolicy(Qt::NoFocus);
            button->setFlat(true);
            button->setPalette(button_palette);
            button->setFont(zoom_font);
        }

        auto display_font = QFont{};
        display_font.setPixelSize(12);
        display_font.setBold(true);

        display_->setFixedSize(DISPLAY_WIDTH_, CONTENT_HEIGHT_);
        display_->setFlat(true);
        display_->setPalette(button_palette);
        display_->setFont(display_font);
    }

    void reposition_()
    {
        auto parent_widget = parentWidget();
        if (!parent_widget) return;

        adjustSize();

        auto x = parent_widget->width() - width() - RIGHT_PADDING_;
        auto y = parent_widget->height() - height() - BOTTOM_PADDING_;
        move(x, y);
    }

    void updateDisplay_()
    {
        (mode_ == Fit) ? display_->setText(QStringLiteral("Fit"))
                       : display_->setText(QStringLiteral("%1%").arg(
                             qRound(factor_ * 100.0)));
    }

    void toggleMode_()
    {
        mode_ = (mode_ == Fit) ? Fixed : Fit;
        updateDisplay_();

        emit zoomChanged(mode_, factor_);
    }

    void step_(int direction)
    {
        mode_ = Fixed;
        auto new_factor = factor_ + (STEP_ * direction);
        factor_ = qBound(MIN_FACTOR_, new_factor, MAX_FACTOR_);
        updateDisplay_();

        emit zoomChanged(mode_, factor_);
    }

    void goTo100_()
    {
        mode_ = Fixed;
        factor_ = 1.0;
        updateDisplay_();

        emit zoomChanged(mode_, factor_);
    }
};

} // namespace Fernanda
