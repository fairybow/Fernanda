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

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QStyle>
#include <QToolTip>
#include <QWidget>

#include <Coco/Concepts.h>

#include "core/Debug.h"
#include "core/Time.h"
#include "ui/Painting.h"

namespace Hearth {

enum class FieldKind
{
    Label,
    Info,
    LabelAndInfo
};

template <Coco::Concepts::QWidgetDerived QWidgetT>
class ControlField : public QWidget
{
public:
    explicit ControlField(FieldKind kind, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_(kind);
    }

    virtual ~ControlField() override
    {
        TRACER;
        if (infoPopup_) delete infoPopup_;
    }

    QWidgetT* control() { return control_; }

    void setText(const QString& text)
    {
        if (label_) label_->setText(text);
    }

    void setInfo(const QString& info) { infoText_ = info; }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (info_ && watched == info_) {
            switch (event->type()) {

            case QEvent::Enter:
                if (!clickLocked_) hoverDelay_->start();
                return false;

            case QEvent::Leave:
                if (!clickLocked_) hideInfoPopup_();
                return false;

            case QEvent::MouseButtonPress:
                hoverDelay_->stop();
                clickLocked_ = true;
                showInfoPopup_();
                Time::delay(3000, this, [this] { hideInfoPopup_(); });
                return true;

            default:
                break;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

private:
    QLabel* label_ = nullptr;

    QLabel* info_ = nullptr;
    QLabel* infoPopup_ = nullptr;
    Time::Delayer* hoverDelay_ = nullptr;
    QString infoText_{};
    bool clickLocked_ = false;

    QWidgetT* control_ = new QWidgetT(this);

    void setup_(FieldKind kind)
    {
        setFocusProxy(control_);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        switch (kind) {

        default:
        case FieldKind::Label: {
            label_ = new QLabel(this);

            layout->addWidget(label_, 0);
            layout->addWidget(control_, 1);

            break;
        }

        case FieldKind::Info: {
            info_ = new QLabel(this);
            setupInfoPopup_();
            setupInfoIcon_();

            layout->addWidget(control_);
            layout->addWidget(info_);

            break;
        }

        case FieldKind::LabelAndInfo: {
            label_ = new QLabel(this);
            info_ = new QLabel(this);
            setupInfoPopup_();
            setupInfoIcon_();

            layout->addWidget(label_, 0);
            layout->addWidget(control_, 1);
            layout->addWidget(info_, 0);

            break;
        }
        }

        // TODO: This seems to be fine but double-check!
        layout->addStretch();
    }

    void setupInfoPopup_()
    {
        if (!info_) return;
        hoverDelay_ = Time::newDelayer(this, [this] { showInfoPopup_(); }, 400);
        info_->installEventFilter(this);
    }

    // TODO: StyleContext support
    void setupInfoIcon_()
    {
        if (!info_) return;

        auto size = 14;
        auto padding = 2;
        auto total = size + padding * 2;

        QPixmap icon(total, total);
        icon.fill(Qt::transparent);

        QPainter painter(&icon);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#3B82F6"));
        painter.drawEllipse(padding, padding, size, size);

        QFont font = painter.font();
        font.setPixelSize(11);
        font.setBold(true);
        font.setItalic(true);
        font.setFamily("Times New Roman");
        font.setStyleHint(QFont::Serif);
        painter.setFont(font);
        painter.setPen(Qt::white);

        auto draw_rect = Painting::centeredGlyphRect(
            font,
            "i",
            QRectF(padding, padding, size, size));
        painter.drawText(draw_rect, Qt::AlignCenter, "i");

        painter.end();

        info_->setPixmap(icon);
    }

    void showInfoPopup_()
    {
        if (!info_ || infoText_.isEmpty()) return;

        if (!infoPopup_) {
            infoPopup_ = new QLabel;
            infoPopup_->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
            infoPopup_->setPalette(QToolTip::palette());
            infoPopup_->setFont(QToolTip::font());
            infoPopup_->setMargin(4);
        }

        infoPopup_->setText(infoText_);
        infoPopup_->adjustSize();
        infoPopup_->move(info_->mapToGlobal(QPoint{ 0, info_->height() + 2 }));
        infoPopup_->show();
    }

    void hideInfoPopup_()
    {
        hoverDelay_->stop();
        delete infoPopup_;
        infoPopup_ = nullptr;
        clickLocked_ = false;
    }
};

} // namespace Hearth
