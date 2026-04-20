/*
 * Hearth — a plain-text-first workbench for creative writing
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
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

#include "core/Debug.h"
#include "ui/Icons.h"
#include "ui/ZoomState.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// TODO: (Maybe) hide/fade out after a set linger time. Reappear on hover in its
// general area (bottom-right). Begin visible, though, so it's obvious the
// widget is there to users
// TODO: Allow typing percentage into display
// TODO: Buttons are a little too far from left and right edges respectively
// TODO: Widget itself is too far right (overlaps with scroll bar slightly and
// also over edge of fitted PDFs
// TODO: Corners look too sharp
// TODO: Double click to change mode
class ZoomControl : public QWidget
{
    Q_OBJECT

public:
    ZoomControl(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~ZoomControl() override { TRACER; }

    // Called by the owning view after any zoom state change
    void setDisplayText(const QString& text) { display_->setText(text); }

signals:
    void stepRequested(ZoomState::Step direction);
    void toggleModeRequested(); // left-click display
    void resetRequested(); // right-click display

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        // Resize with parent
        if (watched == parent() && event->type() == QEvent::Resize) {
            reposition_();
        }

        // Reset on right-click
        if (watched == display_ && event->type() == QEvent::MouseButtonPress) {
            auto mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event && mouse_event->button() == Qt::RightButton) {
                emit resetRequested();
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
    QPushButton* minusButton_ = new QPushButton(this);
    QPushButton* display_ = new QPushButton(u"Fit"_s, this);
    QPushButton* plusButton_ = new QPushButton(this);

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
            emit stepRequested(ZoomState::Out);
        });

        connect(display_, &QPushButton::clicked, this, [this] {
            emit toggleModeRequested();
        });

        connect(plusButton_, &QPushButton::clicked, this, [this] {
            emit stepRequested(ZoomState::In);
        });

        display_->installEventFilter(this);
        if (auto p = parent()) p->installEventFilter(this);

        reposition_();
    }

    void setupButtons_()
    {
        // TODO: Refine button hover/idle contrast maybe

        auto text_color = QColor(255, 255, 255, 210);
        auto button_palette = QPalette{};
        button_palette.setColor(QPalette::Button, QColor(255, 255, 255, 30));
        button_palette.setColor(QPalette::ButtonText, text_color);

        for (auto button : { minusButton_, plusButton_ }) {
            button->setFixedSize(CONTENT_HEIGHT_, CONTENT_HEIGHT_);
            button->setFocusPolicy(Qt::NoFocus);
            button->setFlat(true);
            button->setPalette(button_palette);
        }

        auto display_font = QFont{};
        display_font.setPixelSize(12);
        display_font.setWeight(QFont::DemiBold);

        display_->setFixedSize(DISPLAY_WIDTH_, CONTENT_HEIGHT_);
        display_->setFlat(true);
        display_->setPalette(button_palette);
        display_->setFont(display_font);

        auto icon_size = QSize(14, 14);
        auto dpr = devicePixelRatioF();
        minusButton_->setIcon(
            Icons::get(UiIcon::Minus, icon_size, Qt::white, dpr));
        minusButton_->setIconSize(icon_size);
        plusButton_->setIcon(
            Icons::get(UiIcon::Plus, icon_size, Qt::white, dpr));
        plusButton_->setIconSize(icon_size);
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
};

} // namespace Fernanda
