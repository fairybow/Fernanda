/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
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

#include "core/app/AppActions.h"
#include "core/dist/Updater.h"
#include "ui/UiConstants.h"
#include "ui/windows/BaseWindow.h"

namespace Fernanda::Ui {

// TODO: Should this just be a QWidget?
class PopoutWindow : public BaseWindow
{
    Q_OBJECT

public:
    // A top-level sibling of its VaultWindow, not a Qt child, since an owned
    // window can never stack behind its owner. The VaultWindow lists its
    // pop-outs and tears them down itself, as Qt won't.
    //
    // AppActions are adopted for their shortcuts, not shown (a pop-out has no
    // menu), so Ctrl+Q reaches App directly from a pop-out, not relayed through
    // the VaultWindow. The Updater, App's too, feeds the status bar's update
    // notice, whose click the VaultWindow wires.
    //
    // No closeEvent: a pop-out holds no state. Buffers live in the store, so
    // closing one loses nothing, and VaultWindow::closeEvent's flush covers
    // files open here
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
