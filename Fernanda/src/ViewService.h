/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

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
#include "Coco/Debug.h"

#include "Bus.h"
#include "Constants.h"
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
#include "Utility.h"
#include "ViewServiceCloseHelper.h"
#include "Window.h"

namespace Fernanda {

// Creates and manages file views within Windows, routes editing commands,
// handles view lifecycles, propagates TabWidget signals, and tracks the number
// of views per model
class ViewService : public IService
{
    Q_OBJECT

public:
    ViewService(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        initialize_();
    }

    virtual ~ViewService() override { COCO_TRACER; }

private:
    QHash<Window*, IFileView*> activeFileViews_{};
    ViewServiceCloseHelper* closeHelper_ = nullptr;
    QHash<IFileModel*, int> viewsPerModel_{};

    void initialize_()
    {
        closeHelper_ = new ViewServiceCloseHelper(bus, this);

        bus->addCommandHandler(
            WorkspaceCmd::MODEL_VIEWS_COUNT,
            [&](const Command& cmd) {
                if (auto model = cmd.param<IFileModel*>("model"))
                    return viewsPerModel_[model];

                return -1;
            });

        /*bus->addCommandHandler(Cmd::CloseView, [&](const Command& cmd) {
            return closeHelper_->closeAt(
                cmd.context,
                to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::CloseWindowViews,
            [&](const Command& cmd) {
                return closeHelper_->closeAllInWindow(cmd.context);
            });

        bus->addCommandHandler(Cmd::CloseAllViews, [&] {
            return closeHelper_->closeAll();
        });

        bus->addCommandHandler(Cmd::Undo, [&](const Command& cmd) {
            undoAt_(cmd.context, to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::Redo, [&](const Command& cmd) {
            redoAt_(cmd.context, to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::Cut, [&](const Command& cmd) {
            cutAt_(cmd.context, to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::Copy, [&](const Command& cmd) {
            copyAt_(cmd.context, to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::Paste, [&](const Command& cmd) {
            pasteAt_(cmd.context, to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::Delete, [&](const Command& cmd) {
            deleteAt_(cmd.context, to<int>(cmd.params, "index", -1));
        });

        bus->addCommandHandler(Cmd::SelectAll,
            [&](const Command& cmd) {
                selectAllAt_(cmd.context, to<int>(cmd.params, "index", -1));
            });

        bus->addCommandHandler(Cmd::PreviousTab,
            [&](const Command& cmd) {
                if (cmd.context)
                    if (auto tab_widget = tabWidget(cmd.context))
                        tab_widget->activatePrevious();
            });

        bus->addCommandHandler(Cmd::NextTab,
            [&](const Command& cmd) {
                if (cmd.context)
                    if (auto tab_widget = tabWidget(cmd.context))
                        tab_widget->activateNext();
            });

        */

        /*bus->addCommandHandler(Cids::ActiveFileView, [&](const Command& cmd) {
            if (cmd.context)
                if (auto active_view =
                        activeFileViews_.value(cmd.context, nullptr))
                    return active_view;

            return nullptr;
        });*/

        /*
        bus->addCommandHandler(
            Cmd::WindowAnyViewsOnModifiedFiles,
            [&](const QVariantMap& params) {
                auto window = to<Window*>(params, "window");
                if (!window) return false;

                auto tab_widget = tabWidget(window);
                if (!tab_widget) return false;

                for (auto i = 0; i < tab_widget->count(); ++i) {
                    auto model = modelAt(window, i);
                    if (model && model->isModified()) return true;
                }
                return false;
            });

        bus->addCommandHandler(
            Cmd::WindowAnyFiles,
            [&](const QVariantMap& params) {
                auto window = to<Window*>(params, "window");
                if (!window) return false;

                auto tab_widget = tabWidget(window);
                return tab_widget ? tab_widget->count() > 0 : false;
            });

        bus->addCommandHandler(
            Cmd::WorkspaceAnyViewsOnModifiedFiles,
            [&] {
            for (auto window : bus->call<QSet<Window*>>(Cmd::WindowSet)) {
                if (bus->call<bool>(
                            Cmd::WindowAnyViewsOnModifiedFiles,
                            { { "window", toQVariant(window) } })) {
                        return true;
                    }
                }
                return false;
            });

        bus->addCommandHandler(Cmd::WorkspaceAnyFiles, [&] {
            for (auto window : bus->call<QSet<Window*>>(Cmd::WindowSet)) {
                if (bus->call<bool>(
                        Cmd::WindowAnyFiles,
                        { { "window", toQVariant(window) } })) {
                    return true;
                }
            }
            return false;
        });*/

        connect(bus, &Bus::windowCreated, this, &ViewService::onWindowCreated_);

        connect(bus, &Bus::windowDestroyed, this, [&](Window* window) {
            activeFileViews_.remove(window);
        });

        connect(bus, &Bus::fileReadied, this, &ViewService::onFileReadied_);

        connect(
            bus,
            &Bus::fileModificationChanged,
            this,
            &ViewService::onFileModificationChanged_);

        connect(
            bus,
            &Bus::fileMetaChanged,
            this,
            &ViewService::onFileMetaChanged_);

        connect(bus, &Bus::viewClosed, this, [&](IFileView* view) {
            if (!view) return;
            auto model = view->model();
            if (!model) return;
            --viewsPerModel_[model];
        });

        connect(
            bus,
            &Bus::settingChanged,
            this,
            &ViewService::onSettingChanged_);
    }

    // Active file view can be set nullptr!
    void setActiveFileView_(Window* window, int index)
    {
        if (!window) return;

        IFileView* active = nullptr;

        if (index > -1)
            if (auto view = viewAt(window, index)) active = view;

        activeFileViews_[window] = active;
        emit bus->activeFileViewChanged(active, window);
    }

    void undoAt_(Window* window, int index)
    {
        auto model = modelAt(window, index);
        if (!model) return;

        if (model->hasUndo()) model->undo();
    }

    void redoAt_(Window* window, int index)
    {
        auto model = modelAt(window, index);
        if (!model) return;

        if (model->hasRedo()) model->redo();
    }

    void cutAt_(Window* window, int index)
    {
        auto view = viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasSelection()) view->cut();
    }

    void copyAt_(Window* window, int index)
    {
        auto view = viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasSelection()) view->copy();
    }

    void pasteAt_(Window* window, int index)
    {
        auto view = viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasPaste()) view->paste();
    }

    void deleteAt_(Window* window, int index)
    {
        auto view = viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasSelection()) view->deleteSelection();
    }

    void selectAllAt_(Window* window, int index)
    {
        auto view = viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        view->selectAll();
    }

    template <
        Coco::Concepts::QWidgetPointer FileViewT,
        Coco::Concepts::QObjectPointer FileModelT>
    [[nodiscard]] FileViewT make_(FileModelT model, QWidget* parent)
    {
        auto view = new std::remove_pointer_t<FileViewT>(model, parent);
        view->initialize();
        return view;
    }

private slots:
    void onWindowCreated_(Window* window)
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

        connect(tab_widget, &TabWidget::addTabRequested, this, [=] {
            bus->execute(Cmd::NewTab, window);
        });

        connect(
            tab_widget,
            &TabWidget::closeTabRequested,
            this,
            [&, window](int index) {
                bus->execute(Cmd::CloseView, { { "index", index } }, window);
            });

        connect(tab_widget, &TabWidget::tabCountChanged, this, [=] {
            emit bus->windowTabCountChanged(window, tab_widget->count());
        });

        // connect(tab_widget, &TabWidget::tabDragged, this, [] {
        //     //...
        // });
        // connect(tab_widget, &TabWidget::tabDraggedToDesktop, this, [] {
        //     //...
        // });
    }

    void onFileReadied_(IFileModel* model, Window* window)
    {
        if (!model || !window) return;

        IFileView* view = nullptr;

        if (auto text_model = to<TextFileModel*>(model)) {

            auto text_view = make_<TextFileView*>(text_model, window);
            auto font = bus->call<QFont>(
                Cmd::GetSetting,
                { { "key", Ini::Editor::FONT_KEY },
                  { "default", Ini::Editor::defaultFont() } });
            text_view->setFont(font);
            view = text_view;

        } else if (auto no_op_model = to<NoOpFileModel*>(model)) {
            view = make_<NoOpFileView*>(no_op_model, window);
        } else {
            return;
        }

        if (!view) return;
        ++viewsPerModel_[model];

        auto tab_widget = tabWidget(window);
        if (!tab_widget) return; // Delete view if this fails (shouldn't)?

        auto meta = model->meta();
        if (!meta) return;

        auto index = tab_widget->addTab(view, meta->title());
        tab_widget->setTabFlagged(index, model->isModified());
        tab_widget->setTabToolTip(index, meta->toolTip());
        tab_widget->setCurrentIndex(index);
        view->setFocus();
    }

    void onFileModificationChanged_(IFileModel* model, bool modified)
    {
        if (!model) return;

        // Find all tabs containing views of this model
        for (auto window : bus->call<QSet<Window*>>(Cmd::WindowSet)) {
            auto tab_widget = tabWidget(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {
                auto view = tab_widget->widgetAt<IFileView*>(i);
                if (view && view->model() == model) {
                    tab_widget->setTabFlagged(i, modified);
                }
            }
        }
    }

    void onFileMetaChanged_(IFileModel* model)
    {
        if (!model) return;

        auto meta = model->meta();
        if (!meta) return;

        // Find all tabs containing views of this model and update their
        // text/tooltip
        for (auto window : bus->call<QSet<Window*>>(Cmd::WindowSet)) {
            auto tab_widget = tabWidget(window);
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

    void onSettingChanged_(const QString& key, const QVariant& value)
    {
        // Gotta handle multiple for editor stuff
        if (key != Ini::Editor::FONT_KEY) return;

        auto font = to<QFont>(value);

        for (auto window : bus->call<QSet<Window*>>(Cmd::WindowSet)) {
            auto tab_widget = tabWidget(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto text_view = tab_widget->widgetAt<TextFileView*>(i))
                    text_view->setFont(font);
        }
    }
};

} // namespace Fernanda
