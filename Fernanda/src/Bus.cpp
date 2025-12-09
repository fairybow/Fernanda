/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QModelIndex>
#include <QPoint>

#include "Coco/Path.h"

#include "AbstractFileModel.h"
#include "Bus.h"
#include "Debug.h"
#include "IFileView.h"
#include "Window.h"

// Not technically a good idea, since a comma in the lambda capture will break
// the macro. It should be fine here, though, where we only ever need &
#define SIGLOG_(Signal, Slot) connect(this, &Bus::Signal, this, Slot)

namespace Fernanda {

void Bus::setup_()
{
    SIGLOG_(lastWindowClosed, [&] { INFO("Last window closed"); });

    SIGLOG_(windowCreated, [&](Window* context) {
        INFO("Window created [{}]", context);
    });

    SIGLOG_(windowDestroyed, [&](Window* context) {
        INFO("Window destroyed [{}]", context);
    });

    SIGLOG_(activeFileViewChanged, [&](Window* context, IFileView* fileView) {
        INFO("Active view changed in [{}] to [{}]", context, fileView);
    });

    SIGLOG_(
        treeViewDoubleClicked,
        [&](Window* context, const QModelIndex& index) {
            INFO(
                "Tree view double-clicked in [{}]: index [{}]",
                context,
                index);
        });

    SIGLOG_(
        fileModelReadied,
        [&](Window* context, AbstractFileModel* fileModel) {
            INFO("File model readied in [{}]: [{}]", context, fileModel);
        });

    SIGLOG_(
        fileModelModificationChanged,
        [&](AbstractFileModel* fileModel, bool modified) {
            INFO(
                "File model [{}] modification changed to {}",
                fileModel,
                modified);
        });

    SIGLOG_(fileModelMetaChanged, [&](AbstractFileModel* fileModel) {
        INFO("File model [{}] metadata changed", fileModel);
    });

    SIGLOG_(
        treeViewContextMenuRequested,
        [&](Window* context,
            const QPoint& globalPos,
            const QModelIndex& index) {
            INFO(
                "Tree view context menu in [{}] at {} for index [{}]",
                context,
                globalPos,
                index);
        });

    SIGLOG_(viewDestroyed, [&](AbstractFileModel* fileModel) {
        INFO("View destroyed for model [{}]", fileModel);
    });

    // TODO: Updated regularly
}

} // namespace Fernanda

#undef SIGLOG_
