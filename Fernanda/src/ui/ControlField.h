/*
 * Fernanda — a plain-text-first workbench for creative writing
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
#include <QRectF>
#include <QString>
#include <QStyle>
#include <QWidget>

#include <Coco/Concepts.h>

#include "core/Debug.h"
#include "ui/Painting.h"

namespace Fernanda {

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

    virtual ~ControlField() override { TRACER; }

    QWidgetT* control() { return control_; }

    void setText(const QString& text)
    {
        if (label_) label_->setText(text);
    }

    void setInfo(const QString& info)
    {
        if (info_) info_->setToolTip(info);
    }

private:
    QLabel* label_ = nullptr;
    QLabel* info_ = nullptr;
    QWidgetT* control_ = new QWidgetT(this);

    void setup_(FieldKind kind)
    {
        setFocusProxy(control_);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        // TODO: Click the label here and in CFI to show tooltip also? Then
        // don't auto show on hover if already clicked?

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
            setInfoIcon_();

            layout->addWidget(control_);
            layout->addWidget(info_);

            break;
        }

        case FieldKind::LabelAndInfo: {
            label_ = new QLabel(this);
            info_ = new QLabel(this);
            setInfoIcon_();

            layout->addWidget(label_, 0);
            layout->addWidget(control_, 1);
            layout->addWidget(info_, 0);

            break;
        }
        }

        // TODO: This seems to be fine but double-check!
        layout->addStretch();
    }

    // TODO: StyleContext support
    void setInfoIcon_()
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
};

} // namespace Fernanda
