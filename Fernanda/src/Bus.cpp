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
        COCO_LOG_THIS(QString("\n\tActive window = %0").arg(toQString(window)));
    });

    SIGLOG_(windowDestroyed, [&](Window* window) {});
    SIGLOG_(fileReadied, [&](IFileModel* model, Window* window) {});
    SIGLOG_(fileModificationChanged, [&](IFileModel* model, bool modified) {});
    SIGLOG_(fileMetaChanged, [&](IFileModel* model) {});

    SIGLOG_(fileSaved, [&](SaveResult result, const Coco::Path& path) {
        COCO_LOG_THIS(QString("\n\tFile [%0] saved: %1")
                          .arg(path.toQString())
                          .arg(toQString(result)));
    });

    SIGLOG_(
        fileSavedAs,
        [&](SaveResult result,
            const Coco::Path& path,
            const Coco::Path& oldPath) {
            COCO_LOG_THIS(QString("\n\tFile [%0] saved as [%1]: %2")
                              .arg(oldPath.toQString())
                              .arg(path.toQString())
                              .arg(toQString(result)));
        });

    SIGLOG_(windowSaveExecuted, [&](Window* window, SaveResult result) {});
    SIGLOG_(workspaceSaveExecuted, [&](SaveResult result) {});
    SIGLOG_(windowTabCountChanged, [&](Window* window, int count) {});

    SIGLOG_(activeFileViewChanged, [&](IFileView* view, Window* window) {
        if (!window) return;
        COCO_LOG_THIS(QString("\n\tActive file view for %0 = %1")
                          .arg(toQString(window))
                          .arg(toQString(view)));
    });

    SIGLOG_(viewClosed, [&](IFileView* view) {});
    SIGLOG_(settingChanged, [&](const QString& key, const QVariant& value) {});
}

} // namespace Fernanda

#undef SIGLOG_
