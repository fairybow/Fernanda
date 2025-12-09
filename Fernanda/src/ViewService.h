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

#include "Coco/Concepts.h"
#include "Coco/Utility.h"

#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "FileMeta.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "IService.h"
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
class ViewService : public IService
{
    Q_OBJECT

public:
    // TODO: This is probably more a signal, since no return value
    using NewTabHook = std::function<void(Window*)>;

    using CanCloseTabHook = std::function<bool(Window*, int)>;
    using CanCloseTabEverywhereHook = std::function<bool(Window*, int)>;
    using CanCloseWindowTabsHook = std::function<bool(Window*)>;
    using CanCloseAllTabsHook = std::function<bool(const QList<Window*>&)>;

    ViewService(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        setup_();
    }

    virtual ~ViewService() override { TRACER; }

    DECLARE_HOOK_ACCESSORS(NewTabHook, newTabHook, setNewTabHook, newTabHook_);

    // TODO: Could pass window in hooks...

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

    int countFor(IFileModel* fileModel) const
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

    /// TODO SAVES

    void raise(Window* window, IFileModel* model) const
    {
        if (!window || !model) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        // Find first tab from the left with this model
        for (auto i = 0; i < tab_widget->count(); ++i) {
            auto view = tab_widget->widgetAt<IFileView*>(i);
            if (!view) continue;
            if (view->model() != model) continue;

            raise(window, i);
            return;
        }
    }

    // Returns the first window found (from top to bottom) with this model (if
    // any)
    Window* raise(IFileModel* model) const
    {
        if (!model) return nullptr;
        auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
        if (windows.isEmpty()) return nullptr;

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            // Find first tab from the left with this model
            for (auto i = 0; i < tab_widget->count(); ++i) {
                auto view = tab_widget->widgetAt<IFileView*>(i);
                if (!view) continue;
                if (view->model() != model) continue;

                raise(window, i);
                return window;
            }
        }

        return nullptr;
    }

    bool isMultiWindow(IFileModel* fileModel) const
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
    IFileView* fileViewAt(Window* window, int index) const
    {
        if (!window) return nullptr;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return nullptr;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return nullptr;

        return tab_widget->widgetAt<IFileView*>(i);
    }

    QList<IFileView*> fileViewsIn(Window* window) const
    {
        if (!window) return {};
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return {};

        QList<IFileView*> views{};

        for (auto i = 0; i < tab_widget->count(); ++i)
            if (auto view = tab_widget->widgetAt<IFileView*>(i)) views << view;

        return views;
    }

    QList<IFileView*> fileViews() const
    {
        auto windows = bus->call<QList<Window*>>(Commands::WINDOWS);
        if (windows.isEmpty()) return {};

        QList<IFileView*> views{};

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto view = tab_widget->widgetAt<IFileView*>(i))
                    views << view;
        }

        return views;
    }

    /// TODO SAVES (END)

protected:
    virtual void registerBusCommands() override
    {
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

        bus->addCommandHandler(Commands::NEW_TAB, [&](const Command& cmd) {
            newTab_(cmd.context);
        });

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
    QHash<Window*, IFileView*> activeFileViews_{};
    QHash<IFileModel*, int> fileViewsPerModel_{};
    NewTabHook newTabHook_ = nullptr;
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
    IFileModel* fileModelAt_(Window* window, int index) const
    {
        auto view = fileViewAt(window, index);
        return view ? view->model() : nullptr;
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

    void newTab_(Window* window)
    {
        if (!window || !newTabHook_) return;
        newTabHook_(window);
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
                    auto view = tab_widget->widgetAt<IFileView*>(i);
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

    // Index -1 = current
    void deleteFileViewAt_(Window* window, int index)
    {
        if (!window) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return;

        auto view = tab_widget->removeTab<IFileView*>(i);
        if (!view) return;

        delete view;
    }

    void deleteAllFileViewsIn_(Window* window)
    {
        if (!window) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        auto views = tab_widget->clear<IFileView*>();
        if (views.isEmpty()) return;

        for (auto& view : views)
            delete view;
    }

    // Active file view can be set nullptr!
    void setActiveFileView_(Window* window, int index)
    {
        if (!window) return;

        IFileView* active = nullptr;

        if (index > -1)
            if (auto view = fileViewAt(window, index)) active = view;

        activeFileViews_[window] = active;
        emit bus->activeFileViewChanged(window, active);
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
            newTab_(window);
        });

        connect(
            tab_widget,
            &TabWidget::closeTabRequested,
            this,
            [&, window](int index) { closeTab_(window, index); });

        connect(tab_widget, &TabWidget::tabCountChanged, this, [=] {
            // emit bus->windowTabCountChanged(window, tab_widget->count());
        });

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
    void onFileModelReadied_(Window* window, IFileModel* fileModel)
    {
        if (!window || !fileModel) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        IFileView* view = nullptr;

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

            emit bus->viewDestroyed(fileModel);
        });

        auto index = tab_widget->addTab(view, meta->title());
        tab_widget->setTabFlagged(index, fileModel->isModified());
        tab_widget->setTabToolTip(index, meta->toolTip());
        tab_widget->setCurrentIndex(index);
        view->setFocus();
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in below method, too)
    void onFileModelModificationChanged_(IFileModel* fileModel, bool modified)
    {
        if (!fileModel) return;

        // Find all tabs containing views on this model
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);
        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {

                auto view = tab_widget->widgetAt<IFileView*>(i);
                if (view && view->model() == fileModel)
                    tab_widget->setTabFlagged(i, modified);
            }
        }
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in above method, too)
    void onFileModelMetaChanged_(IFileModel* fileModel)
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

                auto view = tab_widget->widgetAt<IFileView*>(i);
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
