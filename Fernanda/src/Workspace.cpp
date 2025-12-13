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
#include "Coco/PathUtil.h"

#include "AboutDialog.h"
#include "Application.h"
#include "Constants.h"
#include "Fnx.h"
#include "NewNotebookPrompt.h"
#include "Tr.h"
#include "Window.h"
#include "WindowService.h"
#include "Workspace.h"

namespace Fernanda {

void Workspace::registerBusCommands_()
{
    // Will allow creation of new Notebook with a prospective path that is the
    // same as an existing Notebook's. When saved, the user will be warned
    // before saving over the existing Notebook!
    bus->addCommandHandler(Commands::NEW_NOTEBOOK, [&] {
        auto name = NewNotebookPrompt::exec();
        if (name.isEmpty()) return;
        emit newNotebookRequested(startDir / (name + Fnx::EXT));
    });

    bus->addCommandHandler(Commands::OPEN_NOTEBOOK, [&] {
        // nullptr parent makes the dialog application modal
        auto path = Coco::PathUtil::Dialog::file(
            nullptr,
            Tr::nxOpenNotebookCaption(),
            startDir,
            Tr::nxOpenNotebookCaption());

        if (path.isEmpty() || !Fnx::Io::isFnxFile(path)) return;
        emit openNotebookRequested(path);
    });

    bus->addCommandHandler(Commands::ABOUT_DIALOG, [] { AboutDialog::exec(); });
}

} // namespace Fernanda
