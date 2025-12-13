/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QAbstractItemModel>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"

#include "AboutDialog.h"
#include "Application.h"
#include "Constants.h"
#include "Window.h"
#include "WindowService.h"
#include "Workspace.h"

namespace Fernanda {

void Workspace::registerBusCommands_()
{
    bus->addCommandHandler(Commands::NEW_NOTEBOOK, [&] { 
        
        // - Get name from dialog
        // - concat: startDir + name + .fnx
        // - check exists already
        // - if exists, error dialog
        // - else, emit newNotebookRequested with path in signal
        
        });

    bus->addCommandHandler(Commands::OPEN_NOTEBOOK, [] {});
    bus->addCommandHandler(Commands::ABOUT_DIALOG, [] { AboutDialog::exec(); });
}

} // namespace Fernanda
