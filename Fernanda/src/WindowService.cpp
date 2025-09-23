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
#include "Constants.h"
#include "WindowService.h"

namespace Fernanda {

void WindowService::initialize_()
{
    /*bus->addCommandHandler(Cmd::PreviousWindow, [&] { activatePrevious_(); });
    bus->addCommandHandler(Cmd::ViewNextWindow, [&] { activateNext_(); });
    bus->addCommandHandler(Cmd::ActiveWindow, [&] { return activeWindow_.get();
    }); bus->addCommandHandler(Cmd::WindowList, [&] { return windows(); });

    bus->addCommandHandler(Cmd::ReverseWindowList, [&] {
        return windowsReversed();
    });

    bus->addCommandHandler(Cmd::WindowSet, [&] { return windowsUnordered(); });

    bus->addCommandHandler(Cmd::VisibleWindowCount, [&] {
        return visibleCount();
    });*/

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
