/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <type_traits>

#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QWidget>

#include "Coco/Concepts.h"

#include "Debug.h"

namespace Fernanda {

template <Coco::Concepts::QWidgetPointer QWidgetT>
class ControlField : public QWidget
{
public:
    explicit ControlField(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~ControlField() override { TRACER; }

    QWidgetT control() { return control_; }
    void setText(const QString& text) { label_->setText(text); }

private:
    QWidgetT control_ = new std::remove_pointer_t<QWidgetT>(this);
    QLabel* label_ = new QLabel(this);

    void setup_()
    {
        setFocusProxy(control_);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(label_, 0);
        layout->addWidget(control_, 1);
    }
};

} // namespace Fernanda
