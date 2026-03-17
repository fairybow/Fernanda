/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
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

#include <QByteArray>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QStackedWidget>
#include <QString>
#include <QTabBar>
#include <QVariant>
#include <QWidget>
#include <QWidgetList>

#include <Coco/Concepts.h>
#include <Coco/Utility.h>

#include "ui/TabWidgetAlertWidget.h"
#include "ui/TabWidgetButton.h"
#include "ui/TabWidgetCloseButton.h"
#include "ui/TabWidgetTabBar.h"
#include "ui/TabWidgetUnderlay.h"

namespace Fernanda {

// Tabbed interface for multiple file views within a Window, supporting tab
// creation/closing, drag-and-drop between Windows, active tab management, and
// visual state indicators (flagged tabs)
// TODO:
// - Visual feedback for the drag (a moving gap showing the drop area where
//   the in-progress dragged tab will go, following the cursor over the tab
//   bar)
// - Place the tab at the index it was dragged to, not just at the end
// - Pixmap is blurry. Maybe SVG somehow
// - Cursor customization for drag-outside (currently shows "no" cursor)
class TabWidget : public QWidget
{
    Q_OBJECT

public:
    using DragValidator =
        std::function<bool(TabWidget* source, TabWidget* destination)>;

    struct Location
    {
        QPointer<TabWidget> tabWidget = nullptr;
        int index = -1;
        bool isValid() const noexcept;
    };

    struct TabSpec
    {
        QWidget* widget = nullptr;
        QVariant userData{};
        QString text{};
        QString toolTip{};
        QString alertMessage{};
        bool isFlagged = false;
        QPointF offsetRatios{};

        static QPointF getOffsetRatios(const QRect& rect, const QPoint& pos);
        QPoint relPos(int width, int height) const noexcept;
        bool isValid() const noexcept;
    };

    explicit TabWidget(QWidget* parent = nullptr);
    virtual ~TabWidget() override;

    void activatePrevious();
    void activateNext();

    virtual bool eventFilter(QObject* watched, QEvent* event) override;

    // --- Queries ---

    bool isEmpty() const;
    int count() const;
    int currentIndex() const;
    QWidget* currentWidget() const;
    int indexOf(const QWidget* widget) const;
    QWidget* widgetAt(int index) const;
    QList<QWidget*> widgets() const;

    template <Coco::Concepts::QWidgetPointer T> T currentWidget() const
    {
        return qobject_cast<T>(currentWidget());
    }

    template <Coco::Concepts::QWidgetPointer T> T widgetAt(int index) const
    {
        return qobject_cast<T>(widgetAt(index));
    }

    template <Coco::Concepts::QWidgetPointer T> QList<T> widgets() const
    {
        QList<T> result{};

        for (auto i = 0; i < tabBar_->count(); ++i)
            if (auto widget = widgetAt<T>(i)) result << widget;

        return result;
    }

    // --- Tabs ---

    /// TODO: Rename a lot of these (e.g., textAt instead of tabText, maybe)

    int addTab(QWidget* widget, const QString& tabText);
    int insertTab(int index, QWidget* widget, const QString& tabText);
    QWidget* removeTab(int index);

    // TODO: Rename to take/removeAll (clear implies deletion)?
    QWidgetList clear();

    template <Coco::Concepts::QWidgetPointer T> T removeTab(int index)
    {
        return qobject_cast<T>(removeTab(index));
    }

    // TODO: Rename to take/removeAll (clear implies deletion)?
    template <Coco::Concepts::QWidgetPointer T> QList<T> clear()
    {
        QList<T> widgets{};
        while (!isEmpty())
            widgets << removeTab<T>(0);
        return widgets;
    }

    QVariant tabData(int index) const;
    void setTabData(int index, const QVariant& data);

    bool tabFlagged(int index) const;
    void setTabFlagged(int index, bool flagged);

    bool tabAlert(int index) const;
    QString tabAlertMessage(int index) const;
    void setTabAlert(int index, const QString& message);
    void clearTabAlert(int index);

    QIcon tabIcon(int index) const;
    void setTabIcon(int index, const QIcon& icon);

    QString tabToolTip(int index) const;
    void setTabToolTip(int index, const QString& tip);

    bool isTabEnabled(int index) const;
    void setTabEnabled(int index, bool enabled);

    QString tabText(int index) const;
    void setTabText(int index, const QString& text);

    QRect tabRect(int index) const;

    // --- Tab bar ---

    bool tabsClosable() const noexcept;
    void setTabsClosable(bool closable);

    QSize minimumTabSize() const;
    QSize maximumTabSize() const;

    Qt::TextElideMode elideMode() const;
    void setElideMode(Qt::TextElideMode mode);

    bool isMovable() const;
    void setMovable(bool movable);

    bool tabBarAutoHide() const;
    void setTabBarAutoHide(bool enabled);

    QTabBar::Shape tabShape() const;
    void setTabShape(QTabBar::Shape shape);

    bool usesScrollButtons() const;
    void setUsesScrollButtons(bool useButtons);

    // --- Tab dragging ---

    DragValidator dragValidator() const noexcept;
    void setDragValidator(const DragValidator& validator);

    template <typename ClassT>
    void setDragValidator(
        ClassT* object,
        bool (ClassT::*method)(TabWidget*, TabWidget*))
    {
        dragValidator_ = [object,
                          method](TabWidget* source, TabWidget* destination) {
            return (object->*method)(source, destination);
        };
    }

    bool tabsDraggable() const noexcept;
    void setTabsDraggable(bool draggable);

signals:
    void currentChanged(int index);
    void tabCountChanged();
    void addTabRequested();
    void closeTabRequested(int index);
    void tabBarClicked(int index);
    void tabBarDoubleClicked(int index);
    void tabMoved(int from, int to);
    void tabDragged(const Location& old, const Location& now);
    void tabDraggedOutside(
        TabWidget* source,
        const QPoint& dropPos,
        const TabSpec& tabSpec); /// TODO TD

public slots:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget* widget);

protected:
    virtual void tabInserted(int index);
    virtual void tabRemoved(int index);
    // TODO: tabLayoutChanged hook?

    /// TODO TD:

    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dragMoveEvent(QDragMoveEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;

private:
    COCO_TRIVIAL_CLASS(WidgetStack_, QStackedWidget);
    static constexpr auto TAB_BAR_HEIGHT_ = 34;
    static constexpr auto MIN_TAB_WIDTH_ = 75;
    static constexpr auto MAX_TAB_WIDTH_ = 225;
    static constexpr auto ADD_BUTTON_SIZE_ = QSize(30, 30);
    static constexpr auto CLOSE_BUTTON_SIZE_ = QSize(24, 24);
    static constexpr auto BUTTON_SVG_SIZE_ = QSize(18, 18);
    static constexpr auto MIME_TYPE_ = "application/x-fernanda-tab-widget-tab";

    struct TabDragContext_
    {
        TabWidget* origin = nullptr;
        int originIndex = -1;
        TabSpec tabSpec{};
        bool isValid() const noexcept;
    };

    // TODO:
    // QToolButton* dummyL_ = new QToolButton(this);
    // Scroll left here, eventually
    TabWidgetTabBar* tabBar_ = new TabWidgetTabBar(this);
    // Scroll right here, eventually
    TabWidgetButton* addButton_ = new TabWidgetButton(this);
    QWidget* spacer_ = new QWidget(this);
    // QToolButton* dummyR_ = new QToolButton(this);

    QStackedWidget* mainStack_ = new QStackedWidget(this);
    TabWidgetUnderlay* underlay_ = new TabWidgetUnderlay(this);
    WidgetStack_* widgetStack_ = new WidgetStack_(this);

    bool tabsClosable_ = true;
    QList<TabWidgetCloseButton*> closeButtons_{};
    QHash<QWidget*, QVariant> tabUserData_{};

    DragValidator dragValidator_ = nullptr;
    bool tabsDraggable_ = false;
    QPoint dragStartPosition_{};
    int dragPressIndex_ = -1;

    void setup_();
    void initializeTabBarPropagatedSignals_();

    int addOrInsertTab_(int index, QWidget* widget, const QString& tabText);

    void addCloseButtonAt_(int index);
    TabWidgetCloseButton* closeButtonAt_(int index) const;
    void updateMouseHoverAfterLayoutChange_(); // TODO: Review

    TabWidgetAlertWidget* alertWidgetAt_(int index) const;

    void startDrag_(int index);
    QByteArray serialize_(const TabDragContext_& dragContext);
    TabDragContext_ deserialize_(QByteArray& data);
    int addDroppedTab_(const TabSpec& tabSpec);
    QPixmap dragPixmap_(const QString& tabText) const;

private slots:
    void onTabBarCurrentChanged_(int index);
    void onWidgetStackCurrentChanged_(int index);
};

} // namespace Fernanda
