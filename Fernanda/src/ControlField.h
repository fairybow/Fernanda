/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QStyle>
#include <QWidget>

#include "Coco/Concepts.h"

#include "Debug.h"

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

        QFontMetricsF metrics(font);
        auto glyph_rect = metrics.boundingRect("i");
        auto offset = glyph_rect.left();

        painter.drawText(
            QRectF(padding + offset / 2.0, padding, size, size),
            Qt::AlignCenter,
            "i");
        painter.end();

        info_->setPixmap(icon);
    }
};

} // namespace Fernanda
