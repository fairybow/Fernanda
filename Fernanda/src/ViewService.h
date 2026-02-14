/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
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
#include <QTextOption>
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
#include "Debug.h"
#include "FileMeta.h"
#include "Ini.h"
#include "KeyFilters.h"
#include "NoOpFileModel.h"
#include "NoOpFileView.h"
#include "PlainTextEdit.h"
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

    /// TODO TD
    // Insert a dragged tab into a window's TabWidget
    void insertTabSpec(Window* window, const TabWidget::TabSpec& tabSpec)
    {
        auto tab_widget = tabWidget_(window);
        if (!tab_widget || !tabSpec.isValid()) return;

        auto index = tab_widget->addTab(tabSpec.widget, tabSpec.text);
        tab_widget->setTabData(index, tabSpec.userData);
        tab_widget->setTabToolTip(index, tabSpec.toolTip);
        tab_widget->setTabFlagged(index, tabSpec.isFlagged);
        tab_widget->setCurrentIndex(index);
    }

    int countFor(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return 0;
        return fileViewsPerModel_.value(fileModel, 0);
    }

    void raise(Window* window, int index) const
    {
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
        auto windows = bus->call<QList<Window*>>(Bus::WINDOWS);
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
        auto windows = bus->call<QSet<Window*>>(Bus::WINDOWS_SET);

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {
                if (fileModelAt(window, i) == fileModel) {
                    ++window_count;
                    if (window_count >= 2) return true; // Early exit
                    break; // Move to next window
                }
            }
        }

        return false;
    }

    bool anyViews() const
    {
        auto windows = bus->call<QSet<Window*>>(Bus::WINDOWS_SET);

        for (auto& window : windows) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            if (!tab_widget->isEmpty()) return true;
        }

        return false;
    }

    // Index -1 = current
    AbstractFileView* fileViewAt(Window* window, int index) const
    {
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return nullptr;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return nullptr;

        return tab_widget->widgetAt<AbstractFileView*>(i);
    }

    // Index -1 = current
    AbstractFileModel* fileModelAt(Window* window, int index) const
    {
        auto view = fileViewAt(window, index);
        return view ? view->model() : nullptr;
    }

    // TODO: May use this for applying settings! If not, though, make private or
    // remove
    QList<AbstractFileView*> fileViewsIn(Window* window) const
    {
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return {};

        QList<AbstractFileView*> views{};

        for (auto i = 0; i < tab_widget->count(); ++i)
            if (auto view = tab_widget->widgetAt<AbstractFileView*>(i))
                views << view;

        return views;
    }

    // TODO: Check re: SoC
    // TODO: Also check, re: modifiedViewModelsIn...
    bool anyModifiedFileModelsIn(Window* window) const
    {
        if (!window) return false;

        for (auto& view : fileViewsIn(window)) {
            auto model = view->model();
            if (model && model->isModified()) return true;
        }

        return false;
    }

    // TODO: May use this for applying settings! If not, though, make private or
    // remove
    QList<AbstractFileView*> fileViews() const
    {
        auto windows = bus->call<QList<Window*>>(Bus::WINDOWS);
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

        auto windows = bus->call<QList<Window*>>(Bus::WINDOWS);
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

    void closeTab(Window* window, int index = -1)
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

    void closeTabEverywhere(Window* window, int index = -1)
    {
        auto target_model = fileModelAt(window, index);
        if (!target_model) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseTabEverywhereHook_
            || canCloseTabEverywhereHook_(window, index)) {
            auto windows = bus->call<QList<Window*>>(Bus::WINDOWS);
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

    void closeWindowTabs(Window* window)
    {
        if (!window) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseWindowTabsHook_ || canCloseWindowTabsHook_(window))
            deleteAllFileViewsIn_(window);
    }

    void closeAllTabs()
    {
        auto windows = bus->call<QList<Window*>>(Bus::WINDOWS);
        if (windows.isEmpty()) return;

        // Proceed if no hook is set, or if hook approves the close
        if (!canCloseAllTabsHook_ || canCloseAllTabsHook_(windows))
            for (auto& window : windows)
                deleteAllFileViewsIn_(window);
    }

    void undo(Window* window, int index = -1)
    {
        auto model = fileModelAt(window, index);
        if (model && model->hasUndo()) model->undo();
    }

    void redo(Window* window, int index = -1)
    {
        auto model = fileModelAt(window, index);
        if (model && model->hasRedo()) model->redo();
    }

    void cut(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->cut();
    }

    void copy(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->copy();
    }

    void paste(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasPaste()) view->paste();
    }

    void del(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        if (view->hasSelection()) view->deleteSelection();
    }

    void selectAll(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->supportsEditing()) return;
        view->selectAll();
    }

signals:
    void addTabRequested(Window* window);
    void fileViewDestroyed(AbstractFileView* fileView);

    /// TODO TD:
    void tabDragCompleted(Window* fromWindow, Window* toWindow);
    void tabDraggedToNewWindow(
        Window* sourceWindow,
        const QPoint& dropPos,
        const TabWidget::TabSpec& tabSpec);

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &ViewService::onBusWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &ViewService::onBusWindowDestroyed_);

        connect(
            bus,
            &Bus::fileModelReadied,
            this,
            &ViewService::onBusFileModelReadied_);

        connect(
            bus,
            &Bus::fileModelModificationChanged,
            this,
            &ViewService::onBusFileModelModificationChanged_);

        connect(
            bus,
            &Bus::fileModelMetaChanged,
            this,
            &ViewService::onBusFileModelMetaChanged_);

        connect(
            bus,
            &Bus::settingChanged,
            this,
            &ViewService::onBusSettingChanged_);
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

    void addTabWidget_(Window* window)
    {
        if (!window) return;

        auto tab_widget = new TabWidget(window);

        tab_widget->setElideMode(Qt::ElideRight);
        tab_widget->setDragValidator(
            this,
            &ViewService::tabWidgetDragValidator_);
        tab_widget->setTabsDraggable(true);

        window->setCentralWidget(tab_widget);

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
            [&, window](int index) { closeTab(window, index); });

        /// TODO TD
        connect(
            tab_widget,
            &TabWidget::tabDragged,
            this,
            &ViewService::onTabDragged_);

        /// TODO TD
        connect(
            tab_widget,
            &TabWidget::tabDraggedOutside,
            this,
            &ViewService::onTabDraggedOutside_);

        // connect(tab_widget, &TabWidget::tabCountChanged, this, [=] {
        //     //...
        // });
    }

    /// TODO TD
    bool tabWidgetDragValidator_(TabWidget* source, TabWidget* destination)
    {
        auto source_window = Coco::Utility::findParent<Window*>(source);
        auto target_window = Coco::Utility::findParent<Window*>(destination);

        auto our_windows = bus->call<QSet<Window*>>(Bus::WINDOWS_SET);

        auto is_valid = source_window && target_window
                        && our_windows.contains(source_window)
                        && our_windows.contains(target_window);

        if (!is_valid) INFO("Rejected tab drag between different Workspaces");

        return is_valid;
    }

    // TODO: forEach(Abstract)FileView?

    /// TODO KFS
    template <typename CallableT>
    void forEachTextFileView_(CallableT&& callable)
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto text_view = tab_widget->widgetAt<TextFileView*>(i))
                    callable(text_view);
        }
    }

    /// TODO KFS
    void applyInitialTextFileViewSettings_(TextFileView* textFileView)
    {
        if (!textFileView) return;

        if (auto editor = textFileView->editor()) {
            auto font = bus->call<QFont>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_FONT },
                  { "defaultValue", Ini::Defaults::font() } });

            /// TODO ES
            auto center_on_scroll = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_CENTER_ON_SCROLL },
                  { "defaultValue", Ini::Defaults::editorCenterOnScroll() } });

            auto overwrite = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_OVERWRITE },
                  { "defaultValue", Ini::Defaults::editorOverwrite() } });

            auto tab_stop_distance = bus->call<int>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_TAB_STOP_DISTANCE },
                  { "defaultValue", Ini::Defaults::editorTabStopDistance() } });

            auto wrap_mode = bus->call<QTextOption::WrapMode>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_WRAP_MODE },
                  { "defaultValue", Ini::Defaults::editorWrapMode() } });

            auto dbl_click_whitespace = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE },
                  { "defaultValue",
                    Ini::Defaults::editorDoubleClickWhitespace() } });

            auto line_numbers = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_LINE_NUMBERS },
                  { "defaultValue", Ini::Defaults::editorLineNumbers() } });

            auto line_highlight = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_LINE_HIGHLIGHT },
                  { "defaultValue", Ini::Defaults::editorLineHighlight() } });

            auto selection_handles = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::EDITOR_SELECTION_HANDLES },
                  { "defaultValue",
                    Ini::Defaults::editorSelectionHandles() } });

            editor->setFont(font);
            editor->setCenterOnScroll(center_on_scroll);
            editor->setOverwriteMode(overwrite);
            editor->setTabStopDistance(tab_stop_distance);
            editor->setWordWrapMode(wrap_mode);
            editor->setDoubleClickWhitespace(dbl_click_whitespace);
            editor->setLineNumbers(line_numbers);
            editor->setLineHighlight(line_highlight);
            editor->setSelectionHandles(selection_handles);
        }

        /// TODO KFS
        if (auto key_filters = textFileView->keyFilters()) {
            auto active = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::KEY_FILTERS_ACTIVE },
                  { "defaultValue", Ini::Defaults::keyFiltersActive() } });

            auto auto_close = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::KEY_FILTERS_AUTO_CLOSE },
                  { "defaultValue", Ini::Defaults::keyFiltersAutoClose() } });

            auto barging = bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", Ini::Keys::KEY_FILTERS_BARGING },
                  { "defaultValue", Ini::Defaults::keyFiltersBarging() } });

            key_filters->setActive(active);
            key_filters->setAutoClosing(auto_close);
            key_filters->setBarging(barging);
        }
    }

private slots:
    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;
        addTabWidget_(window);
    }

    void onBusWindowDestroyed_(Window* window)
    {
        if (!window) return;
        activeFileViews_.remove(window);
    }

    // TODO: New view settings
    void onBusFileModelReadied_(Window* window, AbstractFileModel* fileModel)
    {
        if (!window || !fileModel) return;
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        AbstractFileView* view = nullptr;

        if (auto text_model = qobject_cast<TextFileModel*>(fileModel)) {
            auto text_view = newFileView_<TextFileView*>(text_model, window);
            applyInitialTextFileViewSettings_(text_view);
            view = text_view;

        } else if (auto no_op_model = qobject_cast<NoOpFileModel*>(fileModel)) {
            view = newFileView_<NoOpFileView*>(no_op_model, window);

        } else {
            FATAL("Type not deduced for model [{}]!", fileModel);
        }

        if (!view) return;

        auto meta = fileModel->meta();
        if (!meta) {
            delete view; // Anything else?
            return;
        }

        // Only adjust this once we're clear
        ++fileViewsPerModel_[fileModel];

        emit bus->fileViewCreated(view);

        connect(view, &QObject::destroyed, this, [&, view, fileModel] {
            if (--fileViewsPerModel_[fileModel] <= 0)
                fileViewsPerModel_.remove(fileModel);
            INFO("File view destroyed [{}] for model [{}]", view, fileModel);
            emit fileViewDestroyed(view);
        });

        auto index = tab_widget->addTab(view, meta->title());
        tab_widget->setTabFlagged(index, fileModel->isModified());
        tab_widget->setTabToolTip(index, meta->toolTip());
        tab_widget->setCurrentIndex(index);
        view->setFocus();
    }

    // TODO: Separate method with callback for iteration over all tabs-per-model
    // (use in below method, too)
    void onBusFileModelModificationChanged_(
        AbstractFileModel* fileModel,
        bool modified)
    {
        if (!fileModel) return;

        // Find all tabs containing views on this model
        auto windows = bus->call<QSet<Window*>>(Bus::WINDOWS_SET);
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
    void onBusFileModelMetaChanged_(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;
        auto meta = fileModel->meta();
        if (!meta) return;

        // Find all tabs containing views of this model and update their
        // text/tooltip
        auto windows = bus->call<QSet<Window*>>(Bus::WINDOWS_SET);
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

    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        if (key == Ini::Keys::EDITOR_FONT) {
            auto v = value.value<QFont>();
            forEachTextFileView_(
                [&](TextFileView* view) { view->editor()->setFont(v); });
        }

        /// TODO KFS
        if (key == Ini::Keys::KEY_FILTERS_ACTIVE) {
            auto v = value.value<bool>();
            forEachTextFileView_(
                [&](TextFileView* view) { view->keyFilters()->setActive(v); });
        }

        /// TODO KFS
        if (key == Ini::Keys::KEY_FILTERS_AUTO_CLOSE) {
            auto v = value.value<bool>();
            forEachTextFileView_([&](TextFileView* view) {
                view->keyFilters()->setAutoClosing(v);
            });
        }

        /// TODO KFS
        if (key == Ini::Keys::KEY_FILTERS_BARGING) {
            auto v = value.value<bool>();
            forEachTextFileView_(
                [&](TextFileView* view) { view->keyFilters()->setBarging(v); });
        }

        /// TODO ES
        if (key == Ini::Keys::EDITOR_CENTER_ON_SCROLL) {
            auto v = value.value<bool>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setCenterOnScroll(v);
            });
        }

        /// TODO ES
        if (key == Ini::Keys::EDITOR_OVERWRITE) {
            auto overwrite = value.value<bool>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setOverwriteMode(overwrite);
            });
        }

        /// TODO ES
        if (key == Ini::Keys::EDITOR_TAB_STOP_DISTANCE) {
            auto v = value.value<int>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setTabStopDistance(v);
            });
        }

        /// TODO ES
        if (key == Ini::Keys::EDITOR_WRAP_MODE) {
            auto v = value.value<QTextOption::WrapMode>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setWordWrapMode(v);
            });
        }

        if (key == Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE) {
            auto v = value.value<bool>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setDoubleClickWhitespace(v);
            });
        }

        if (key == Ini::Keys::EDITOR_LINE_NUMBERS) {
            auto v = value.value<bool>();
            forEachTextFileView_(
                [&](TextFileView* view) { view->editor()->setLineNumbers(v); });
        }

        if (key == Ini::Keys::EDITOR_LINE_HIGHLIGHT) {
            auto v = value.value<bool>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setLineHighlight(v);
            });
        }

        if (key == Ini::Keys::EDITOR_SELECTION_HANDLES) {
            auto v = value.value<bool>();
            forEachTextFileView_([&](TextFileView* view) {
                view->editor()->setSelectionHandles(v);
            });
        }
    }

    /// TODO TD
    void onTabDragged_(
        const TabWidget::Location& old,
        const TabWidget::Location& now)
    {
        if (!old.isValid() || !now.isValid()) return;

        auto old_window = Coco::Utility::findParent<Window*>(old.tabWidget);
        auto new_window = Coco::Utility::findParent<Window*>(now.tabWidget);

        if (!old_window || !new_window) return;

        // Same-window reorder needs no further handling
        if (old_window == new_window) return;

        new_window->activate();

        if (auto view = fileViewAt(new_window, now.index)) view->setFocus();

        emit tabDragCompleted(old_window, new_window);
    }

    /// TODO TD
    void onTabDraggedOutside_(
        TabWidget* source,
        const QPoint& dropPos,
        const TabWidget::TabSpec& tabSpec)
    {
        if (!source || !tabSpec.isValid()) return;

        auto source_window = Coco::Utility::findParent<Window*>(source);
        emit tabDraggedToNewWindow(source_window, dropPos, tabSpec);
    }
};

} // namespace Fernanda
