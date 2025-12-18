/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "CollapsibleWidget.h"
#include "Debug.h"

namespace Fernanda {

// Vertically stacked container for CollapsibleWidgets with dynamic space
// distribution. Collapsed sections anchor to the top; expanded sections share
// available space equally. Requests a minimum height when any section is
// expanded to ensure usable content area within splitter layouts.
class AccordionWidget : public QWidget
{
    Q_OBJECT

public:
    AccordionWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~AccordionWidget() override { TRACER; }

    void addWidget(const QString& title, QWidget* widget)
    {
        if (!widget) return;

        auto collapsible = new CollapsibleWidget(title, widget, this);
        collapsibles_ << collapsible;

        // Insert before the spacer
        auto index = layout_->indexOf(spacer_);
        layout_->insertWidget(index, collapsible, 0);

        connect(
            collapsible,
            &CollapsibleWidget::expandedChanged,
            this,
            &AccordionWidget::updateStretchFactors_);

        updateStretchFactors_();
    }

private:
    QVBoxLayout* layout_ = nullptr;
    QWidget* spacer_ = new QWidget(this);
    QSet<CollapsibleWidget*> collapsibles_{};

    void setup_()
    {
        layout_ = new QVBoxLayout(this);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->setSpacing(0);

        layout_->addWidget(spacer_, 1);
    }

    void updateStretchFactors_()
    {
        auto any_expansions = false;

        for (auto& collapsible : collapsibles_)
            if (collapsible->isExpanded()) any_expansions = true;

        for (auto& collapsible : collapsibles_) {
            auto index = layout_->indexOf(collapsible);
            layout_->setStretch(index, collapsible->isExpanded() ? 1 : 0);
        }

        auto spacer_index = layout_->indexOf(spacer_);
        layout_->setStretch(spacer_index, any_expansions ? 0 : 1);

        // Set a fixed minimum for the accordion when anything is expanded
        setMinimumHeight(any_expansions ? 200 : 0);
    }
};

} // namespace Fernanda
