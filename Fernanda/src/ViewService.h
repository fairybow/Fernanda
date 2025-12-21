/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>
#include <type_traits>

#include <QFont>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

#include "Coco/Bool.h"
#include "Coco/Concepts.h"
#include "Coco/Utility.h"

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "AbstractService.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "FileMeta.h"
#include "Ini.h"
#include "NoOpFileModel.h"
#include "NoOpFileView.h"
#include "TabWidget.h"
#include "TextFileModel.h"
#include "TextFileView.h"
#include "Window.h"

namespace Fernanda {

// Creates and manages program views (TabWidgets and FileViews) within
// Windows, routes editing commands, handles view lifecycles, propagates
// TabWidget signals, and tracks the number of views per model
class ViewService : public AbstractService
{
    Q_OBJECT

public:
    using CanCloseTabHook = std::function<bool(Window*, int index)>;
    using CanCloseTabEverywhereHook = std::function<bool(Window*, int index)>;
    using CanCloseWindowTabsHook = std::function<bool(Window*)>;
    using CanCloseAllTabsHook = std::function<bool(const QList<Window*>&)>;

    ViewService(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~ViewService() override { TRACER; }

    DECLARE_HOOK_ACCESSORS(
        CanCloseTabHook,
        canCloseTabHook,
        setCanCloseTabHook,
        canCloseTabHook_);

    DECLARE_HOOK_ACCESSORS(
        CanCloseTabEverywhereHook,
        canCloseTabEverywhereHook,
        setCanCloseTabEverywhereHook,
        canCloseTabEverywhereHook_);

    DECLARE_HOOK_ACCESSORS(
        CanCloseWindowTabsHook,
        canCloseWindowTabsHook,
        setCanCloseWindowTabsHook,
        canCloseWindowTabsHook_);

    DECLARE_HOOK_ACCESSORS(
        CanCloseAllTabsHook,
        canCloseAllTabsHook,
        setCanCloseAllTabsHook,
        canCloseAllTabsHook_);

    int countFor(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return 0;
        return fileViewsPerModel_.value(fileModel, 0);
    }

    void raise(Window* window, int index) const
    {
        if (!window) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        window->activate();
        tab_widget->setCurrentIndex(index);
    }

    void raise(Window* window, AbstractFileModel* model) const
    {
        if (!window || !model) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        // Find first tab from the left with this model
        for (auto i = 0; i < tab_widget->count(); ++i) {
            auto view = tab_widget->widgetAt<AbstractFileView*>(i);
            if (!view) continue;
            if (view->model() != model) continue;

            raise(window, i);
            return;
        }
    }

    // Returns the first window found (from top to bottom) with this model (if
    // any)
    Window* raise(AbstractFileModel* model) const
    {
        if (!model) return nullptr;
        auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
        if (windows.isEmpty()) return nullptr;

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            // Find first tab from the left with this model
            for (auto i = 0; i < tab_widget->count(); ++i) {
                auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                if (!view) continue;
                if (view->model() != model) continue;

                raise(window, i);
                return window;
            }
        }

        return nullptr;
    }

    bool isMultiWindow(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return false;

        auto window_count = 0;
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {
                if (fileModelAt_(window, i) == fileModel) {
                    ++window_count;
                    if (window_count >= 2) return true; // Early exit
                    break; // Move to next window
                }
            }
        }

        return false;
    }

    // Index -1 = current
    AbstractFileView* fileViewAt(Window* window, int index) const
    {
        if (!window) return nullptr;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return nullptr;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return nullptr;

        return tab_widget->widgetAt<AbstractFileView*>(i);
    }

    // TODO: May use this for applying settings! If not, though, make private or
    // remove
    QList<AbstractFileView*> fileViewsIn(Window* window) const
    {
        if (!window) return {};
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return {};

        QList<AbstractFileView*> views{};

        for (auto i = 0; i < tab_widget->count(); ++i)
            if (auto view = tab_widget->widgetAt<AbstractFileView*>(i))
                views << view;

        return views;
    }

    // TODO: May use this for applying settings! If not, though, make private or
    // remove
    QList<AbstractFileView*> fileViews() const
    {
        auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
        if (windows.isEmpty()) return {};

        QList<AbstractFileView*> views{};

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto view = tab_widget->widgetAt<AbstractFileView*>(i))
                    views << view;
        }

        return views;
    }

    COCO_BOOL(ExcludeMultiWindow);

    QList<AbstractFileModel*> modifiedViewModelsIn(
        Window* window,
        ExcludeMultiWindow excludeMultiWindow = ExcludeMultiWindow::No) const
    {
        if (!window) return {};

        // We're using a list here and going by view so it's consistent with UI
        // order
        QList<AbstractFileModel*> result{};

        for (auto view : fileViewsIn(window)) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (excludeMultiWindow && isMultiWindow(model)) continue;
            if (result.contains(model)) continue;

            result << model;
        }

        return result;
    }

    QList<AbstractFileModel*> modifiedViewModels() const
    {
        QList<AbstractFileModel*> result{};

        for (auto view : fileViews()) {
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (result.contains(model)) continue;

            result << model;
        }

        return result;
    }

    // No hook!
    void closeViewsForModels(const QSet<AbstractFileModel*>& fileModels)
    {
        if (fileModels.isEmpty()) return;

        auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
        if (windows.isEmpty()) return;

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget || tab_widget->isEmpty()) continue;

            // Iterate backward to avoid index shifting issues
            for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                if (view && fileModels.contains(view->model()))
                    deleteFileViewAt_(window, i);
            }
        }
    }

signals:
    void viewDestroyed(AbstractFileModel* fileModel);
    void addTabRequested(Window* window);

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(Commands::CLOSE_TAB, [&](const Command& cmd) {
            closeTab_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(
            Commands::CLOSE_TAB_EVERYWHERE,
            [&](const Command& cmd) {
                closeTabEverywhere_(cmd.context, cmd.param<int>("index", -1));
            });

        bus->addCommandHandler(
            Commands::CLOSE_WINDOW_TABS,
            [&](const Command& cmd) { closeWindowTabs_(cmd.context); });

        bus->addCommandHandler(
            Commands::CLOSE_ALL_TABS,
            [&](const Command& cmd) { closeAllTabs_(); });

        // All the methods used in these handlers aren't called anywhere else
        // right now except in menus. As such, they don't really need the index
        // parameter (as they only ever operate on the current view in a given
        // window/cmd.context. However, leaving it as-is for now just in case.

        bus->addCommandHandler(Commands::UNDO, [&](const Command& cmd) {
            undo_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(Commands::REDO, [&](const Command& cmd) {
            redo_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(Commands::CUT, [&](const Command& cmd) {
            cut_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(Commands::COPY, [&](const Command& cmd) {
            copy_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(Commands::PASTE, [&](const Command& cmd) {
            paste_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(Commands::DEL, [&](const Command& cmd) {
            delete_(cmd.context, cmd.param<int>("index", -1));
        });

        bus->addCommandHandler(Commands::SELECT_ALL, [&](const Command& cmd) {
            selectAll_(cmd.context, cmd.param<int>("index", -1));
        });
    }

    virtual void connectBusEvents() override
    {
        connect(bus, &Bus::windowCreated, this, &ViewService::onWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &ViewService::onWindowDestroyed_);

        connect(
            bus,
            &Bus::fileModelReadied,
            this,
            &ViewService::onFileModelReadied_);

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            &ViewService::onFileModelModificationChanged_);

        connect(
            bus,
            &Bus::fileModelMetaChanged,
            this,
            &ViewService::onFileModelMetaChanged_);
    }

private:
    QHash<Window*, AbstractFileView*> activeFileViews_{};
    QHash<AbstractFileModel*, int> fileViewsPerModel_{};
    CanCloseTabHook canCloseTabHook_ = nullptr;
    CanCloseTabEverywhereHook canCloseTabEverywhereHook_ = nullptr;
    CanCloseWindowTabsHook canCloseWindowTabsHook_ = nullptr;
    CanCloseAllTabsHook canCloseAllTabsHook_ = nullptr;

    void setup_()
    {
        //...
    }

    TabWidget* tabWidget_(Window* window) const
    {
        if (!window) return nullptr;
        return qobject_cast<TabWidget*>(window->centralWidget());
    }

    // If index is -1, it will become current index
    int normalizeIndex_(TabWidget* tabWidget, int index) const
    {
        if (!tabWidget || tabWidget->isEmpty()) return -1;
        auto i = (index < 0) ? tabWidget->currentIndex() : index;
        return (i < 0 || i >= tabWidget->count()) ? -1 : i;
    }

    // Index -1 = current
    AbstractFileModel* fileModelAt_(Window* window, int index) const
    {
        auto view = fileViewAt(window, index);
        return view ? view->model() : nullptr;
    }

    void closeTab_(Window* window, int index = -1)
    {
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;
        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return;
        // Check for view here; if the hook approves but the view is somehow
        // null, we silently return without emitting viewDestroyed
        auto view = fileViewAt(window, i);
        if (!view) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseTabHook_ || canCloseTabHook_(window, i))
            deleteFileViewAt_(window, i);
    }

    void closeTabEverywhere_(Window* window, int index = -1)
    {
        auto target_model = fileModelAt_(window, index);
        if (!target_model) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseTabEverywhereHook_
            || canCloseTabEverywhereHook_(window, index)) {
            auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
            if (windows.isEmpty()) return;

            for (auto& window : windows) {
                auto tab_widget = tabWidget_(window);
                if (!tab_widget || tab_widget->isEmpty()) continue;

                // Iterate backward to avoid index shifting issues
                for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                    auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                    if (view && view->model() == target_model)
                        deleteFileViewAt_(window, i);
                }
            }
        }
    }

    void closeWindowTabs_(Window* window)
    {
        if (!window) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseWindowTabsHook_ || canCloseWindowTabsHook_(window))
            deleteAllFileViewsIn_(window);
    }

    void closeAllTabs_()
    {
        auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
        if (windows.isEmpty()) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseAllTabsHook_ || canCloseAllTabsHook_(windows))
            for (auto& window : windows)
                deleteAllFileViewsIn_(window);
    }

    void undo_(Window* window, int index = -1)
    {
        auto model = fileModelAt_(window, index);
        if (model && model->hasUndo()) model->undo();
    }

    void redo_(Window* window, int index = -1)
    {
        auto model = fileModelAt_(window, index);
        if (model && model->hasRedo()) model->redo();
    }

    void cut_(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->cut();
    }

    void copy_(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->copy();
    }

    void paste_(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasPaste()) view->paste();
    }

    void delete_(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->deleteSelection();
    }

    void selectAll_(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        view->selectAll();
    }

    // Index -1 = current
    void deleteFileViewAt_(Window* window, int index)
    {
        if (!window) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return;

        auto view = tab_widget->removeTab<AbstractFileView*>(i);
        if (!view) return;

        delete view;
    }

    void deleteAllFileViewsIn_(Window* window)
    {
        if (!window) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        auto views = tab_widget->clear<AbstractFileView*>();
        if (views.isEmpty()) return;

        for (auto& view : views)
            delete view;
    }

    // Active file view can be set nullptr!
    void setActiveFileView_(Window* window, int index)
    {
        if (!window) return;

        AbstractFileView* active = nullptr;

        if (index > -1)
            if (auto view = fileViewAt(window, index)) active = view;

        activeFileViews_[window] = active;
        INFO("Active file view changed in [{}] to [{}]", window, active);
    }

    template <
        Coco::Concepts::QWidgetPointer FileViewT,
        Coco::Concepts::QObjectPointer FileModelT>
    FileViewT newFileView_(FileModelT fileModel, QWidget* parent)
    {
        auto view = new std::remove_pointer_t<FileViewT>(fileModel, parent);
        view->initialize();
        return view;
    }

    // TODO: Set drag validator
    void addTabWidget_(Window* window)
    {
        if (!window) return;

        auto tab_widget = new TabWidget(window);
        window->setCentralWidget(tab_widget);

        // tab_widget->setDragValidator(this,
        // &ViewService::tabWidgetDragValidator_);

        connect(
            tab_widget,
            &TabWidget::currentChanged,
            this,
            [&, window](int index) { setActiveFileView_(window, index); });

        connect(tab_widget, &TabWidget::addTabRequested, this, [&, window] {
            emit addTabRequested(window);
        });

        connect(
            tab_widget,
            &TabWidget::closeTabRequested,
            this,
            [&, window](int index) { closeTab_(window, index); });

        // connect(tab_widget, &TabWidget::tabCountChanged, this, [=] {
        //     //...
        // });
        // connect(tab_widget, &TabWidget::tabDragged, this, [] {
        //     //...
        // });
        // connect(tab_widget, &TabWidget::tabDraggedToDesktop, this, [] {
        //     //...
        // });
    }

private slots:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addTabWidget_(window);
    }

    void onWindowDestroyed_(Window* window)
    {
        if (!window) return;
        activeFileViews_.remove(window);
    }

    // TODO: New view settings
    void onFileModelReadied_(Window* window, AbstractFileModel* fileModel)
    {
        if (!window || !fileModel) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        AbstractFileView* view = nullptr;

        if (auto text_model = qobject_cast<TextFileModel*>(fileModel)) {

            auto text_view = newFileView_<TextFileView*>(text_model, window);
            /*auto font = bus->call<QFont>(
                Commands::SETTINGS_GET,
                { { "key", Ini::Editor::FONT_KEY },
                  { "default", Ini::Editor::defaultFont() } });
            text_view->setFont(font);*/
            view = text_view;

        } else if (auto no_op_model = qobject_cast<NoOpFileModel*>(fileModel)) {
            view = newFileView_<NoOpFileView*>(no_op_model, window);
        } else {
            // TODO: UI feedback?
            WARN("Could not narrow down view type for {}!", fileModel);
            return;
        }

        if (!view) return;

        auto meta = fileModel->meta();
        if (!meta) {
            delete view; // Anything else?
            return;
        }

        // Only adjust this once we're clear
        ++fileViewsPerModel_[fileModel];
        connect(view, &QObject::destroyed, this, [&, view, fileModel] {
            if (--fileViewsPerModel_[fileModel] <= 0)
                fileViewsPerModel_.remove(fileModel);

            INFO("File view destroyed for model [{}]", fileModel);
            emit viewDestroyed(fileModel);
        });

        auto index = tab_widget->addTab(view, meta->title());
        tab_widget->setTabFlagged(index, fileModel->isModified());
        tab_widget->setTabToolTip(index, meta->toolTip());
        tab_widget->setCurrentIndex(index);
        view->setFocus();
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in below method, too)
    void
    onFileModelModificationChanged_(AbstractFileModel* fileModel, bool modified)
    {
        if (!fileModel) return;

        // Find all tabs containing views on this model
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);
        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {

                auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                if (view && view->model() == fileModel)
                    tab_widget->setTabFlagged(i, modified);
            }
        }
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in above method, too)
    void onFileModelMetaChanged_(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;
        auto meta = fileModel->meta();
        if (!meta) return;

        // Find all tabs containing views of this model and update their
        // text/tooltip
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);
        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {

                auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                if (view && view->model() == fileModel) {
                    tab_widget->setTabText(i, meta->title());
                    tab_widget->setTabToolTip(i, meta->toolTip());
                }
            }
        }
    }

    // TODO: Implement
    void onSettingChanged_(const QString& key, const QVariant& value)
    {
        // Gotta handle multiple for editor stuff
        /*if (key != Ini::Editor::FONT_KEY) return;

        auto font = to<QFont>(value);

        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);
        for (auto& window : windows)
        { auto tab_widget = Util::tabWidget(window); if (!tab_widget)
        continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto text_view =
        tab_widget->widgetAt<TextFileView*>(i)) text_view->setFont(font);
        }*/
    }
};

} // namespace Fernanda
