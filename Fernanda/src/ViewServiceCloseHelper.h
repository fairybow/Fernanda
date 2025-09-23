/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QWidgetList>

#include "Coco/Debug.h"

#include "Bus.h"
#include "Constants.h"
#include "SavePrompt.h"
#include "TabWidget.h"
#include "Utility.h"
#include "Window.h"

namespace Fernanda {

// Handles view closing operations with save prompts, managing multi-file save
// scenarios and coordinating closes across Windows when files have multiple
// views. A closing, edited file won't prompt for a save unless all views on it
// are closing
//
// Note: May be reintegrated with ViewService later
class ViewServiceCloseHelper : public QObject
{
    Q_OBJECT

public:
    ViewServiceCloseHelper(Bus* bus, QObject* parent = nullptr)
        : QObject(parent)
        , bus_(bus)
    {
    }

    virtual ~ViewServiceCloseHelper() override { COCO_TRACER; }

    bool closeAt(Window* window, int index)
    {
        if (!window) return false;
        auto tab_widget = tabWidget(window);
        if (!tab_widget) return false;

        // I would like a standard way to handle sentinels, maybe
        auto i = (index < 0) ? tab_widget->currentIndex() : index;
        if (i < 0 || i > tab_widget->count() - 1) return false;

        // If the view's model has unsaved changes, we prompt (and stop if
        // needed) here
        auto model = modelAt(window, i);
        if (!model) return false;

        auto windows = bus_->call<QSet<Window*>>(Cmd::WindowSet);

        if (model->isModified() && !isMultiWindow(model, windows)) {
            tab_widget->setCurrentIndex(i);

            switch (SavePrompt::exec(model, window)) {
            default:
            case SaveChoice::Cancel:
                return false;
            case SaveChoice::Save: {
                auto result = bus_->call<SaveResult>(
                    Cmd::NotepadSaveFile,
                    { { "index", i } },
                    window);

                if (result != SaveResult::Success) return false;
            } break;
            case SaveChoice::Discard:
                break;
            }
        }

        auto view = tab_widget->removeTab<IFileView*>(i);
        if (!view) return false;

        emit bus_->viewClosed(view);
        delete view;

        return true;
    }

    /// WIP
    bool closeAllInWindow(Window* window)
    {
        if (!window) return false;
        auto tab_widget = tabWidget(window);
        if (!tab_widget) return false;

        // Early success for no tabs
        if (tab_widget->count() < 1) return true;

        // Map each unique modified model to its last occurrence index
        // (iterating backward)
        QHash<IFileModel*, int> model_to_index{};
        QList<IFileModel*> modified_models{};

        auto windows = bus_->call<QSet<Window*>>(Cmd::WindowSet);

        for (auto i = tab_widget->count() - 1; i >= 0; --i) {
            auto view = viewAt(window, i);
            if (!view) continue;

            auto model = view->model();
            if (model && model->isModified()
                && !isMultiWindow(model, windows)) {
                // Only store the first occurrence of each model (which is the
                // last since we're iterating backward)
                if (!model_to_index.contains(model)) {
                    model_to_index[model] = i;
                    modified_models << model;
                }
            }
        }

        if (!modified_models.isEmpty()) {
            auto result = SavePrompt::exec(modified_models, window);

            switch (result.choice) {
            default:
            case SaveChoice::Cancel:
                return false;
            case SaveChoice::Save: {
                // Convert the chosen models back to their indexes
                QList<int> indexes_to_save_choices{};

                for (auto model : result.chosenSaves)
                    if (model_to_index.contains(model))
                        indexes_to_save_choices << model_to_index[model];

                if (!indexes_to_save_choices.isEmpty()) {
                    auto result = bus_->call<SaveResult>(
                        Cmd::NotepadSaveIndexesInWindow,
                        { { "indexes", toQVariant(indexes_to_save_choices) } },
                        window);

                    if (result != SaveResult::Success) return false;
                }
            } break;
            case SaveChoice::Discard:
                break;
            }
        }

        // Close all views (how to handle all nullptr (shouldn't happen...)?)
        for (auto view : tab_widget->clear<IFileView*>()) {
            if (!view) continue;
            emit bus_->viewClosed(view);
            delete view;
        }

        return true;
    }

    bool closeAll()
    {
        for (auto& window :
             bus_->call<QList<Window*>>(Cmd::ReverseWindowList)) {
            if (!window) continue;
            if (!closeAllInWindow(window)) return false;
        }

        return true;
    }

private:
    Bus* bus_; // ViewService's
};

} // namespace Fernanda
