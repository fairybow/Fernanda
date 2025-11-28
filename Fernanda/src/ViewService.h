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
    using CanCloseTabHook = std::function<bool(IFileView*)>;
    using CanCloseTabEverywhereHook =
        std::function<bool(const QList<IFileView*>&)>;
    using CanCloseWindowTabsHook =
        std::function<bool(const QList<IFileView*>&)>;
    using CanCloseAllTabsHook = std::function<bool(const QList<IFileView*>&)>;

    ViewService(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
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

    int countFor(IFileModel* model) const
    {
        if (!model) return 0;
        return fileViewsPerModel_.value(model, 0);
    }

    void raise(IFileView* view) const
    {
        auto window = Coco::Utility::findParent<Window*>(view);
        if (!window) return;
        auto tab_widget = Coco::Utility::findParent<TabWidget*>(view);
        if (!tab_widget) return;

        window->activate();
        tab_widget->setCurrentWidget(view);
    }

    bool isMultiWindow(IFileModel* model) const
    {
        if (!model) return false;

        auto window_count = 0;
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {
                if (modelAt_(window, i) == model) {
                    ++window_count;
                    if (window_count >= 2) return true; // Early exit
                    break; // Move to next window
                }
            }
        }

        return false;
    }

    QList<IFileView*> fileViewsIn(Window* window) const
    {
        if (!window) return {};
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return {};

        QList<IFileView*> views{};

        for (auto i = tab_widget->count() - 1; i >= 0; --i)
            if (auto view = tab_widget->widgetAt<IFileView*>(i)) views << view;

        return views;
    }

    QList<IFileView*> fileViews() const
    {
        QList<IFileView*> views{};

        auto rz_windows = bus->call<QList<Window*>>(Commands::RZ_WINDOWS);
        for (auto& window : rz_windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = tab_widget->count() - 1; i >= 0; --i)
                if (auto view = tab_widget->widgetAt<IFileView*>(i))
                    views << view;
        }

        return views;
    }

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
        if (!tabWidget) return -1;
        auto i = (index < 0) ? tabWidget->currentIndex() : index;
        return (i < 0 || i >= tabWidget->count()) ? -1 : i;
    }

    // Index -1 = current
    IFileView* viewAt_(Window* window, int index) const
    {
        if (!window) return nullptr;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return nullptr;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return nullptr;

        return tab_widget->widgetAt<IFileView*>(i);
    }

    // Index -1 = current
    IFileModel* modelAt_(Window* window, int index) const
    {
        auto view = viewAt_(window, index);
        return view ? view->model() : nullptr;
    }

    void undo_(Window* window, int index = -1)
    {
        auto model = modelAt_(window, index);
        if (model && model->hasUndo()) model->undo();
    }

    void redo_(Window* window, int index = -1)
    {
        auto model = modelAt_(window, index);
        if (model && model->hasRedo()) model->redo();
    }

    void cut_(Window* window, int index = -1)
    {
        auto view = viewAt_(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->cut();
    }

    void copy_(Window* window, int index = -1)
    {
        auto view = viewAt_(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->copy();
    }

    void paste_(Window* window, int index = -1)
    {
        auto view = viewAt_(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasPaste()) view->paste();
    }

    void delete_(Window* window, int index = -1)
    {
        auto view = viewAt_(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->deleteSelection();
    }

    void selectAll_(Window* window, int index = -1)
    {
        auto view = viewAt_(window, index);
        if (!view || !view->supportsEditing()) return;
        view->selectAll();
    }

    void closeTab_(Window* window, int index = -1)
    {
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return;

        auto view = viewAt_(window, i);
        if (!view) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseTabHook_ || canCloseTabHook_(view)) {
            auto model = view->model();
            deleteFileViewAt_(window, i);
            if (model) emit bus->viewDestroyed(model);
        }
    }

    void closeTabEverywhere_(Window* window, int index = -1)
    {
        auto target_model = modelAt_(window, index);
        if (!target_model) return;

        QList<IFileView*> views{};

        auto rz_windows = bus->call<QList<Window*>>(Commands::RZ_WINDOWS);
        for (auto& window : rz_windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = tab_widget->count() - 1; i >= 0; --i)
                if (auto view = tab_widget->widgetAt<IFileView*>(i))
                    if (view->model() == target_model) views << view;
        }

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseTabEverywhereHook_ || canCloseTabEverywhereHook_(views)) {
            for (auto& window : rz_windows) {
                auto tab_widget = tabWidget_(window);
                if (!tab_widget) continue;

                // Iterate backward to avoid index shifting issues
                for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                    auto view = tab_widget->widgetAt<IFileView*>(i);
                    if (view && view->model() == target_model)
                        deleteFileViewAt_(window, i);
                }
            }

            // Technically we could emit this having destroyed no views, but I
            // doubt it. That would mean we would've had a model without a view
            // that we also then somehow managed to close by index
            emit bus->viewDestroyed(target_model);
        }
    }

    void closeWindowTabs_(Window* window)
    {
        if (!window) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        QList<IFileView*> views{};
        QSet<IFileModel*> models{};

        for (auto i = tab_widget->count() - 1; i >= 0; --i) {
            if (auto view = tab_widget->widgetAt<IFileView*>(i)) {
                views << view;
                if (auto model = view->model()) models << model;
            }
        }

        if (views.isEmpty()) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseWindowTabsHook_ || canCloseWindowTabsHook_(views)) {
            deleteAllFileViewsIn_(window);

            for (auto& model : models)
                emit bus->viewDestroyed(model);
        }
    }

    void closeAllTabs_()
    {
        QList<IFileView*> views{};
        QSet<IFileModel*> models{};
        auto rz_windows = bus->call<QList<Window*>>(Commands::RZ_WINDOWS);

        for (auto& window : rz_windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                if (auto view = tab_widget->widgetAt<IFileView*>(i)) {
                    views << view;
                    if (auto model = view->model()) models << model;
                }
            }
        }

        if (views.isEmpty()) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseAllTabsHook_ || canCloseAllTabsHook_(views)) {
            for (auto& window : rz_windows)
                deleteAllFileViewsIn_(window);

            for (auto& model : models)
                emit bus->viewDestroyed(model);
        }
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
            if (auto view = viewAt_(window, index)) active = view;

        activeFileViews_[window] = active;
        emit bus->activeFileViewChanged(window, active);
    }

    template <
        Coco::Concepts::QWidgetPointer FileViewT,
        Coco::Concepts::QObjectPointer FileModelT>
    FileViewT newFileView_(FileModelT model, QWidget* parent)
    {
        auto view = new std::remove_pointer_t<FileViewT>(model, parent);
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
            bus->execute(Commands::NEW_TAB, window);
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
    void onFileModelReadied_(Window* window, IFileModel* model)
    {
        if (!window || !model) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        IFileView* view = nullptr;

        if (auto text_model = qobject_cast<TextFileModel*>(model)) {

            auto text_view = newFileView_<TextFileView*>(text_model, window);
            /*auto font = bus->call<QFont>(
                Commands::SETTINGS_GET,
                { { "key", Ini::Editor::FONT_KEY },
                  { "default", Ini::Editor::defaultFont() } });
            text_view->setFont(font);*/
            view = text_view;

        } else if (auto no_op_model = qobject_cast<NoOpFileModel*>(model)) {
            view = newFileView_<NoOpFileView*>(no_op_model, window);
        } else {
            // TODO: UI feedback?
            WARN("Could not narrow down view type for {}!", model);
            return;
        }

        if (!view) return;

        auto meta = model->meta();
        if (!meta) {
            delete view; // Anything else?
            return;
        }

        // Only adjust this once we're clear
        ++fileViewsPerModel_[model];
        connect(view, &QObject::destroyed, this, [&, view, model] {
            if (--fileViewsPerModel_[model] <= 0)
                fileViewsPerModel_.remove(model);
        });

        auto index = tab_widget->addTab(view, meta->title());
        tab_widget->setTabFlagged(index, model->isModified());
        tab_widget->setTabToolTip(index, meta->toolTip());
        tab_widget->setCurrentIndex(index);
        view->setFocus();
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in below method, too)
    void onFileModelModificationChanged_(IFileModel* model, bool modified)
    {
        if (!model) return;

        // Find all tabs containing views on this model
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);
        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {

                auto view = tab_widget->widgetAt<IFileView*>(i);
                if (view && view->model() == model)
                    tab_widget->setTabFlagged(i, modified);
            }
        }
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in above method, too)
    void onFileModelMetaChanged_(IFileModel* model)
    {
        if (!model) return;
        auto meta = model->meta();
        if (!meta) return;

        // Find all tabs containing views of this model and update their
        // text/tooltip
        auto windows = bus->call<QSet<Window*>>(Commands::WINDOWS_SET);
        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {

                auto view = tab_widget->widgetAt<IFileView*>(i);
                if (view && view->model() == model) {
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
