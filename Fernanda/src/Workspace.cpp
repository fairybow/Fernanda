#include <QAbstractItemModel>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"

#include "AboutDialog.h"
#include "Application.h"
#include "Commander.h"
#include "Utility.h"
#include "Window.h"
#include "WindowService.h"
#include "Workspace.h"

namespace Fernanda {

void Workspace::addCommandHandlers_()
{
    commander->addCommandHandler(Commands::NewWindow, [&] { newWindow_(); });

    commander->addCommandHandler(
        Commands::CloseWindow,
        [&](const Command& cmd) {
            if (cmd.context) cmd.context->close();
        });

    commander->addCommandHandler(Commands::CloseAllWindows, [&] {
        // Close each window individually, triggering the CloseAcceptor for each
        for (auto window : windows_->windowsReversed()) {
            if (!window) continue;
            if (!window->close()) return;
        }
    });

    commander->addCommandHandler(Commands::Quit, [] { Application::quit(); });

    commander->addCommandHandler(Commands::AboutDialog, [] {
        AboutDialog::exec();
    });

    commander->addQueryHandler(Queries::Root, [&] {
        return rootPath.toQString();
    });

    commander->addCallHandler(Calls::NewTreeViewModel, [&] {
        return toQVariant(makeTreeViewModel_());
    });
}

} // namespace Fernanda
