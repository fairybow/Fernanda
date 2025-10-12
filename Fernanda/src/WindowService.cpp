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

void WindowService::setup_()
{
    connect(
        app(),
        &Application::focusChanged,
        this,
        &WindowService::onApplicationFocusChanged_);
}

} // namespace Fernanda
