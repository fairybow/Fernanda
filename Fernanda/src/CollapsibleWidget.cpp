/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "CollapsibleWidget.h"

#include <QEvent>
#include <QWidget>

#include "Application.h"
#include "CollapsibleWidgetHeader.h"

namespace Fernanda {

void CollapsibleWidget::setContentExpanded_(bool expanded)
{
    expanded_ = expanded;
    content_->setMaximumHeight(expanded ? QWIDGETSIZE_MAX : 0);

    // Clear stale hover state after layout change
    // TODO: Review/also use App
    if (header_->underMouse()) {
        QEvent leave_event(QEvent::Leave);
        Application::sendEvent(header_, &leave_event);
    }

    header_->setChecked(
        expanded); // This isn't needed now (since a checkable button sets
                   // itself checked when pressed), but would be needed if
                   // we programmatically changed expanded state

    emit expandedChanged(expanded);
}

} // namespace Fernanda
