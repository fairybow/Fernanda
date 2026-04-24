/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QHash>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QWidget>

#include <Coco/Bool.h>
#include <Coco/Concepts.h>

#include "models/AbstractFileModel.h"
#include "services/AbstractService.h"
#include "ui/TabSurface.h"
#include "ui/TabWidget.h"
#include "ui/Window.h"
#include "views/TextFileView.h"
#include "workspaces/Bus.h"

namespace Hearth {

class ViewService : public AbstractService
{
    Q_OBJECT

public:
    explicit ViewService(Bus* bus, QObject* parent = nullptr);
    virtual ~ViewService() override;

    // --- Hooks ---

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

    DECLARE_HOOK(
        std::function<bool(Window*, AbstractFileModel*)>,
        shouldOpenTabHook,
        setShouldOpenTabHook)

    // --- Queries ---

    COCO_BOOL(ExcludeMultiWindow)

    bool anyViewsIn(Window* window) const;
    bool anyViews() const;
    bool anyModifiedFileModelsIn(Window* window) const;
    int countFor(AbstractFileModel* fileModel) const;
    int splitCount(Window* window) const;

    // (index -1 = current)

    AbstractFileView* fileViewAt(Window* window, int index) const;
    AbstractFileModel* fileModelAt(Window* window, int index) const;
    QList<AbstractFileView*> fileViewsIn(Window* window) const;
    QList<AbstractFileView*> fileViews() const;
    QList<AbstractFileModel*> modifiedViewModelsIn(
        Window* window,
        ExcludeMultiWindow excludeMultiWindow = ExcludeMultiWindow::No) const;
    QList<AbstractFileModel*>
    modifiedViewModelsInActiveSplit(Window* window) const;
    QList<AbstractFileModel*> modifiedViewModels() const;

    // --- Tabs ---

    // Insert a dragged tab into a window's TabWidget
    void insertTabSpec(Window* window, const TabWidget::TabSpec& tabSpec);

    // Raises Window and tab at index
    void raise(Window* window, int index) const;

    // Raises Window and model, if found
    void raise(Window* window, AbstractFileModel* model) const;

    // Raises first Window found (from top to bottom) containing model (and
    // raises model), if any, and returns the window
    Window* raise(AbstractFileModel* model) const;
    void duplicateTab(Window* window, int index = -1);

    void closeTab(Window* window, int index = -1);
    void closeTabEverywhere(Window* window, int index = -1);

    // No hook!
    void closeViewsForModels(const QSet<AbstractFileModel*>& fileModels);
    void closeWindowTabs(Window* window);
    void closeAllTabs();

    // --- Splits ---

    void splitLeft(Window* window, int index = -1);
    void splitRight(Window* window, int index = -1);
    void duplicateToSplitLeft(Window* window, int index = -1);
    void duplicateToSplitRight(Window* window, int index = -1);
    void closeSplit(Window* window);

    // --- Edit commands ---

    void undo(Window* window, int index = -1);
    void redo(Window* window, int index = -1);
    void cut(Window* window, int index = -1);
    void copy(Window* window, int index = -1);
    void paste(Window* window, int index = -1);
    void del(Window* window, int index = -1);
    void selectAll(Window* window, int index = -1);

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
    virtual void registerBusCommands() override;
    virtual void connectBusEvents() override;

private:
    /// TODO TS
    enum class SplitDirection_
    {
        Left,
        Right
    };

    struct TextViewSetting_
    {
        QString key{};
        std::function<void(TextFileView*, const QVariant&)> apply = nullptr;
    };

    // Prevents auto-collapse from deleting a TabWidget while closeSplit is
    // iterating over its tabs (closeSplit handles removal itself after the
    // loop)
    QHash<Window*, bool> suppressAutoCollapse_{}; /// TODO TS

    QHash<Window*, AbstractFileView*> activeFileViews_{};
    QHash<AbstractFileModel*, int> fileViewsPerModel_{};

    // Dispatches live setting changes to all open TextFileViews. Populated from
    // textViewSettings_() in setup_(), keyed by Ini setting key. When
    // Bus::settingChanged fires, connectBusEvents() looks up the key here and
    // calls the matching applier (if any)
    QHash<QString, std::function<void(const QVariant&)>> settingAppliers_{};

    // --- Setup & wiring ---

    void setup_();
    void addTabSurface_(Window* window);
    void wireTabWidget_(TabWidget* tabWidget);
    bool tabWidgetDragValidator_(TabWidget* source, TabWidget* destination);

    // --- Surface accessors ---

    TabSurface* tabSurface_(Window* window) const;
    TabWidget* activeTabWidget_(Window* window) const;
    QList<TabWidget*> tabWidgets_(Window* window) const;

    // --- View lifecycle ---

    template <
        Coco::Concepts::QWidgetPointer FileViewT,
        Coco::Concepts::QObjectPointer FileModelT>
    FileViewT newFileView_(FileModelT fileModel, QWidget* parent)
    {
        auto view = new std::remove_pointer_t<FileViewT>(fileModel, parent);
        view->initialize();
        return view;
    }

    AbstractFileView*
    createFileView_(Window* window, AbstractFileModel* fileModel);
    void deleteFileViewAt_(TabWidget* tabWidget, int index);
    void deleteAllFileViewsIn_(Window* window);

    // Active file view can be set nullptr!
    void setActiveFileView_(Window* window, int index);
    void applyInitialTextFileViewSettings_(TextFileView* textFileView);

    // --- Tab helpers ---

    // Adds view + model as a tab to tabWidget, sets flag/tooltip/current/focus.
    // If insertAt >= 0, inserts at that index, otherwise appends
    void addViewTab_(
        TabWidget* tabWidget,
        AbstractFileView* view,
        AbstractFileModel* model,
        int insertAt = -1);
    void closeTabIn_(TabWidget* tabWidget, int index);

    // Searches all tab widgets in `window` for a tab bound to `model`. If
    // found, sets the tab current and focuses. Returns true if focused
    bool focusExistingTabForModel_(Window* window, AbstractFileModel* model);

    // If index is -1, it will become current index
    int normalizeIndex_(TabWidget* tabWidget, int index) const;

    // Returns index of first tab with this model in the given TabWidget, or -1
    int indexOfModel_(TabWidget* tabWidget, AbstractFileModel* model) const;
    bool isMultiWindow_(AbstractFileModel* fileModel) const;

    // --- Split helpers ---

    TabWidget* findOrCreateSplit_(
        TabSurface* surface,
        TabWidget* current,
        SplitDirection_ direction);
    void
    moveToSplit_(Window* window, SplitDirection_ direction, int index = -1);
    void duplicateToSplit_(
        Window* window,
        SplitDirection_ direction,
        int index = -1);
    void cleanupEmptySplits_();

    // --- Iteration helpers ---

    /// TODO TS
    template <typename ViewT, typename CallableT>
    void forEachFileView_(CallableT&& callable)
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            for (auto& tab_widget : tabWidgets_(window)) {
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
            for (auto& tab_widget : tabWidgets_(window)) {
                for (auto i = 0; i < tab_widget->count(); ++i) {
                    auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                    if (view && view->model() == model) callable(tab_widget, i);
                }
            }
        }
    }

    template <typename MatcherT> void closeMatchingViews_(MatcherT&& match)
    {
        for (auto& window : bus->call<QList<Window*>>(Bus::WINDOWS)) {
            for (auto& tab_widget : tabWidgets_(window)) {
                for (auto i = tab_widget->count() - 1; i >= 0; --i) {
                    auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                    if (view && match(view->model())) {
                        deleteFileViewAt_(tab_widget, i);
                    }
                }
            }
        }
    }

    // --- Text view settings ---

    // Maps each Ini key to a function that applies the value to a view. Used in
    // setup_() (registers each entry as a live-change applier (via
    // settingAppliers_)) and applyInitialTextFileViewSettings_() (reads current
    // values and applies them to a newly created view)
    static const QList<TextViewSetting_>& textViewSettings_();

private slots:
    // --- Bus slots ---

    void onBusWindowCreated_(Window* window);
    void onBusWindowDestroyed_(Window* window);
    void onBusFileModelReadied_(Window* window, AbstractFileModel* fileModel);
    void onBusFileModelModificationChanged_(
        AbstractFileModel* fileModel,
        bool modified);
    void onBusFileModelMetaChanged_(AbstractFileModel* fileModel);
    void onBusFileModelExternallyModified_(AbstractFileModel* fileModel);
    void onBusFileModelPathInvalidated_(AbstractFileModel* fileModel);

    // --- Tab slots ---

    void onTabDragged_(
        const TabWidget::Location& old,
        const TabWidget::Location& now);
    void onTabDraggedOutside_(
        TabWidget* source,
        const QPoint& dropPos,
        const TabWidget::TabSpec& tabSpec);

    // Drag-to-split always creates a new split at the drop location, even if a
    // neighbor already exists. This is intentional: the drag gesture targets a
    // specific visual position ("put a new split here"), unlike the menu
    // actions (splitLeft/splitRight) which reuse an existing neighbor when
    // available
    void onTabDraggedToSplitEdge_(
        TabWidget* source,
        TabWidget* dropTarget,
        const TabWidget::TabSpec& tabSpec,
        TabWidget::SplitSide side);
};

} // namespace Hearth
