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

    TabWidget* addSplit()
    {
        auto tab_widget = new TabWidget(splitter_);
        splitter_->addWidget(tab_widget);

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

    void removeSplit(TabWidget* tabWidget)
    {
        if (!tabWidget || splitCount() <= 1) return;

        auto was_active = (tabWidget == activeTabWidget_);

        tabWidget->setParent(nullptr);
        delete tabWidget;

        if (was_active) {
            auto remaining = tabWidgets();
            setActiveTabWidget_(
                remaining.isEmpty() ? nullptr : remaining.last());
        }
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
};

} // namespace Fernanda
