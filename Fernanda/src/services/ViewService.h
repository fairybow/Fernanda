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
#include "models/AbstractFileModel.h"
#include "models/FileMeta.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "models/TextFileModel.h"
#include "services/AbstractService.h"
#include "services/ReloadPrompt.h"
#include "settings/Ini.h"
#include "ui/PlainTextEdit.h"
#include "ui/TabWidget.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "views/FountainFileView.h"
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

    DECLARE_HOOK(
        std::function<bool(Window*, int index)>,
        canCloseTabHook,
        setCanCloseTabHook)

    DECLARE_HOOK(
        std::function<bool(Window*, int index)>,
        canCloseTabEverywhereHook,
        setCanCloseTabEverywhereHook)

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
        auto tab_widget = tabWidget_(window);
        if (!tab_widget || !tabSpec.isValid()) return;

        auto index = tab_widget->addTab(tabSpec.widget, tabSpec.text);
        tab_widget->setTabData(index, tabSpec.userData);
        tab_widget->setTabToolTip(index, tabSpec.toolTip);
        tab_widget->setTabAlert(index, tabSpec.alertMessage);
        tab_widget->setTabFlagged(index, tabSpec.isFlagged);
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
        auto tab_widget = tabWidget_(window);
        if (!tab_widget) return;

        window->activate();
        tab_widget->setCurrentIndex(index);
    }

    // Raises Window and model, if found
    void raise(Window* window, AbstractFileModel* model) const
    {
        if (!window || !model) return;
        auto i = indexOfModel_(tabWidget_(window), model);
        if (i >= 0) raise(window, i);
    }

    // Raises first Window found (from top to bottom) containing model (and
    // raises model), if any, and returns the window
    // TODO: Should this return nullptr if the model isn't found?
    Window* raise(AbstractFileModel* model) const
    {
        if (!model) return nullptr;

        for (auto& window : bus->call<QList<Window*>>(Bus::WINDOWS)) {
            auto i = indexOfModel_(tabWidget_(window), model);

            if (i >= 0) {
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
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            if (indexOfModel_(tabWidget_(window), fileModel) >= 0) {
                if (++window_count >= 2) return true;
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

    void duplicateTab(Window* window, int index = -1)
    {
        auto model = fileModelAt(window, index);
        if (!model) return;
        auto tab_widget = tabWidget_(window);
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
            [this, window](int index) { setActiveFileView_(window, index); });

        connect(tab_widget, &TabWidget::addTabRequested, this, [this, window] {
            emit addTabRequested(window);
        });

        connect(
            tab_widget,
            &TabWidget::closeTabRequested,
            this,
            [this, window](int index) { closeTab(window, index); });

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

        connect(
            tab_widget,
            &TabWidget::tabContextMenuRequested,
            this,
            [this, window](int index, const QPoint& globalPos) {
                emit tabContextMenuRequested(window, index, globalPos);
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

    template <typename ViewT, typename CallableT>
    void forEachFileView_(CallableT&& callable)
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto view = tab_widget->widgetAt<ViewT>(i)) callable(view);
        }
    }

    template <typename CallableT>
    void forEachTextFileView_(CallableT&& callable)
    {
        forEachFileView_<TextFileView*>(std::forward<CallableT>(callable));
    }

    template <typename CallableT>
    void forEachTabOfModel_(AbstractFileModel* model, CallableT&& callable)
    {
        for (auto& window : bus->call<QSet<Window*>>(Bus::WINDOWS_SET)) {
            auto tab_widget = tabWidget_(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {
                auto view = tab_widget->widgetAt<AbstractFileView*>(i);
                if (view && view->model() == model) callable(tab_widget, i);
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
            case FileTypes::Fountain: {
                view = newFileView_<FountainFileView*>(text_model, window);
                break;
            }
            case FileTypes::Markdown: {
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

        } else if (
            auto image_model = qobject_cast<ImageFileModel*>(fileModel)) {
            view = newFileView_<ImageFileView*>(image_model, window);

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
};

} // namespace Fernanda
