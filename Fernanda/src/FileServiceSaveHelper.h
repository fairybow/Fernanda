#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>

#include "Coco/Debug.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Commander.h"
#include "EventBus.h"
#include "FileMeta.h"
#include "IFileModel.h"
#include "TabWidget.h"
#include "Tr.h"
#include "Utility.h"
#include "Window.h"

namespace Fernanda {

// Handles all save operations for FileService including save dialogs, path
// management, multi-Window coordination, and save event emission across the
// Workspace
//
// Note: May be reintegrated with FileService later
class FileServiceSaveHelper : public QObject
{
    Q_OBJECT

public:
    FileServiceSaveHelper(
        Commander* commander,
        EventBus* eventBus,
        QHash<Coco::Path, IFileModel*>& pathToFileModel,
        QObject* parent = nullptr)
        : QObject(parent)
        , commander_(commander)
        , eventBus_(eventBus)
        , pathToFileModel_(pathToFileModel)
    {
    }

    virtual ~FileServiceSaveHelper() override { COCO_TRACER; }

    [[nodiscard]] SaveResult saveAt(Window* window, int index)
    {
        return offDiskCascade_(window, index);
    }

    [[nodiscard]] SaveResult saveAsAt(Window* window, int index)
    {
        if (!window) return SaveResult::NoOp;
        auto model = modelAt(window, index);
        if (!model) return SaveResult::NoOp;

        // This might not have been off-disk, as we can Save As something with a
        // path
        return setTabAndPromptAndSaveAs_(window, model, index);
    }

    [[nodiscard]] SaveResult
    saveIndexesInWindow(Window* window, const QList<int>& indexes)
    {
        if (!window) return SaveResult::NoOp;

        auto any_fails = false;
        auto any_successes = false;

        for (auto& i : indexes) {
            auto result_i = offDiskCascade_(window, i);
            setFailOrSuccess_(result_i, any_fails, any_successes);
        }

        return priorityReturn_(any_fails, any_successes);
    }

    [[nodiscard]] SaveResult saveWindow(Window* window)
    {
        if (!window) return SaveResult::NoOp;

        auto any_fails = false;
        auto any_successes = false;

        for (auto& i : reverseUniqueModelIndexes_(window)) {
            auto result_i = offDiskModifiedCascade_(window, i);
            setFailOrSuccess_(result_i, any_fails, any_successes);
        }

        return priorityReturn_(any_fails, any_successes);
    }

    [[nodiscard]] SaveResult saveAll()
    {
        auto any_fails = false;
        auto any_successes = false;

        // Reverse windows here or not?
        for (auto& window :
             commander_->query<QList<Window*>>(Queries::ReverseWindowList)) {
            if (!window) continue;

            for (auto& i : reverseUniqueModelIndexes_(window)) {
                auto result_i = offDiskModifiedCascade_(window, i);
                setFailOrSuccess_(result_i, any_fails, any_successes);
            }
        }

        return priorityReturn_(any_fails, any_successes);
    }

private:
    struct PathChange
    {
        Coco::Path old{};
        Coco::Path now{};
    };

    Commander* commander_; // FileService's
    EventBus* eventBus_; // FileService's
    QHash<Coco::Path, IFileModel*>& pathToFileModel_;

    void
    setFailOrSuccess_(SaveResult result, bool& anyFails, bool& anySuccesses)
    {
        switch (result) {
        default:
        case SaveResult::NoOp:
            break;
        case SaveResult::Success:
            anySuccesses = true;
            break;
        case SaveResult::Fail:
            anyFails = true;
            break;
        }
    }

    SaveResult offDiskCascade_(Window* window, int index)
    {
        if (!window) return SaveResult::NoOp;
        auto model = modelAt(window, index);
        if (!model) return SaveResult::NoOp;
        auto meta = model->meta();
        if (!meta) return SaveResult::NoOp;

        if (!meta->isOnDisk()) {
            return setTabAndPromptAndSaveAs_(window, model, index);
        }

        return saveModel_(model);
    }

    SaveResult offDiskModifiedCascade_(Window* window, int index)
    {
        if (!window) return SaveResult::NoOp;
        auto model = modelAt(window, index);
        if (!model) return SaveResult::NoOp;
        auto meta = model->meta();
        if (!meta) return SaveResult::NoOp;

        if (!meta->isOnDisk() && model->isModified()) {
            return setTabAndPromptAndSaveAs_(window, model, index);
        }

        return saveModel_(model);
    }

    SaveResult
    setTabAndPromptAndSaveAs_(Window* window, IFileModel* model, int index)
    {
        if (!window || !model) return SaveResult::NoOp;

        window->activate();
        if (auto tab_widget = tabWidget(window))
            tab_widget->setCurrentIndex(index);

        auto path_change = saveAsDialog_(window, model);
        return saveModelAs_(model, path_change);
    }

    SaveResult priorityReturn_(bool anyFails, bool anySuccesses)
    {
        return anyFails       ? SaveResult::Fail
               : anySuccesses ? SaveResult::Success
                              : SaveResult::NoOp;
    }

    // Core save-handling logic, no dialogs, no events
    SaveResult saveModel_(IFileModel* model)
    {
        if (!model || !model->supportsModification() || !model->isModified())
            return SaveResult::NoOp;
        auto meta = model->meta();
        if (!meta || !meta->isOnDisk()) return SaveResult::NoOp;

        auto result = model->save();
        emit eventBus_->fileSaved(result, meta->path());
        return result;
    }

    // Core save-handling logic, no dialogs, no events
    SaveResult saveModelAs_(IFileModel* model, const PathChange& pathChange)
    {
        if (!model || !model->supportsModification()) return SaveResult::NoOp;
        if (pathChange.now.isEmpty()) return SaveResult::NoOp;

        // Don't overwrite
        if (auto existing_model = pathToFileModel_[pathChange.now])
            if (existing_model != model) return SaveResult::Fail;

        auto result =
            model->saveAs(pathChange.now); // Changes meta path if successful

        // Assign new path if necessary
        if (result == SaveResult::Success) {
            if (!pathChange.old.isEmpty())
                pathToFileModel_.remove(pathChange.old);
            pathToFileModel_[pathChange.now] = model;
        }

        emit eventBus_->fileSavedAs(result, pathChange.now, pathChange.old);
        return result;
    }

    /// WIP
    PathChange saveAsDialog_(Window* window, IFileModel* model) const
    {
        if (!model) return {};
        auto meta = model->meta();
        if (!meta) return {};

        auto old_path = meta->path();
        /// auto title = meta->title(); // Add an arg to the tr function
        /// auto ext_filter = meta->extFilter();

        auto new_path = Coco::PathUtil::Dialog::save(
            window,
            Tr::Dialogs::saveFileCaption(),
            old_path.isEmpty() ? commander_->query<QString>(Queries::Root)
                               : old_path); /// Filter arg is last arg (not
                                            /// present here), do later

        return { old_path, new_path };
    }

    QList<int> reverseUniqueModelIndexes_(Window* window)
    {
        if (!window) return {};

        QSet<IFileModel*> seen{};
        QList<int> reverse_models{};

        for (auto i = tabCount(window) - 1; i >= 0; --i) {
            if (auto model = modelAt(window, i)) {
                if (!seen.contains(model)) {
                    seen << model;
                    reverse_models << i;
                }
            }
        }

        return reverse_models;
    }
};

} // namespace Fernanda
