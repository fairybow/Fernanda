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

#include "Bus.h"
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
#include "Utility.h"
// #include "ViewServiceCloseHelper.h"
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
        setup_();
    }

    virtual ~ViewService() override { TRACER; }

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    QHash<Window*, IFileView*> activeFileViews_{};
    QHash<IFileModel*, int> viewsPerModel_{};
    // ViewServiceCloseHelper* closeHelper_ = nullptr;

    void setup_()
    {
        // closeHelper_ = new ViewServiceCloseHelper(bus, this);
    }

    // Active file view can be set nullptr!
    void setActiveFileView_(Window* window, int index)
    {
        if (!window) return;

        IFileView* active = nullptr;

        if (index > -1)
            if (auto view = Util::viewAt(window, index)) active = view;

        activeFileViews_[window] = active;
        emit bus->activeFileViewChanged(active, window);
    }

    void undoAt_(Window* window, int index)
    {
        auto model = Util::modelAt(window, index);
        if (!model) return;

        if (model->hasUndo()) model->undo();
    }

    void redoAt_(Window* window, int index)
    {
        auto model = Util::modelAt(window, index);
        if (!model) return;

        if (model->hasRedo()) model->redo();
    }

    void cutAt_(Window* window, int index)
    {
        auto view = Util::viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasSelection()) view->cut();
    }

    void copyAt_(Window* window, int index)
    {
        auto view = Util::viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasSelection()) view->copy();
    }

    void pasteAt_(Window* window, int index)
    {
        auto view = Util::viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasPaste()) view->paste();
    }

    void deleteAt_(Window* window, int index)
    {
        auto view = Util::viewAt(window, index);
        if (!view || !view->supportsEditing()) return;

        if (view->hasSelection()) view->deleteSelection();
    }

    void selectAllAt_(Window* window, int index)
    {
        auto view = Util::viewAt(window, index);
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
            /// bus->execute(Cmd::NewTab, window);
            TRACER;
            qDebug() << "Implement";
        });

        connect(
            tab_widget,
            &TabWidget::closeTabRequested,
            this,
            [&, window](int index) {
                /// bus->execute(Cmd::CloseView, { { "index", index } },
                /// window);
                TRACER;
                qDebug() << "Implement";
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

        // IFileView* view = nullptr;

        // if (auto text_model = cast<TextFileModel*>(model)) {

        //    auto text_view = make_<TextFileView*>(text_model, window);
        //    auto font = bus->call<QFont>(
        //        Commands::SETTINGS_GET,
        //        { { "key", Ini::Editor::FONT_KEY },
        //          { "default", Ini::Editor::defaultFont() } });
        //    text_view->setFont(font);
        //    view = text_view;

        //} else if (auto no_op_model = cast<NoOpFileModel*>(model)) {
        //    view = make_<NoOpFileView*>(no_op_model, window);
        //} else {
        //    return;
        //}

        // if (!view) return;
        //++viewsPerModel_[model];

        // auto tab_widget = Util::tabWidget(window);
        // if (!tab_widget) return; // Delete view if this fails (shouldn't)?

        // auto meta = model->meta();
        // if (!meta) return;

        // auto index = tab_widget->addTab(view, meta->title());
        // tab_widget->setTabFlagged(index, model->isModified());
        // tab_widget->setTabToolTip(index, meta->toolTip());
        // tab_widget->setCurrentIndex(index);
        // view->setFocus();
    }

    void onFileModificationChanged_(IFileModel* model, bool modified)
    {
        if (!model) return;

        // Find all tabs containing views of this model
        /*for (auto window : bus->call<QSet<Window*>>(Commands::WINDOWS_SET)) {
            auto tab_widget = Util::tabWidget(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i) {
                auto view = tab_widget->widgetAt<IFileView*>(i);
                if (view && view->model() == model) {
                    tab_widget->setTabFlagged(i, modified);
                }
            }
        }*/
    }

    void onFileMetaChanged_(IFileModel* model)
    {
        if (!model) return;

        // auto meta = model->meta();
        // if (!meta) return;

        //// Find all tabs containing views of this model and update their
        //// text/tooltip
        // for (auto window : bus->call<QSet<Window*>>(Commands::WINDOWS_SET)) {
        //     auto tab_widget = Util::tabWidget(window);
        //     if (!tab_widget) continue;

        //    for (auto i = 0; i < tab_widget->count(); ++i) {
        //        auto view = tab_widget->widgetAt<IFileView*>(i);
        //        if (view && view->model() == model) {
        //            tab_widget->setTabText(i, meta->title());
        //            tab_widget->setTabToolTip(i, meta->toolTip());
        //        }
        //    }
        //}
    }

    void onSettingChanged_(const QString& key, const QVariant& value)
    {
        // Gotta handle multiple for editor stuff
        /*if (key != Ini::Editor::FONT_KEY) return;

        auto font = to<QFont>(value);

        for (auto window : bus->call<QSet<Window*>>(Commands::WINDOWS_SET)) {
            auto tab_widget = Util::tabWidget(window);
            if (!tab_widget) continue;

            for (auto i = 0; i < tab_widget->count(); ++i)
                if (auto text_view = tab_widget->widgetAt<TextFileView*>(i))
                    text_view->setFont(font);
        }*/
    }
};

} // namespace Fernanda
