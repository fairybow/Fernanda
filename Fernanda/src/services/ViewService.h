/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include <QFont>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTextOption>
#include <QVariant>
#include <QWidget>

#include <Coco/Bool.h>
#include <Coco/Concepts.h>
#include <Coco/Utility.h>

#include "core/Debug.h"
#include "core/Files.h"
#include "models/AbstractFileModel.h"
#include "models/FileMeta.h"
#include "models/PdfFileModel.h"
#include "models/RawFileModel.h"
#include "models/TextFileModel.h"
#include "services/AbstractService.h"
#include "services/ReloadPrompt.h"
#include "settings/Ini.h"
#include "ui/PlainTextEdit.h"
#include "ui/TabSurface.h"
#include "ui/TabWidget.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "views/FountainFileView.h"
#include "views/HtmlFileView.h"
#include "views/ImageFileView.h"
#include "views/KeyFilters.h"
#include "views/MarkdownFileView.h"
#include "views/PdfFileView.h"
#include "views/TextFileView.h"
#include "workspaces/Bus.h"

namespace Fernanda {

// Creates and manages program views (TabWidgets and FileViews) within
// Windows, routes editing commands, handles view lifecycles, propagates
// TabWidget signals, and tracks the number of views per model
class ViewService : public AbstractService
{
    Q_OBJECT

public:
    ViewService(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~ViewService() override { TRACER; }

    /// TODO TS
    DECLARE_HOOK(
        std::function<bool(Window*, AbstractFileModel*)>,
        canCloseTabHook,
        setCanCloseTabHook)

    /// TODO TS
    DECLARE_HOOK(
        std::function<bool(Window*, AbstractFileModel*)>,
        canCloseTabEverywhereHook,
        setCanCloseTabEverywhereHook)

    /// TODO TS
    DECLARE_HOOK(
        std::function<bool(Window*)>,
        canCloseSplitHook,
        setCanCloseSplitHook)

    DECLARE_HOOK(
        std::function<bool(Window*)>,
        canCloseWindowTabsHook,
        setCanCloseWindowTabsHook)

    DECLARE_HOOK(
        std::function<bool(const QList<Window*>&)>,
        canCloseAllTabsHook,
        setCanCloseAllTabsHook)

    /// TODO TD
    // Insert a dragged tab into a window's TabWidget
    void insertTabSpec(Window* window, const TabWidget::TabSpec& tabSpec)
    {
        auto tab_widget = activeTabWidget_(window);
        if (!tab_widget || !tabSpec.isValid()) return;

        auto index = tab_widget->addTab(tabSpec);
        tab_widget->setCurrentIndex(index);
    }

    int countFor(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return 0;
        return fileViewsPerModel_.value(fileModel, 0);
    }

    // Raises Window and tab at index
    void raise(Window* window, int index) const
    {
        auto tab_widget = activeTabWidget_(window);
        if (!tab_widget) return;

        window->activate();
        tab_widget->setCurrentIndex(index);
    }

    // Raises Window and model, if found
    /// TODO TS
    void raise(Window* window, AbstractFileModel* model) const
    {
        if (!window || !model) return;

        for (auto tab_widget : tabWidgets_(window)) {
            auto i = indexOfModel_(tab_widget, model);
            if (i >= 0) {
                window->activate();
                tab_widget->setCurrentIndex(i);
                return;
            }
        }
    }

    // Raises first Window found (from top to bottom) containing model (and
    // raises model), if any, and returns the window
    // TODO: Should this return nullptr if the model isn't found?
    /// TODO TS
    Window* raise(AbstractFileModel* model) const
    {
        if (!model) return nullptr;

        for (auto& window : bus->call<QList<Window*>>(Bus::WINDOWS)) {
            for (auto tab_widget : tabWidgets_(window)) {
                auto i = indexOfModel_(tab_widget, model);
                if (i >= 0) {
                    window->activate();
                    tab_widget->setCurrentIndex(i);
                    return window;
                }
            }
        }

        return nullptr;
    }

    /// TODO TS
    bool anyViews() const
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            for (auto tab_widget : tabWidgets_(window)) {
                if (!tab_widget->isEmpty()) return true;
            }
        }

        return false;
    }

    // Index -1 = current
    AbstractFileView* fileViewAt(Window* window, int index) const
    {
        auto tab_widget = activeTabWidget_(window);
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
    /// TODO TS
    QList<AbstractFileView*> fileViewsIn(Window* window) const
    {
        QList<AbstractFileView*> views{};

        for (auto tab_widget : tabWidgets_(window)) {
            for (auto i = 0; i < tab_widget->count(); ++i) {
                if (auto view = tab_widget->widgetAt<AbstractFileView*>(i)) {
                    views << view;
                }
            }
        }

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
    /// TODO TS
    QList<AbstractFileView*> fileViews() const
    {
        QList<AbstractFileView*> views{};

        for (auto& window : bus->call<QList<Window*>>(Bus::WINDOWS)) {
            for (auto tab_widget : tabWidgets_(window)) {
                for (auto i = 0; i < tab_widget->count(); ++i) {
                    if (auto view =
                            tab_widget->widgetAt<AbstractFileView*>(i)) {
                        views << view;
                    }
                }
            }
        }

        return views;
    }

    COCO_BOOL(ExcludeMultiWindow)

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
            if (excludeMultiWindow && isMultiWindow_(model)) continue;
            if (result.contains(model)) continue;

            result << model;
        }

        return result;
    }

    /// TODO TS
    // Modified models in the active split that would be lost if the split
    // were closed (i.e., no other views exist elsewhere)
    QList<AbstractFileModel*>
    modifiedViewModelsInActiveSplit(Window* window) const
    {
        auto tab_widget = activeTabWidget_(window);
        if (!tab_widget) return {};

        QList<AbstractFileModel*> result{};

        for (auto i = 0; i < tab_widget->count(); ++i) {
            auto view = tab_widget->widgetAt<AbstractFileView*>(i);
            if (!view) continue;
            auto model = view->model();
            if (!model || !model->isModified()) continue;
            if (result.contains(model)) continue;

            // Count how many views of this model are in this split
            auto count_in_split = 0;
            for (auto j = 0; j < tab_widget->count(); ++j) {
                auto v = tab_widget->widgetAt<AbstractFileView*>(j);
                if (v && v->model() == model) ++count_in_split;
            }

            // Model survives if it has views outside this split
            if (countFor(model) > count_in_split) continue;

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

    void duplicateTab(Window* window, int index = -1)
    {
        auto model = fileModelAt(window, index);
        if (!model) return;
        auto tab_widget = activeTabWidget_(window);
        if (!tab_widget) return;

        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return;

        auto view = createFileView_(window, model);
        if (!view) return;

        auto meta = model->meta();
        auto insert_at = i + 1;
        auto new_index = tab_widget->insertTab(insert_at, view, meta->title());
        tab_widget->setTabFlagged(new_index, model->isModified());
        tab_widget->setTabToolTip(new_index, meta->toolTip());
        tab_widget->setCurrentIndex(new_index);
        view->setFocus();
    }

    // No hook!
    /// TODO TS
    void closeViewsForModels(const QSet<AbstractFileModel*>& fileModels)
    {
        if (fileModels.isEmpty()) return;

        for (auto& window : bus->call<QList<Window*>>(Bus::WINDOWS)) {
            for (auto tab_widget : tabWidgets_(window)) {
                for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                    auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                    if (view && fileModels.contains(view->model()))
                        deleteFileViewAt_(tab_widget, i);
                }
            }
        }
    }

    /// TODO TS
    void closeTab(Window* window, int index = -1)
    {
        auto tab_widget = activeTabWidget_(window);
        if (!tab_widget) return;
        auto i = normalizeIndex_(tab_widget, index);
        if (i < 0) return;
        closeTabIn_(tab_widget, i);
    }

    /// TODO TS
    void closeTabEverywhere(Window* window, int index = -1)
    {
        auto target_model = fileModelAt(window, index);
        if (!target_model) return;

        if (!canCloseTabEverywhereHook_
            || canCloseTabEverywhereHook_(window, target_model)) {

            for (auto& w : bus->call<QList<Window*>>(Bus::WINDOWS)) {
                for (auto tab_widget : tabWidgets_(w)) {
                    for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                        auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                        if (view && view->model() == target_model) {
                            deleteFileViewAt_(tab_widget, i);
                        }
                    }
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

    /// TODO TS
    void splitLeft(Window* window, int index = -1)
    {
        moveToSplit_(window, SplitDirection_::Left, index);
    }

    /// TODO TS
    void splitRight(Window* window, int index = -1)
    {
        moveToSplit_(window, SplitDirection_::Right, index);
    }

    /// TODO TS
    void duplicateToSplitLeft(Window* window, int index = -1)
    {
        duplicateToSplit_(window, SplitDirection_::Left, index);
    }

    /// TODO TS
    void duplicateToSplitRight(Window* window, int index = -1)
    {
        duplicateToSplit_(window, SplitDirection_::Right, index);
    }

    /// TODO TS
    void closeSplit(Window* window)
    {
        auto surface = tabSurface_(window);
        if (!surface || surface->splitCount() <= 1) return;

        auto tab_widget = surface->activeTabWidget();
        if (!tab_widget) return;

        if (canCloseSplitHook_ && !canCloseSplitHook_(window)) return;

        suppressAutoCollapse_ = true;

        for (auto i = tab_widget->count() - 1; i >= 0; --i)
            deleteFileViewAt_(tab_widget, i);

        suppressAutoCollapse_ = false;

        surface->removeSplit(tab_widget);
        emit bus->splitCountChanged(window);
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
        if (!view || !view->isUserEditable()) return;
        if (view->hasSelection()) view->cut();
    }

    void copy(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->isUserEditable()) return;
        if (view->hasSelection()) view->copy();
    }

    void paste(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->isUserEditable()) return;
        if (view->hasPaste()) view->paste();
    }

    void del(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->isUserEditable()) return;
        if (view->hasSelection()) view->deleteSelection();
    }

    void selectAll(Window* window, int index = -1)
    {
        auto view = fileViewAt(window, index);
        if (!view || !view->isUserEditable()) return;
        view->selectAll();
    }

signals:
    void addTabRequested(Window* window);
    void fileViewDestroyed(AbstractFileView* fileView);
    void tabDragCompleted(Window* fromWindow, Window* toWindow); /// TODO TD
    void tabDraggedToNewWindow(
        Window* sourceWindow,
        const QPoint& dropPos,
        const TabWidget::TabSpec& tabSpec); /// TODO TD
    void
    tabContextMenuRequested(Window* window, int index, const QPoint& globalPos);
    void addButtonContextMenuRequested(Window* window, const QPoint& globalPos);

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
            &Bus::fileModelExternallyModified,
            this,
            &ViewService::onBusFileModelExternallyModified_);

        connect(
            bus,
            &Bus::fileModelPathInvalidated,
            this,
            &ViewService::onBusFileModelPathInvalidated_);

        connect(
            bus,
            &Bus::settingChanged,
            this,
            [this](const QString& key, const QVariant& value) {
                if (auto applier = settingAppliers_.value(key)) applier(value);
            });
    }

private:
    // Prevents auto-collapse from deleting a TabWidget while closeSplit is
    // iterating over its tabs (closeSplit handles removal itself after the
    // loop)
    bool suppressAutoCollapse_ = false; /// TODO TS

    QHash<Window*, AbstractFileView*> activeFileViews_{};
    QHash<AbstractFileModel*, int> fileViewsPerModel_{};

    // Dispatches live setting changes to all open TextFileViews. Populated from
    // textViewSettings_() in setup_(), keyed by Ini setting key. When
    // Bus::settingChanged fires, connectBusEvents() looks up the key here and
    // calls the matching applier (if any)
    QHash<QString, std::function<void(const QVariant&)>> settingAppliers_{};

    struct TextViewSetting_
    {
        QString key{};
        std::function<void(TextFileView*, const QVariant&)> apply = nullptr;
    };

    // Maps each Ini key to a function that applies the value to a view. Used in
    // setup_() (registers each entry as a live-change applier (via
    // settingAppliers_)) and applyInitialTextFileViewSettings_() (reads current
    // values and applies them to a newly created view)
    static const QList<TextViewSetting_>& textViewSettings_()
    {
        static const QList<TextViewSetting_> list{
            // Editor
            { Ini::Keys::EDITOR_FONT,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setFont(val.value<QFont>());
              } },
            { Ini::Keys::EDITOR_CENTER_ON_SCROLL,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setCenterOnScroll(val.value<bool>());
              } },
            { Ini::Keys::EDITOR_OVERWRITE,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setOverwriteMode(val.value<bool>());
              } },
            { Ini::Keys::EDITOR_TAB_STOP_DISTANCE,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setTabStopDistance(val.value<int>());
              } },
            { Ini::Keys::EDITOR_WRAP_MODE,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setWordWrapMode(
                      val.value<QTextOption::WrapMode>());
              } },
            { Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setDoubleClickWhitespace(val.value<bool>());
              } },
            { Ini::Keys::EDITOR_LINE_NUMBERS,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setLineNumbers(val.value<bool>());
              } },
            { Ini::Keys::EDITOR_LINE_HIGHLIGHT,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setLineHighlight(val.value<bool>());
              } },
            { Ini::Keys::EDITOR_SELECTION_HANDLES,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setSelectionHandles(val.value<bool>());
              } },
            { Ini::Keys::EDITOR_LR_MARGIN,
              [](TextFileView* v, const QVariant& val) {
                  v->editor()->setLeftRightMargin(val.value<int>());
              } },

            // Key filters
            { Ini::Keys::KEY_FILTERS_ACTIVE,
              [](TextFileView* v, const QVariant& val) {
                  v->keyFilters()->setActive(val.value<bool>());
              } },
            { Ini::Keys::KEY_FILTERS_AUTO_CLOSE,
              [](TextFileView* v, const QVariant& val) {
                  v->keyFilters()->setAutoClosing(val.value<bool>());
              } },
            { Ini::Keys::KEY_FILTERS_BARGING,
              [](TextFileView* v, const QVariant& val) {
                  v->keyFilters()->setBarging(val.value<bool>());
              } },
        };

        return list;
    }

    void setup_()
    {
        for (const auto& setting : textViewSettings_()) {
            settingAppliers_[setting.key] = [this,
                                             &setting](const QVariant& v) {
                forEachTextFileView_([&setting, v](TextFileView* view) {
                    setting.apply(view, v);
                });
            };
        }
    }

    /// TODO TS
    bool isMultiWindow_(AbstractFileModel* fileModel) const
    {
        if (!fileModel) return false;

        auto window_count = 0;
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            for (auto tab_widget : tabWidgets_(window)) {
                if (indexOfModel_(tab_widget, fileModel) >= 0) {
                    if (++window_count >= 2) return true;
                    break; // Found in this window, move to next
                }
            }
        }

        return false;
    }

    /// TODO TS
    TabSurface* tabSurface_(Window* window) const
    {
        if (!window) return nullptr;
        return qobject_cast<TabSurface*>(window->centralWidget());
    }

    /// TODO TS
    TabWidget* activeTabWidget_(Window* window) const
    {
        auto surface = tabSurface_(window);
        return surface ? surface->activeTabWidget() : nullptr;
    }

    /// TODO TS
    QList<TabWidget*> tabWidgets_(Window* window) const
    {
        auto surface = tabSurface_(window);
        return surface ? surface->tabWidgets() : QList<TabWidget*>{};
    }

    /// TODO TS
    void closeTabIn_(TabWidget* tabWidget, int index)
    {
        if (!tabWidget || index < 0) return;
        auto view = tabWidget->widgetAt<AbstractFileView*>(index);
        if (!view) return;
        auto model = view->model();
        auto window = Coco::findParent<Window*>(tabWidget);

        if (!canCloseTabHook_ || canCloseTabHook_(window, model))
            deleteFileViewAt_(tabWidget, index);
    }

    /// TODO TS
    enum class SplitDirection_
    {
        Left,
        Right
    };

    /// TODO TS
    TabWidget* findOrCreateSplit_(
        TabSurface* surface,
        TabWidget* current,
        SplitDirection_ direction)
    {
        if (!surface || !current) return nullptr;

        auto neighbor = (direction == SplitDirection_::Left)
                            ? surface->leftOf(current)
                            : surface->rightOf(current);

        if (neighbor) return neighbor;

        return (direction == SplitDirection_::Left)
                   ? surface->addSplitBefore(current)
                   : surface->addSplitAfter(current);
    }

    /// TODO TS
    void moveToSplit_(Window* window, SplitDirection_ direction, int index = -1)
    {
        auto surface = tabSurface_(window);
        if (!surface) return;

        auto source = surface->activeTabWidget();
        if (!source || source->isEmpty()) return;

        auto i = normalizeIndex_(source, index);
        if (i < 0) return;

        auto target = findOrCreateSplit_(surface, source, direction);
        if (!target) return;

        auto spec = source->tabSpecAt(i);
        if (!spec.isValid()) return;

        source->removeTab(i);

        auto new_index = target->addTab(spec);
        target->setCurrentIndex(new_index);

        /// TODO TS: Should we be setting widget focus, too, every where we call
        /// setCurrentIndex for a tab drop (here or maybe also in TabWidget) (I
        /// think maybe we already do here in ViewService)?
        if (spec.widget) spec.widget->setFocus();
    }

    /// TODO TS
    void
    duplicateToSplit_(Window* window, SplitDirection_ direction, int index = -1)
    {
        auto surface = tabSurface_(window);
        if (!surface) return;

        auto source = surface->activeTabWidget();
        if (!source || source->isEmpty()) return;

        auto i = normalizeIndex_(source, index);
        if (i < 0) return;

        auto view_at_i = source->widgetAt<AbstractFileView*>(i);
        if (!view_at_i) return;
        auto model = view_at_i->model();
        if (!model) return;

        auto target = findOrCreateSplit_(surface, source, direction);
        if (!target) return;

        auto view = createFileView_(window, model);
        if (!view) return;

        auto meta = model->meta();
        auto new_index = target->addTab(view, meta->title());
        target->setTabFlagged(new_index, model->isModified());
        target->setTabToolTip(new_index, meta->toolTip());
        target->setCurrentIndex(new_index);
        view->setFocus();
    }

    // If index is -1, it will become current index
    int normalizeIndex_(TabWidget* tabWidget, int index) const
    {
        if (!tabWidget || tabWidget->isEmpty()) return -1;
        auto i = (index < 0) ? tabWidget->currentIndex() : index;
        return (i < 0 || i >= tabWidget->count()) ? -1 : i;
    }

    // Index -1 = current
    /// TODO TS
    void deleteFileViewAt_(TabWidget* tabWidget, int index)
    {
        if (!tabWidget) return;
        auto i = normalizeIndex_(tabWidget, index);
        if (i < 0) return;

        auto view = tabWidget->removeTab<AbstractFileView*>(i);
        if (!view) return;

        delete view;
    }

    /// TODO TS
    void deleteAllFileViewsIn_(Window* window)
    {
        for (auto tab_widget : tabWidgets_(window)) {
            auto views = tab_widget->clear<AbstractFileView*>();
            for (auto& view : views) {
                delete view;
            }
        }
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

    /// TODO TS
    void addTabSurface_(Window* window)
    {
        if (!window) return;

        auto tab_surface = new TabSurface(window);
        window->setCentralWidget(tab_surface);

        connect(
            tab_surface,
            &TabSurface::splitAdded,
            this,
            &ViewService::wireTabWidget_);

        connect(
            tab_surface,
            &TabSurface::splitAdded,
            this,
            [this, window]([[maybe_unused]] TabWidget*) {
                emit bus->splitCountChanged(window);
            });

        connect(
            tab_surface,
            &TabSurface::splitEmpty,
            this,
            [this, tab_surface, window](TabWidget* tabWidget) {
                if (!suppressAutoCollapse_ && tab_surface->splitCount() > 1) {
                    tab_surface->removeSplit(tabWidget);
                    emit bus->splitCountChanged(window);
                }
            });

        connect(
            tab_surface,
            &TabSurface::activeTabWidgetChanged,
            this,
            [this, window](TabWidget* tw) {
                auto index = tw ? tw->currentIndex() : -1;
                setActiveFileView_(window, index);
            });

        // Create the initial split (triggers splitAdded -> wireTabWidget_)
        tab_surface->addSplit();
    }

    /// TODO TS
    void wireTabWidget_(TabWidget* tab_widget)
    {
        if (!tab_widget) return;

        auto window = Coco::findParent<Window*>(tab_widget);
        if (!window) return;

        tab_widget->setElideMode(Qt::ElideRight);
        tab_widget->setDragValidator(
            this,
            &ViewService::tabWidgetDragValidator_);
        tab_widget->setTabsDraggable(true);

        connect(
            tab_widget,
            &TabWidget::currentChanged,
            this,
            [this, window, tab_widget](int index) {
                // Only update active view if this is the active split
                if (activeTabWidget_(window) == tab_widget)
                    setActiveFileView_(window, index);
            });

        connect(tab_widget, &TabWidget::addTabRequested, this, [this, window] {
            emit addTabRequested(window);
        });

        connect(
            tab_widget,
            &TabWidget::closeTabRequested,
            this,
            [this, tab_widget](int index) { closeTabIn_(tab_widget, index); });

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

        /// TODO TS
        connect(
            tab_widget,
            &TabWidget::tabDraggedToSplitEdge,
            this,
            &ViewService::onTabDraggedToSplitEdge_);

        /// TODO TS
        connect(tab_widget, &TabWidget::dragStarted, this, [this] {
            suppressAutoCollapse_ = true;
        });

        /// TODO TS
        connect(tab_widget, &TabWidget::dragEnded, this, [this] {
            suppressAutoCollapse_ = false;
            cleanupEmptySplits_();
        });

       connect(
            tab_widget,
            &TabWidget::tabContextMenuRequested,
            this,
            [this, window, tab_widget](int index, const QPoint& globalPos) {
                tab_widget->setFocus();
                emit tabContextMenuRequested(window, index, globalPos);
            });

        connect(
            tab_widget,
            &TabWidget::addButtonContextMenuRequested,
            this,
            [this, window](const QPoint& globalPos) {
                emit addButtonContextMenuRequested(window, globalPos);
            });
    }

    /// TODO TD
    bool tabWidgetDragValidator_(TabWidget* source, TabWidget* destination)
    {
        auto source_window = Coco::findParent<Window*>(source);
        auto target_window = Coco::findParent<Window*>(destination);

        auto our_windows = bus->call<QSet<Window*>>(Bus::WINDOWS_SET);

        auto is_valid = source_window && target_window
                        && our_windows.contains(source_window)
                        && our_windows.contains(target_window);

        if (!is_valid) INFO("Rejected tab drag between different Workspaces");

        return is_valid;
    }

    /// TODO TS
    void cleanupEmptySplits_()
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            auto surface = tabSurface_(window);
            if (!surface) continue;

            QList<TabWidget*> empty{};
            for (auto tw : surface->tabWidgets())
                if (tw->isEmpty()) empty << tw;

            for (auto tw : empty) {
                if (surface->splitCount() > 1) {
                    surface->removeSplit(tw);
                    emit bus->splitCountChanged(window);
                }
            }
        }
    }

    /// TODO TS
    template <typename ViewT, typename CallableT>
    void forEachFileView_(CallableT&& callable)
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            for (auto tab_widget : tabWidgets_(window)) {
                for (auto i = 0; i < tab_widget->count(); ++i) {
                    if (auto view = tab_widget->widgetAt<ViewT>(i)) {
                        callable(view);
                    }
                }
            }
        }
    }

    template <typename CallableT>
    void forEachTextFileView_(CallableT&& callable)
    {
        forEachFileView_<TextFileView*>(std::forward<CallableT>(callable));
    }

    /// TODO TS
    template <typename CallableT>
    void forEachTabOfModel_(AbstractFileModel* model, CallableT&& callable)
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            for (auto tab_widget : tabWidgets_(window)) {
                for (auto i = 0; i < tab_widget->count(); ++i) {
                    auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                    if (view && view->model() == model) callable(tab_widget, i);
                }
            }
        }
    }

    // Returns index of first tab with this model in the given TabWidget, or -1
    int indexOfModel_(TabWidget* tabWidget, AbstractFileModel* model) const
    {
        if (!tabWidget || !model) return -1;

        for (auto i = 0; i < tabWidget->count(); ++i) {
            auto view = tabWidget->widgetAt<AbstractFileView*>(i);
            if (view && view->model() == model) return i;
        }

        return -1;
    }

    void applyInitialTextFileViewSettings_(TextFileView* textFileView)
    {
        if (!textFileView) return;

        for (const auto& setting : textViewSettings_()) {
            auto value = bus->call<QVariant>(
                Bus::GET_SETTING,
                { { "key", setting.key } });
            setting.apply(textFileView, value);
        }
    }

    AbstractFileView*
    createFileView_(Window* window, AbstractFileModel* fileModel)
    {
        if (!window || !fileModel) return nullptr;

        AbstractFileView* view = nullptr;

        if (auto text_model = qobject_cast<TextFileModel*>(fileModel)) {
            // TextFileView and subclasses only
            switch (text_model->meta()->fileType()) {
            case Files::Fountain: {
                view = newFileView_<FountainFileView*>(text_model, window);
                break;
            }
            case Files::Markdown: {
                view = newFileView_<MarkdownFileView*>(text_model, window);
                break;
            }
            default: {
                view = newFileView_<TextFileView*>(text_model, window);
                break;
            }
            }

            applyInitialTextFileViewSettings_(
                qobject_cast<TextFileView*>(view));

        } else if (auto pdf_model = qobject_cast<PdfFileModel*>(fileModel)) {
            view = newFileView_<PdfFileView*>(pdf_model, window);

        } else if (auto raw_model = qobject_cast<RawFileModel*>(fileModel)) {
            switch (raw_model->meta()->fileType()) {
            case Files::Html: {
                view = newFileView_<HtmlFileView*>(raw_model, window);
                break;
            }

            // case Files::Png:
            // case Files::Tiff:
            // case Files::Gif:
            // case Files::Jpeg:
            // case Files::Bmp:
            // case Files::WebP:
            default: {
                view = newFileView_<ImageFileView*>(raw_model, window);
                break;
            }
            }

        } else {
            UNREACHABLE("Type not deduced for model [{}]!", fileModel);
        }

        if (!view) return nullptr;

        auto meta = fileModel->meta();
        if (!meta) {
            delete view; // Anything else?
            return nullptr;
        }

        // Only adjust this once we're clear
        ++fileViewsPerModel_[fileModel];

        emit bus->fileViewCreated(view);

        connect(view, &QObject::destroyed, this, [this, view, fileModel] {
            if (--fileViewsPerModel_[fileModel] <= 0)
                fileViewsPerModel_.remove(fileModel);
            INFO("File view destroyed [{}] for model [{}]", view, fileModel);
            emit fileViewDestroyed(view);
        });

        return view;
    }

private slots:
    /// TODO TS
    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;
        addTabSurface_(window);
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
        auto tab_widget = activeTabWidget_(window);
        if (!tab_widget) return;

        auto view = createFileView_(window, fileModel);
        if (!view) return;

        auto meta = fileModel->meta();
        auto index = tab_widget->addTab(view, meta->title());
        tab_widget->setTabFlagged(index, fileModel->isModified());
        tab_widget->setTabToolTip(index, meta->toolTip());
        tab_widget->setCurrentIndex(index);
        view->setFocus();
    }

    void onBusFileModelModificationChanged_(
        AbstractFileModel* fileModel,
        bool modified)
    {
        if (!fileModel) return;

        forEachTabOfModel_(fileModel, [modified](TabWidget* tw, int i) {
            tw->setTabFlagged(i, modified);
        });
    }

    void onBusFileModelMetaChanged_(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;
        auto meta = fileModel->meta();
        if (!meta) return;

        auto title = meta->title();
        auto tool_tip = meta->toolTip();

        forEachTabOfModel_(fileModel, [title, tool_tip](TabWidget* tw, int i) {
            tw->setTabText(i, title);
            tw->setTabToolTip(i, tool_tip);
        });
    }

    void onBusFileModelExternallyModified_(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;

        auto meta = fileModel->meta();
        if (!meta) return;

        // Set alerts on all tabs for this model, and find a window to parent
        // the prompt
        Window* prompt_parent = nullptr;

        forEachTabOfModel_(fileModel, [&prompt_parent](TabWidget* tw, int i) {
            tw->setTabAlert(i, Tr::fileModifiedExternally());
            if (!prompt_parent) prompt_parent = Coco::findParent<Window*>(tw);
        });

        if (!prompt_parent) return;

        auto display_path = meta->isOnDisk()
                                ? meta->path()
                                : meta->title() + meta->preferredExt();

        if (ReloadPrompt::exec(display_path, prompt_parent)) {
            emit bus->fileModelReloadRequested(fileModel);
        } else {
            fileModel->setModified(true);
        }

        // Clear alerts regardless of choice (user has acknowledged)
        forEachTabOfModel_(fileModel, [](TabWidget* tw, int i) {
            tw->clearTabAlert(i);
        });
    }

    void onBusFileModelPathInvalidated_(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;

        forEachTabOfModel_(fileModel, [](TabWidget* tw, int i) {
            tw->setTabAlert(i, Tr::filePathInvalidated());
        });
    }

    /// TODO TD
    void onTabDragged_(
        const TabWidget::Location& old,
        const TabWidget::Location& now)
    {
        if (!old.isValid() || !now.isValid()) return;

        auto old_window = Coco::findParent<Window*>(old.tabWidget);
        auto new_window = Coco::findParent<Window*>(now.tabWidget);

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

        auto source_window = Coco::findParent<Window*>(source);
        emit tabDraggedToNewWindow(source_window, dropPos, tabSpec);
    }

    /// TODO TS
    // Drag-to-split always creates a new split at the drop location, even if a
    // neighbor already exists. This is intentional: the drag gesture targets a
    // specific visual position ("put a new split here"), unlike the menu
    // actions (splitLeft/splitRight) which reuse an existing neighbor when
    // available
    void onTabDraggedToSplitEdge_(
        TabWidget* source,
        TabWidget* dropTarget,
        const TabWidget::TabSpec& tabSpec,
        TabWidget::SplitSide side)
    {
        if (!tabSpec.isValid() || !dropTarget) return;

        auto window = Coco::findParent<Window*>(dropTarget);
        if (!window) return;

        auto surface = tabSurface_(window);
        if (!surface) return;

        auto new_split = (side == TabWidget::SplitSide::Left)
                             ? surface->addSplitBefore(dropTarget)
                             : surface->addSplitAfter(dropTarget);

        if (!new_split) return;

        auto new_index = new_split->addTab(tabSpec);
        new_split->setCurrentIndex(new_index);

        if (tabSpec.widget) tabSpec.widget->setFocus();

        auto source_window = Coco::findParent<Window*>(source);
        emit tabDragCompleted(source_window, window);
    }
};

} // namespace Fernanda
