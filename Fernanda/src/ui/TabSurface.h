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

#include <QApplication>
#include <QHBoxLayout>
#include <QList>
#include <QPointer>
#include <QSplitter>
#include <QWidget>

#include "core/Debug.h"
#include "ui/TabWidget.h"

namespace Fernanda {

// Creates TabWidgets and manages them (TabWidgets wired by ViewService)
class TabSurface : public QWidget
{
    Q_OBJECT

public:
    explicit TabSurface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~TabSurface() override { TRACER; }

    TabWidget* activeTabWidget() const { return activeTabWidget_; }

    QList<TabWidget*> tabWidgets() const
    {
        QList<TabWidget*> result{};

        for (auto i = 0; i < splitter_->count(); ++i) {
            if (auto tw = qobject_cast<TabWidget*>(splitter_->widget(i))) {
                result << tw;
            }
        }

        return result;
    }

    int splitCount() const { return splitter_->count(); }

    TabWidget* addSplit() { return insertSplit_(splitter_->count()); }

    void removeSplit(TabWidget* tabWidget)
    {
        if (!tabWidget || splitCount() <= 1) return;

        auto was_active = (tabWidget == activeTabWidget_);
        auto old_index = indexOf(tabWidget);

        tabWidget->setParent(nullptr);
        delete tabWidget;

        if (was_active) {
            auto clamped = qBound(0, old_index, splitter_->count() - 1);
            auto new_active = tabWidgetAt(clamped);
            setActiveTabWidget_(new_active);
            if (new_active) new_active->setFocus();
        }
    }

    int indexOf(TabWidget* tabWidget) const
    {
        for (auto i = 0; i < splitter_->count(); ++i) {
            if (splitter_->widget(i) == tabWidget) return i;
        }

        return -1;
    }

    TabWidget* tabWidgetAt(int index) const
    {
        if (index < 0 || index >= splitter_->count()) return nullptr;
        return qobject_cast<TabWidget*>(splitter_->widget(index));
    }

    TabWidget* leftOf(TabWidget* tabWidget) const
    {
        auto i = indexOf(tabWidget);
        return (i > 0) ? tabWidgetAt(i - 1) : nullptr;
    }

    TabWidget* rightOf(TabWidget* tabWidget) const
    {
        auto i = indexOf(tabWidget);
        return (i >= 0 && i < splitter_->count() - 1) ? tabWidgetAt(i + 1)
                                                      : nullptr;
    }

    TabWidget* addSplitBefore(TabWidget* tabWidget)
    {
        auto i = indexOf(tabWidget);
        if (i < 0) return nullptr;
        return insertSplit_(i);
    }

    TabWidget* addSplitAfter(TabWidget* tabWidget)
    {
        auto i = indexOf(tabWidget);
        if (i < 0) return nullptr;
        return insertSplit_(i + 1);
    }

signals:
    void activeTabWidgetChanged(TabWidget* tabWidget);
    void splitAdded(TabWidget* tabWidget);
    void splitEmpty(TabWidget* tabWidget);

private:
    QSplitter* splitter_ = new QSplitter(Qt::Horizontal, this);
    QPointer<TabWidget> activeTabWidget_ = nullptr;

    void setup_()
    {
        splitter_->setChildrenCollapsible(false);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(splitter_);

        /// TODO TS: Use App? Or go back and use qApp in other places to reduce
        /// source files...
        connect(
            qApp,
            &QApplication::focusChanged,
            this,
            [this]([[maybe_unused]] QWidget* old, QWidget* now) {
                if (!now) return;

                for (auto tw : tabWidgets()) {
                    if (tw == now || tw->isAncestorOf(now)) {
                        setActiveTabWidget_(tw);
                        return;
                    }
                }
            });
    }

    void setActiveTabWidget_(TabWidget* tabWidget)
    {
        if (activeTabWidget_ == tabWidget) return;
        activeTabWidget_ = tabWidget;
        emit activeTabWidgetChanged(tabWidget);
    }

    TabWidget* insertSplit_(int index)
    {
        auto tab_widget = new TabWidget(nullptr);

        // QSplitter::insertWidget takes ownership
        splitter_->insertWidget(index, tab_widget);

        connect(
            tab_widget,
            &TabWidget::tabCountChanged,
            this,
            [this, tab_widget] {
                if (tab_widget->isEmpty()) emit splitEmpty(tab_widget);
            });

        if (!activeTabWidget_) setActiveTabWidget_(tab_widget);

        emit splitAdded(tab_widget);
        return tab_widget;
    }
};

} // namespace Fernanda
