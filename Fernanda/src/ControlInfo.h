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
#include <QPixmap>
#include <QStyle>
#include <QWidget>

#include "Coco/Concepts.h"

#include "Debug.h"

namespace Fernanda {

template <Coco::Concepts::QWidgetPointer QWidgetT>
class ControlInfo : public QWidget
{
public:
    explicit ControlInfo(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~ControlInfo() override { TRACER; }

    QWidgetT control() { return control_; }
    void setInfo(const QString& info) { info_->setToolTip(info); }

private:
    QWidgetT control_ = new std::remove_pointer_t<QWidgetT>(this);
    QLabel* info_ = new QLabel(this);

    void setup_()
    {
        setFocusProxy(control_);

        info_->setPixmap(
            style()
                ->standardPixmap(QStyle::SP_MessageBoxInformation)
                .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(control_);
        layout->addWidget(info_);
        layout->addStretch();
    }
};

} // namespace Fernanda
