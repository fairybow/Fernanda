#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"

#include "AboutDialog.h"
#include "Application.h"
#include "Commander.h"
#include "EventBus.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "Utility.h"
#include "Window.h"
#include "WindowService.h"
#include "Workspace.h"

namespace Fernanda {

void Workspace::initialize_()
{
    windows_->setCloseAcceptor(this, &Workspace::windowsCloseAcceptor_);

    commander_->addInterceptor(Commands::OpenFile, [&](Command& cmd) {
        if (pathInterceptor_
            && pathInterceptor_(to<QString>(cmd.params, "path"))) {
            return true;
        }

        return false;
    });

    commander_->addCommandHandler(Commands::NewWindow, [&] { newWindow(); });

    commander_->addCommandHandler(
        Commands::CloseWindow,
        [&](const Command& cmd) {
            if (cmd.context) cmd.context->close();
        });

    commander_->addCommandHandler(Commands::CloseAllWindows, [&] {
        // Close each window individually, triggering the CloseAcceptor for each
        for (auto window : windows_->windowsReversed()) {
            if (!window) continue;
            if (!window->close()) return;
        }
    });

    commander_->addCommandHandler(Commands::Quit, [] { Application::quit(); });

    commander_->addCommandHandler(Commands::AboutDialog, [] {
        AboutDialog::exec();
    });

    commander_->addQueryHandler(Queries::Root, [&] {
        return root_.toQString();
    });

    // emit eventBus_->workspaceInitialized(this); // Later, if needed
}

} // namespace Fernanda
