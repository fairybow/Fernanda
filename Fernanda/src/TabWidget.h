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

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QCursor>
#include <QDataStream>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHash>
#include <QIODevice>
#include <QIcon>
#include <QLinearGradient>
#include <QList>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QStackedWidget>
#include <QString>
#include <QTabBar>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QWidgetList>
#include <Qt>
#include <QtGlobal>
#include <QtTypes>

#include "Coco/Concepts.h"
#include "Coco/Debug.h" /// Move trivial class?
#include "Coco/Utility.h"

#include "Debug.h"
#include "StyleContext.h"
#include "TabWidgetButton.h"
#include "TabWidgetCloseButton.h"
#include "TabWidgetTabBar.h"
#include "TabWidgetUnderlay.h"

namespace Fernanda {

// Tabbed interface for multiple file views within a Window, supporting tab
// creation/closing, drag-and-drop between Windows, active tab management, and
// visual state indicators (flagged tabs)
class TabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~TabWidget() override { TRACER; }

    void activatePrevious()
    {
        auto count = tabBar_->count();
        if (count <= 1) return;

        auto i = tabBar_->currentIndex();
        auto previous = (i - 1 + count) % count;
        setCurrentIndex(previous);
    }

    void activateNext()
    {
        auto count = tabBar_->count();
        if (count <= 1) return;

        auto i = tabBar_->currentIndex();
        auto next = (i + 1) % count;
        setCurrentIndex(next);
    }

public slots:
    void setCurrentIndex(int index) { tabBar_->setCurrentIndex(index); }
    void setCurrentWidget(QWidget* widget) { setCurrentIndex(indexOf(widget)); }

signals:
    void currentChanged(int index);
    void tabCountChanged();

private:
    COCO_TRIVIAL_CLASS(WidgetStack_, QStackedWidget);
    static constexpr auto TAB_BAR_HEIGHT_ = 34;
    static constexpr auto MIN_TAB_WIDTH_ = 75;
    static constexpr auto MAX_TAB_WIDTH_ = 225;
    static constexpr auto ADD_BUTTON_SIZE_ = QSize(30, 30);
    static constexpr auto CLOSE_BUTTON_SIZE_ = QSize(24, 24);
    static constexpr auto BUTTON_SVG_SIZE_ = QSize(18, 18);

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

    void setup_()
    {
        // Setup
        setAcceptDrops(tabsDraggable_);
        mainStack_->addWidget(underlay_);
        mainStack_->addWidget(widgetStack_);
        tabBar_->installEventFilter(this);
        tabBar_->setFixedHeight(TAB_BAR_HEIGHT_);
        tabBar_->setMaximumTabWidth(MIN_TAB_WIDTH_);
        tabBar_->setMaximumTabWidth(MAX_TAB_WIDTH_);
        addButton_->setFixedSize(ADD_BUTTON_SIZE_);
        addButton_->setIconSize(BUTTON_SVG_SIZE_);
        addButton_->setIcon(UiIcon::Plus);

        // Populate
        mainStack_->setCurrentIndex(0);

        // Layout
        auto main_layout = new QVBoxLayout(this);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);

        auto top_layout = new QHBoxLayout;
        top_layout->setContentsMargins(0, 0, 0, 0);
        top_layout->setSpacing(0);

        // top_layout->addWidget(dummyL_, 0);
        top_layout->addWidget(tabBar_, 0);
        top_layout->addWidget(addButton_, 0);
        top_layout->addWidget(spacer_, 1); // Important! Should be the only 1
        // top_layout->addWidget(dummyR_, 0);

        main_layout->addLayout(top_layout, 0);
        main_layout->addWidget(mainStack_, 1);

        // Connect
        connect(addButton_, &TabWidgetButton::clicked, this, [&] {
            emit addTabRequested();
        });

        connect(
            tabBar_,
            &QTabBar::currentChanged,
            this,
            &TabWidget::onTabBarCurrentChanged_);

        connect(
            widgetStack_,
            &QStackedWidget::currentChanged,
            this,
            &TabWidget::onWidgetStackCurrentChanged_);

        initializeTabBarPropagatedSignals_();
    }

    /// *** TAB MANAGEMENT *** ///

public:
    int addTab(QWidget* widget, const QString& tabText)
    {
        return addOrInsertTab_(-1, widget, tabText);
    }

    int insertTab(int index, QWidget* widget, const QString& tabText)
    {
        return addOrInsertTab_(index, widget, tabText);
    }

    QWidget* removeTab(int index)
    {
        auto widget = widgetAt(index);
        auto button = closeButtonAt_(index);

        tabBar_->removeTab(index);

        if (button) {
            closeButtons_.removeAll(button);
            delete button;
        }

        if (widget) // Should happen
        {
            widgetStack_->removeWidget(widget);
            widget->setParent(nullptr);
            tabUserData_.remove(widget);
        }

        // Force hover state update after layout change, so if the next-to-last
        // tab's close button is now under mouse after the last tab is closed,
        // we can ensure it is highlighted
        updateMouseHoverAfterLayoutChange_();

        tabRemoved(index); // Hook
        emit tabCountChanged(); // Right place for this?
        return widget;
    }

    template <Coco::Concepts::QWidgetPointer T> T removeTab(int index)
    {
        return qobject_cast<T>(removeTab(index));
    }

    // TODO: Rename to take/removeAll (clear implies deletion)?
    QWidgetList clear()
    {
        QWidgetList widgets{};
        while (!isEmpty())
            widgets << removeTab(0);
        return widgets;
    }

    // TODO: Rename to take/removeAll (clear implies deletion)?
    template <Coco::Concepts::QWidgetPointer T> QList<T> clear()
    {
        QList<T> widgets{};
        while (!isEmpty())
            widgets << removeTab<T>(0);
        return widgets;
    }

signals:
    void addTabRequested();
    void closeTabRequested(int index);

protected:
    virtual void tabInserted(int index) { (void)index; } // Hook
    virtual void tabRemoved(int index) { (void)index; } // Hook
    // tabLayoutChanged hook?

private:
    int addOrInsertTab_(int index, QWidget* widget, const QString& tabText)
    {
        widgetStack_->addWidget(widget);

        // When adding the first tab, the tab bar automatically changes its
        // current index, which would normally trigger the currentChanged
        // signal. However, this causes an issue because the signal would be
        // emitted before the widget is properly registered with the tab,
        // resulting in nullptr when consumers try to access the widget.
        //
        // To solve this, we temporarily block signals from the tab bar during
        // setup, then manually emit currentChanged after everything is properly
        // initialized.

        auto was_blocked = tabBar_->blockSignals(true);

        // Inserting at -1 is what tab bar's addTab does
        auto new_index = tabBar_->insertTab(index, tabText);

        // We set tab bar tab data to widget pointer for easy storage and
        // retrieval/location
        tabBar_->setTabData(new_index, QVariant::fromValue(widget));

        if (tabsClosable_) addCloseButtonAt_(new_index);

        tabBar_->blockSignals(was_blocked);

        // Auto-hide (tab bar default auto-hide is hide if 1 or less, not only
        // if none). We want our tab bar to collapse to 0 width only when
        // there's no tabs
        tabBar_->setVisible(tabBar_->count() > 0);

        // If this was the first tab, emit the signal manually after everything
        // is set up
        if (tabBar_->count() == 1) emit currentChanged(0);

        tabInserted(new_index); // Hook
        emit tabCountChanged(); // Right place for this?
        return new_index;
    }

    /// *** TAB PROPERTIES *** ///

public:
    bool tabFlagged(int index) const
    {
        if (auto button = closeButtonAt_(index)) return button->flagged();

        return false;
    }

    void setTabFlagged(int index, bool flagged)
    {
        if (auto button = closeButtonAt_(index)) button->setFlagged(flagged);
    }

    QVariant tabData(int index) const { return tabUserData_[widgetAt(index)]; }
    void setTabData(int index, const QVariant& data)
    {
        tabUserData_[widgetAt(index)] = data;
    }

private:
    QHash<QWidget*, QVariant> tabUserData_{};

    /// *** TAB CLOSING *** ///

public:
    bool tabsClosable() const noexcept { return tabsClosable_; }

    void setTabsClosable(bool closable)
    {
        if (tabsClosable_ == closable) return;
        tabsClosable_ = closable;

        if (closable) {
            for (auto i = 0; i < tabBar_->count(); ++i)
                addCloseButtonAt_(i);
        } else // !closable
        {
            for (auto i = 0; i < tabBar_->count(); ++i) {
                if (auto button = closeButtonAt_(i)) {
                    tabBar_->setTabButton(i, QTabBar::RightSide, nullptr);
                    closeButtons_.removeAll(button);
                    delete button;
                }
            }
        }
    }

private:
    bool tabsClosable_ = true;
    QList<TabWidgetCloseButton*> closeButtons_{};

    // TODO: Review/also use App
    void updateMouseHoverAfterLayoutChange_()
    {
        // Get current global mouse position
        auto global_mouse_pos = QCursor::pos();

        // Find which widget is actually under the mouse now
        auto widget_under_mouse = QApplication::widgetAt(global_mouse_pos);
        auto close_button_under_mouse =
            qobject_cast<TabWidgetCloseButton*>(widget_under_mouse);

        // Update all close buttons' hover states
        for (auto& button : closeButtons_) {
            if (!button) continue;

            if (button == close_button_under_mouse) {
                // This button should be in hover state but might not be
                if (!button->underMouse()) {
                    auto local_pos = button->mapFromGlobal(global_mouse_pos);
                    QEnterEvent enter_event(
                        local_pos,
                        local_pos,
                        global_mouse_pos);
                    QApplication::sendEvent(button, &enter_event);
                }
            } else {
                // This button should not be in hover state
                if (button->underMouse()) {
                    QEvent leave_event(QEvent::Leave);
                    QApplication::sendEvent(button, &leave_event);
                }
            }
        }
    }

    TabWidgetCloseButton* closeButtonAt_(int index) const
    {
        auto button = tabBar_->tabButton(index, QTabBar::RightSide);
        return qobject_cast<TabWidgetCloseButton*>(button);
    }

    void addCloseButtonAt_(int index)
    {
        auto close_button = new TabWidgetCloseButton(tabBar_);
        close_button->setFixedSize(CLOSE_BUTTON_SIZE_);
        close_button->setIconSize(BUTTON_SVG_SIZE_);
        close_button->setIcon(UiIcon::X);
        close_button->setFlagIcon(UiIcon::Dot);
        closeButtons_ << close_button;

        tabBar_->setTabButton(index, QTabBar::RightSide, close_button);

        connect(
            close_button,
            &TabWidgetCloseButton::clickedAt,
            this,
            [&](int i) { emit closeTabRequested(i); });
    }

    /// *** STACK MANAGEMENT *** ///

private slots:
    void onTabBarCurrentChanged_(int index)
    {
        if (auto widget = widgetAt(index))
            widgetStack_->setCurrentWidget(widget);

        emit currentChanged(index);
    }

    void onWidgetStackCurrentChanged_(int index)
    {
        // We do NOT use widget stack index for anything!
        (void)index;

        widgetStack_->count() ? mainStack_->setCurrentWidget(widgetStack_)
                              : mainStack_->setCurrentWidget(underlay_);
    }

    /// *** QUERIES *** ///

public:
    bool isEmpty() const { return tabBar_->count() < 1; }
    int count() const { return tabBar_->count(); }
    int currentIndex() const { return tabBar_->currentIndex(); }
    QWidget* currentWidget() const { return widgetAt(currentIndex()); }

    template <Coco::Concepts::QWidgetPointer T> T currentWidget() const
    {
        return qobject_cast<T>(currentWidget());
    }

    int indexOf(const QWidget* widget) const
    {
        for (auto i = 0; i < tabBar_->count(); ++i)
            if (widgetAt(i) == widget) return i;

        return -1;
    }

    // Will return nullptr for an invalid index
    QWidget* widgetAt(int index) const
    {
        auto data = tabBar_->tabData(index);
        return qvariant_cast<QWidget*>(data);
    }

    // Will return nullptr for an invalid index
    template <Coco::Concepts::QWidgetPointer T> T widgetAt(int index) const
    {
        return qobject_cast<T>(widgetAt(index));
    }

    QList<QWidget*> widgets() const
    {
        QList<QWidget*> w{};

        for (auto i = 0; i < tabBar_->count(); ++i)
            if (auto widget = widgetAt(i)) w << widget;

        return w;
    }

    template <Coco::Concepts::QWidgetPointer T> QList<T> widgets() const
    {
        QList<T> w{};

        for (auto i = 0; i < tabBar_->count(); ++i)
            if (auto widget = widgetAt<T>(i)) w << widget;

        return w;
    }

    /// *** TAB BAR PROPAGATION *** ///

public:
    QSize minimumTabSize() const { return tabBar_->minimumTabSize(); }
    QSize maximumTabSize() const { return tabBar_->maximumTabSize(); }

    // QTabBar propagation
    QRect tabRect(int index) const { return tabBar_->tabRect(index); }

    Qt::TextElideMode elideMode() const { return tabBar_->elideMode(); }
    void setElideMode(Qt::TextElideMode mode) { tabBar_->setElideMode(mode); }

    QIcon tabIcon(int index) const { return tabBar_->tabIcon(index); }
    void setTabIcon(int index, const QIcon& icon)
    {
        tabBar_->setTabIcon(index, icon);
    }

    QString tabToolTip(int index) const { return tabBar_->tabToolTip(index); }
    void setTabToolTip(int index, const QString& tip)
    {
        tabBar_->setTabToolTip(index, tip);
    }

    bool isMovable() const { return tabBar_->isMovable(); }
    void setMovable(bool movable) { tabBar_->setMovable(movable); }

    bool tabBarAutoHide() const { return tabBar_->autoHide(); }
    void setTabBarAutoHide(bool enabled) { tabBar_->setAutoHide(enabled); }

    QTabBar::Shape tabShape() const { return tabBar_->shape(); }
    void setTabShape(QTabBar::Shape shape) { tabBar_->setShape(shape); }

    bool usesScrollButtons() const { return tabBar_->usesScrollButtons(); }
    void setUsesScrollButtons(bool useButtons)
    {
        tabBar_->setUsesScrollButtons(useButtons);
    }

    bool isTabEnabled(int index) const { return tabBar_->isTabEnabled(index); }
    void setTabEnabled(int index, bool enabled)
    {
        tabBar_->setTabEnabled(index, enabled);
    }

    QString tabText(int index) const { return tabBar_->tabText(index); }
    void setTabText(int index, const QString& text)
    {
        tabBar_->setTabText(index, text);
    }

signals:
    void tabBarClicked(int index);
    void tabBarDoubleClicked(int index);
    void tabMoved(int from, int to);

private:
    void initializeTabBarPropagatedSignals_()
    {
        connect(tabBar_, &QTabBar::tabBarClicked, this, [&](int index) {
            emit tabBarClicked(index);
        });

        connect(tabBar_, &QTabBar::tabBarDoubleClicked, this, [&](int index) {
            emit tabBarDoubleClicked(index);
        });

        connect(tabBar_, &QTabBar::tabMoved, this, [&](int from, int to) {
            emit tabMoved(from, to);
        });
    }

    /// *** TAB DRAGGING *** ///

    // Current issues:
    // - Ideally, we want visual feedback for the drag (a moving gap showing the
    //   drop area where the in-progress dragged tab will go, and the gap will
    //   follow the drag as long as it is over top of the tab bar)
    // - We also should place the tab at the index it was dragged to, not just
    //   at the end of the target tab bar
    // - Pixmap is blurry. Maybe SVG somehow
    // - Icon for desktop drag should not be "no"
    // - Icon for another Workspace should be "no" if possible...

public:
    using DragValidator =
        std::function<bool(TabWidget* source, TabWidget* destination)>;

    struct Location
    {
        QPointer<TabWidget> tabWidget = nullptr;
        int index = -1;
        bool isValid() const noexcept { return tabWidget && index > -1; }
    };

    struct TabSpec
    {
        QWidget* widget = nullptr;
        QVariant userData{};
        QString text{};
        QString toolTip{};
        bool isFlagged = false;
        QPointF offsetRatios{};

        static QPointF getOffsetRatios(const QRect& rect, const QPoint& pos)
        {
            auto w = rect.width();
            auto h = rect.height();

            return { w > 0 ? (qreal)pos.x() / w : 0.0,
                     h > 0 ? (qreal)pos.y() / h : 0.0 };
        }

        QPoint relPos(int width, int height) const noexcept
        {
            // Seems a little off (mouse is lower on the pixmap than it should
            // be), but it'll have to do
            QPointF pf(width * offsetRatios.x(), height * offsetRatios.y());
            return pf.toPoint();
        }

        bool isValid() const noexcept { return widget; }
    };

    DragValidator dragValidator() const noexcept { return dragValidator_; }
    void setDragValidator(const DragValidator& validator)
    {
        dragValidator_ = validator;
    }

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

    bool tabsDraggable() const noexcept { return tabsDraggable_; }

    void setTabsDraggable(bool draggable)
    {
        if (tabsDraggable_ == draggable) return;
        tabsDraggable_ = draggable;
        setAcceptDrops(draggable);
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        // Handle mouse events on the tab bar for dragging
        if (tabsDraggable_ && watched == tabBar_) {
            if (event->type() == QEvent::MouseButtonPress) {
                auto mouse_event = static_cast<QMouseEvent*>(event);

                if (mouse_event->button() == Qt::LeftButton)
                    dragStartPosition_ = mouse_event->pos();
            } else if (event->type() == QEvent::MouseMove) {
                auto mouse_event = static_cast<QMouseEvent*>(event);

                if (mouse_event->buttons() & Qt::LeftButton) {
                    auto delta = mouse_event->pos() - dragStartPosition_;

                    // Only start drag if we've moved far enough VERTICALLY.
                    // This allows horizontal movement to use the tab bar's
                    // natural reordering

                    // This works well, may want to increase required distance.
                    if (qAbs(delta.y())
                        >= QApplication::startDragDistance() * 1.5) {
                        auto index = tabBar_->tabAt(dragStartPosition_);

                        if (index > -1) {
                            startDrag_(index);
                            return true; // Prevent further processing.
                        }
                    }
                }
            }
        }

        return QWidget::eventFilter(watched, event);
    }

signals:
    void tabDragged(const Location& old, const Location& now);
    void tabDraggedToDesktop(
        TabWidget* source,
        const QPoint& dropPos,
        const TabSpec& tabSpec);

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event) override
    {
        event->mimeData()->hasFormat(MIME_TYPE_) ? event->acceptProposedAction()
                                                 : event->ignore();
    }

    virtual void dragMoveEvent(QDragMoveEvent* event) override
    {
        event->mimeData()->hasFormat(MIME_TYPE_) ? event->acceptProposedAction()
                                                 : event->ignore();
    }

    virtual void dropEvent(QDropEvent* event) override
    {
        auto mime_data = event->mimeData();

        if (!mime_data->hasFormat(MIME_TYPE_)) {
            event->ignore();
            return;
        }

        // Extract data from the mime data
        auto item_data = mime_data->data(MIME_TYPE_);
        auto tab_assembly = deserialize_(item_data);

        // Validate the extracted data
        if (!tab_assembly.isValid()) {
            event->ignore();
            return;
        }

        // Check with validator if drag should be allowed
        if (dragValidator_ && !dragValidator_(tab_assembly.origin, this)) {
            event->ignore();
            return;
        }

        // Drop and notify about successful drag
        auto new_index = addDroppedTab_(tab_assembly.tabSpec);
        emit tabDragged(
            { tab_assembly.origin, tab_assembly.originIndex },
            { this, new_index });
        event->acceptProposedAction();
    }

private:
    static constexpr auto MIME_TYPE_ = "application/x-fernanda-tab-widget-tab";
    DragValidator dragValidator_ = nullptr;
    bool tabsDraggable_ = false;
    QPoint dragStartPosition_{};

    struct TabDragContext_
    {
        TabWidget* origin = nullptr;
        int originIndex = -1;
        TabSpec tabSpec{};

        bool isValid() const noexcept
        {
            return origin && originIndex > -1 && tabSpec.isValid();
        }
    };

    void startDrag_(int index)
    {
        auto widget = widgetAt(index);
        if (!widget) return;

        auto drag = new QDrag(this);
        auto mime_data = new QMimeData;

        auto tab_rect = tabBar_->tabRect(index);
        auto offset_within_tab = dragStartPosition_ - tab_rect.topLeft();

        // Store tab data BEFORE removing
        TabDragContext_ drag_context{
            this,
            index,

            TabSpec{ widget,
                     tabData(index),
                     tabText(index),
                     tabToolTip(index),
                     tabFlagged(index),
                     TabSpec::getOffsetRatios(tab_rect, offset_within_tab) }
        };

        // Store necessary information
        mime_data->setData(MIME_TYPE_, serialize_(drag_context));
        drag->setMimeData(mime_data);

        // Remove the tab before dragging
        removeTab(index);

        // Visual representation
        auto pixmap = dragPixmap_(drag_context.tabSpec.text);
        drag->setPixmap(pixmap);
        drag->setHotSpot(
            drag_context.tabSpec.relPos(pixmap.width(), pixmap.height()));

        // Works to hide the "no" sign on valid desktop drags, but not having a
        // cursor at all is bad
        // QPixmap transparent_pixmap(1, 1);
        // transparent_pixmap.fill(Qt::transparent);
        // drag->setDragCursor(transparent_pixmap, Qt::IgnoreAction);
        // drag->setDragCursor(transparent_pixmap, Qt::MoveAction);
        // drag->setDragCursor(transparent_pixmap, Qt::CopyAction);
        // drag->setDragCursor(transparent_pixmap, Qt::LinkAction);

        // Execute
        switch (drag->exec(Qt::MoveAction)) {
        case Qt::DropAction::IgnoreAction:
            if (isDesktopDrop_()) {
                emit tabDraggedToDesktop(
                    this,
                    QCursor::pos(),
                    drag_context.tabSpec);
                break;
            }

            [[fallthrough]]; // Else, maybe fail

            // Fail states
        default:
        case Qt::DropAction::CopyAction:
        case Qt::DropAction::LinkAction:
        case Qt::DropAction::TargetMoveAction:
            addDroppedTab_(drag_context.tabSpec);
            break;

            // Success state (handled by target TabWidget)
        case Qt::DropAction::MoveAction:
            break;
        }
    }

    QByteArray serialize_(const TabDragContext_& dragContext)
    {
        if (!dragContext.isValid()) return {};

        QByteArray data{};
        QDataStream data_stream(&data, QIODevice::WriteOnly);

        data_stream << quintptr(dragContext.origin) << dragContext.originIndex
                    << quintptr(dragContext.tabSpec.widget)
                    << dragContext.tabSpec.userData << dragContext.tabSpec.text
                    << dragContext.tabSpec.toolTip
                    << dragContext.tabSpec.isFlagged
                    << dragContext.tabSpec.offsetRatios;

        return data;
    }

    TabDragContext_ deserialize_(QByteArray& data)
    {
        QDataStream data_stream(&data, QIODevice::ReadOnly);

        quintptr origin{};
        int origin_index = -1;
        quintptr widget{};
        QVariant user_data{};
        QString text{};
        QString tool_tip{};
        bool is_flagged = false;
        QPointF offset_ratio{};

        data_stream >> origin >> origin_index >> widget >> user_data >> text
            >> tool_tip >> is_flagged >> offset_ratio;

        return { reinterpret_cast<TabWidget*>(origin),
                 origin_index,

                 TabSpec{ reinterpret_cast<QWidget*>(widget),
                          user_data,
                          text,
                          tool_tip,
                          is_flagged,
                          offset_ratio } };
    }

    int addDroppedTab_(const TabSpec& tabSpec)
    {
        auto new_index = addTab(tabSpec.widget, tabSpec.text);
        setTabData(new_index, tabSpec.userData);
        setTabToolTip(new_index, tabSpec.toolTip);
        setTabFlagged(new_index, tabSpec.isFlagged);
        setCurrentIndex(new_index);
        return new_index;
    }

    bool isDesktopDrop_() const
    {
        auto final_pos = QCursor::pos();

        // Check if mouse is over any application window
        for (auto& widget : QApplication::topLevelWidgets())
            if (auto window = qobject_cast<QWidget*>(widget))
                if (window->isVisible()
                    && window->geometry().contains(final_pos))
                    return false; // Mouse is over an application window

        return true; // Mouse is outside all application windows
    }

    QPixmap dragPixmap_(const QString& tabText) const
    {
        constexpr auto corner_radius = 4;
        constexpr auto pad = 5;

        QPixmap pixmap(155, 35);
        pixmap.fill(Qt::transparent);

        auto font = QFont("Arial", 10, 600);
        QFontMetrics font_metrics(font);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setFont(font);

        auto main_rect = pixmap.rect();

        // Add Bezel
        constexpr auto bezel_width = 2;
        QPainterPath bezel_path{};
        bezel_path.addRoundedRect(main_rect, corner_radius, corner_radius);

        QLinearGradient bezel_border_gradient(
            main_rect.topLeft(),
            main_rect.bottomRight());
        bezel_border_gradient.setColorAt(
            0.0,
            QColor(190, 190, 190, 180)); // Light
        bezel_border_gradient.setColorAt(
            1.0,
            QColor(125, 125, 125, 180)); // Dark
        painter.fillPath(bezel_path, bezel_border_gradient);

        // Content area (inset from bezel border)
        auto content_rect = main_rect.adjusted(
            bezel_width,
            bezel_width,
            -bezel_width,
            -bezel_width);

        // Content gradient
        QLinearGradient content_gradient(
            content_rect.topLeft(),
            content_rect.bottomLeft());
        content_gradient.setColorAt(
            0.0,
            QColor(65, 65, 65, 220)); // Lighter at top
        content_gradient.setColorAt(
            0.05,
            QColor(55, 55, 55, 210)); // Top highlight
        content_gradient.setColorAt(0.5, QColor(35, 35, 35, 200)); // Middle
        content_gradient.setColorAt(
            0.95,
            QColor(20, 20, 20, 190)); // Bottom shadow
        content_gradient.setColorAt(
            1.0,
            QColor(15, 15, 15, 180)); // Darkest at bottom

        QPainterPath content_path{};
        content_path.addRoundedRect(
            content_rect,
            corner_radius - 1,
            corner_radius - 1);
        painter.fillPath(content_path, content_gradient);

        // Top shine
        static constexpr auto gradient_height = 4;
        static constexpr auto inset = 1;
        static constexpr auto total_height = 5;
        static constexpr auto radius_reduction = 2;
        static constexpr auto color =
            QColor(100, 100, 100, 100); // was 85, 85, 85, 80

        QLinearGradient highlight_gradient(
            content_rect.topLeft(),
            QPoint(content_rect.left(), content_rect.top() + gradient_height));
        highlight_gradient.setColorAt(0.0, color);
        highlight_gradient.setColorAt(1.0, Qt::transparent);

        QPainterPath highlight_path{};
        auto highlight_rect = content_rect.adjusted(
            inset,
            inset,
            -inset,
            -content_rect.height() + total_height);
        highlight_path.addRoundedRect(
            highlight_rect,
            corner_radius - radius_reduction,
            corner_radius - radius_reduction);
        painter.fillPath(highlight_path, highlight_gradient);

        // Border
        painter.setPen(QPen(QColor(80, 80, 80, 120), 1)); // Was 0.5
        painter.drawPath(content_path);

        // Draw text
        painter.setPen(Qt::white);
        auto text_rect = content_rect.adjusted(pad, pad, -pad, -pad);
        auto elided_text =
            font_metrics.elidedText(tabText, Qt::ElideRight, text_rect.width());
        painter.drawText(text_rect, Qt::AlignCenter, elided_text);

        return pixmap;
    }
};

} // namespace Fernanda
