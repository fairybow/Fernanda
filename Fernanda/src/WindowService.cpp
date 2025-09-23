/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QEventLoop>
#include <QTime>
#include <QVariant>
#include <QVariantMap>

#include "Application.h"
#include "Bus.h"
#include "WindowService.h"

namespace Fernanda {

void WindowService::initialize_()
{
    bus->addCommandHandler(Commands::PreviousWindow, [&] {
        activatePrevious_();
    });

    bus->addCommandHandler(Commands::ViewNextWindow, [&] { activateNext_(); });

    bus->addQueryHandler(Queries::ActiveWindow, [&] {
        return toQVariant(activeWindow_);
    });

    bus->addQueryHandler(Queries::WindowList, [&] {
        return toQVariant(windows());
    });

    bus->addQueryHandler(Queries::ReverseWindowList, [&] {
        return toQVariant(windowsReversed());
    });

    bus->addQueryHandler(Queries::WindowSet, [&] {
        return toQVariant(windowsUnordered());
    });

    bus->addQueryHandler(Queries::VisibleWindowCount, [&] {
        return visibleCount();
    });

    connect(
        app(),
        &Application::focusChanged,
        this,
        &WindowService::onApplicationFocusChanged_);
}

// https://stackoverflow.com/a/11487434
// Questionable
void WindowService::bubbleDelay_(unsigned int msecs) const
{
    auto die_time = QTime::currentTime().addMSecs(msecs);

    while (QTime::currentTime() < die_time)
        Application::processEvents(QEventLoop::AllEvents, 100);
}

} // namespace Fernanda
