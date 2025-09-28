/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QString>
#include <QVariant>

#include "Coco/Log.h"
#include "Coco/Path.h"

#include "Bus.h"
#include "Debug.h"
#include "Enums.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "Utility.h"
#include "Window.h"

#define SIGLOG_(Signal, Slot) connect(this, &Bus::Signal, this, Slot)

namespace Fernanda {

void Bus::initialize_()
{
    SIGLOG_(workspaceInitialized, [&] {});
    SIGLOG_(windowCreated, [&](Window* window) {});
    SIGLOG_(visibleWindowCountChanged, [&](int count) {});
    SIGLOG_(lastWindowClosed, [&] {});
    SIGLOG_(activeWindowChanged, [&](Window* window) {
        INFO("Active window = {}", window);
    });
    SIGLOG_(windowDestroyed, [&](Window* window) {});
    SIGLOG_(fileReadied, [&](IFileModel* model, Window* window) {});
    SIGLOG_(fileModificationChanged, [&](IFileModel* model, bool modified) {});
    SIGLOG_(fileMetaChanged, [&](IFileModel* model) {});
    SIGLOG_(fileSaved, [&](SaveResult result, const Coco::Path& path) {
        INFO("File [{}] saved: {}", path, result);
    });
    SIGLOG_(
        fileSavedAs,
        [&](SaveResult result,
            const Coco::Path& path,
            const Coco::Path& oldPath) {
            INFO("File [{}] saved as [{}]: {}", oldPath, path, result);
        });
    SIGLOG_(windowSaveExecuted, [&](Window* window, SaveResult result) {});
    SIGLOG_(workspaceSaveExecuted, [&](SaveResult result) {});
    SIGLOG_(windowTabCountChanged, [&](Window* window, int count) {});
    SIGLOG_(activeFileViewChanged, [&](IFileView* view, Window* window) {
        if (!window) return;
        INFO("Active view for {} = {}", window, view);
    });
    SIGLOG_(viewClosed, [&](IFileView* view) {});
    SIGLOG_(settingChanged, [&](const QString& key, const QVariant& value) {});
}

} // namespace Fernanda

#undef SIGLOG_
