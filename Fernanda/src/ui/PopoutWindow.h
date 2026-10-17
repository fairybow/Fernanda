/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <Coco/Debug.h>

#include "core/AppActions.h"
#include "core/Updater.h"
#include "ui/BaseWindow.h"
#include "ui/UiConstants.h"

namespace Fernanda::Ui {

// TODO: Should this just be a QWidget?
class PopoutWindow : public BaseWindow
{
    Q_OBJECT

public:
    // A top-level sibling of its VaultWindow, not a Qt child of it: an owned
    // window can never stack behind its owner. The VaultWindow keeps pop-outs
    // in a list and tears them down itself, since Qt won't.
    //
    // Takes AppActions only to pass up: a pop-out has no menu, so it displays
    // none of them, but adopting gives it their shortcuts — Ctrl+Q works from a
    // focused pop-out, and it reaches App directly rather than being relayed
    // through the owning VaultWindow. The Updater is App's too, for the status
    // bar's update notice, whose click the VaultWindow wires.
    //
    // Deliberately no closeEvent: a pop-out has no state to flush. Buffers live
    // in the Vault, so the one flush in VaultWindow::closeEvent covers the
    // files open here too. Closing a pop-out on its own saves nothing and loses
    // nothing — the buffer stays in the Vault, whether or not it's open
    // elsewhere
    PopoutWindow(const AppActions& appActions, Updater* updater)
        : BaseWindow(appActions, updater)
    {
        setup_();
    }

    ~PopoutWindow() override { TRACER; }

private:
    void setup_()
    {
        resize(DEFAULT_POPOUT_WINDOW_WIDTH, DEFAULT_POPOUT_WINDOW_HEIGHT);

        // Pop-outs get splits too, and no sidebar
        setCentralWidget(tabPaneTree());
    }
};

} // namespace Fernanda::Ui
