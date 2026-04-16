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

#include "ui/TabWidget.h"

#include <QColor>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDataStream>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLinearGradient>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QVBoxLayout>

#include "core/Application.h"
#include "core/Debug.h"
#include "modules/StyleContext.h"
#include "ui/Icons.h"
#include "ui/TabWidgetAlertWidget.h"
#include "ui/TabWidgetCloseButton.h"

namespace Fernanda {

bool TabWidget::Location::isValid() const noexcept
{
    return tabWidget && index > -1;
}

QPointF
TabWidget::TabSpec::getOffsetRatios(const QRect& rect, const QPoint& pos)
{
    auto w = rect.width();
    auto h = rect.height();

    return { w > 0 ? static_cast<qreal>(pos.x()) / w : 0.0,
             h > 0 ? static_cast<qreal>(pos.y()) / h : 0.0 };
}

QPoint TabWidget::TabSpec::relPos(int width, int height) const noexcept
{
    // Seems a little off (mouse is lower on the pixmap than it should
    // be), but it'll have to do
    QPointF pf(width * offsetRatios.x(), height * offsetRatios.y());
    return pf.toPoint();
}

// --- Queries ---

bool TabWidget::TabSpec::isValid() const noexcept { return widget; }

TabWidget::TabWidget(QWidget* parent)
    : QWidget(parent)
{
    setup_();
}

TabWidget::~TabWidget() { TRACER; }

void TabWidget::activatePrevious()
{
    auto count = tabBar_->count();
    if (count <= 1) return;

    auto i = tabBar_->currentIndex();
    auto previous = (i - 1 + count) % count;
    setCurrentIndex(previous);
}

void TabWidget::activateNext()
{
    auto count = tabBar_->count();
    if (count <= 1) return;

    auto i = tabBar_->currentIndex();
    auto next = (i + 1) % count;
    setCurrentIndex(next);
}

bool TabWidget::eventFilter(QObject* watched, QEvent* event)
{
    // TODO: Notate this better!

    // Handle mouse events on the tab bar for dragging
    if (tabsDraggable_ && watched == tabBar_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event->button() == Qt::LeftButton) {
                dragStartPosition_ = mouse_event->pos();
                dragPressIndex_ = tabBar_->tabAt(dragStartPosition_);
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto mouse_event = static_cast<QMouseEvent*>(event);

            if (mouse_event->buttons() & Qt::LeftButton) {
                auto delta = mouse_event->pos() - dragStartPosition_;

                // Only start drag if we've moved far enough VERTICALLY.
                // This allows horizontal movement to use the tab bar's
                // natural reordering

                // This works well, may want to increase required distance.
                if (qAbs(delta.y()) >= Application::startDragDistance() * 1.5) {
                    tabBar_->tabAt(dragStartPosition_);

                    if (dragPressIndex_ > -1) {
                        startDrag_(dragPressIndex_);
                        dragPressIndex_ = -1;
                        return true;
                    }
                }
            }
        }
    }

    if (watched == tabBar_ && event->type() == QEvent::ContextMenu) {
        auto context_event = static_cast<QContextMenuEvent*>(event);
        auto index = tabBar_->tabAt(context_event->pos());

        if (index >= 0) {
            emit tabContextMenuRequested(index, context_event->globalPos());
        }

        return true;
    }

    if (watched == addButton_ && event->type() == QEvent::ContextMenu) {
        auto context_event = static_cast<QContextMenuEvent*>(event);
        emit addButtonContextMenuRequested(context_event->globalPos());
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

bool TabWidget::isEmpty() const { return tabBar_->count() < 1; }

int TabWidget::count() const { return tabBar_->count(); }

int TabWidget::currentIndex() const { return tabBar_->currentIndex(); }

QWidget* TabWidget::currentWidget() const { return widgetAt(currentIndex()); }

int TabWidget::indexOf(const QWidget* widget) const
{
    for (auto i = 0; i < tabBar_->count(); ++i)
        if (widgetAt(i) == widget) return i;

    return -1;
}

QWidget* TabWidget::widgetAt(int index) const
{
    auto data = tabBar_->tabData(index);
    return qvariant_cast<QWidget*>(data);
}

QList<QWidget*> TabWidget::widgets() const
{
    QList<QWidget*> result{};

    for (auto i = 0; i < tabBar_->count(); ++i)
        if (auto widget = widgetAt(i)) result << widget;

    return result;
}

// --- Tabs ---

TabWidget::TabSpec TabWidget::tabSpecAt(int index) const
{
    return { widgetAt(index),   tabData(index),         tabText(index),
             tabToolTip(index), tabAlertMessage(index), tabFlagged(index) };
}

int TabWidget::addTab(QWidget* widget, const QString& tabText)
{
    return addOrInsertTab_(-1, widget, tabText);
}

int TabWidget::addTab(const TabSpec& tabSpec)
{
    auto index = addTab(tabSpec.widget, tabSpec.text);

    setTabData(index, tabSpec.userData);
    setTabToolTip(index, tabSpec.toolTip);
    setTabFlagged(index, tabSpec.isFlagged);

    if (!tabSpec.alertMessage.isEmpty()) {
        setTabAlert(index, tabSpec.alertMessage);
    }

    return index;
}

int TabWidget::insertTab(int index, QWidget* widget, const QString& tabText)
{
    return addOrInsertTab_(index, widget, tabText);
}

QWidget* TabWidget::removeTab(int index)
{
    auto widget = widgetAt(index);
    auto button = closeButtonAt_(index);
    auto alert = alertWidgetAt_(index);

    tabBar_->removeTab(index);

    if (button) {
        closeButtons_.removeAll(button);
        delete button;
    }

    if (alert) delete alert;

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

QWidgetList TabWidget::clear()
{
    QWidgetList widgets{};
    while (!isEmpty())
        widgets << removeTab(0);
    return widgets;
}

QVariant TabWidget::tabData(int index) const
{
    return tabUserData_[widgetAt(index)];
}

void TabWidget::setTabData(int index, const QVariant& data)
{
    tabUserData_[widgetAt(index)] = data;
}

bool TabWidget::tabFlagged(int index) const
{
    if (auto button = closeButtonAt_(index)) return button->flagged();
    return false;
}

void TabWidget::setTabFlagged(int index, bool flagged)
{
    if (auto button = closeButtonAt_(index)) button->setFlagged(flagged);
}

bool TabWidget::tabAlert(int index) const { return alertWidgetAt_(index); }

QString TabWidget::tabAlertMessage(int index) const
{
    if (auto alert = alertWidgetAt_(index)) return alert->message();
    return {};
}

void TabWidget::setTabAlert(int index, const QString& message)
{
    if (index < 0 || index >= tabBar_->count()) return;

    clearTabAlert(index);
    if (message.isEmpty()) return;

    auto alert = new TabWidgetAlertWidget(message, tabBar_);
    tabBar_->setTabButton(index, QTabBar::LeftSide, alert);
}

void TabWidget::clearTabAlert(int index)
{
    if (auto alert = alertWidgetAt_(index)) {
        tabBar_->setTabButton(index, QTabBar::LeftSide, nullptr);
        delete alert;
    }
}

QIcon TabWidget::tabIcon(int index) const { return tabBar_->tabIcon(index); }

void TabWidget::setTabIcon(int index, const QIcon& icon)
{
    tabBar_->setTabIcon(index, icon);
}

QString TabWidget::tabToolTip(int index) const
{
    return tabBar_->tabToolTip(index);
}

void TabWidget::setTabToolTip(int index, const QString& tip)
{
    tabBar_->setTabToolTip(index, tip);
}

bool TabWidget::isTabEnabled(int index) const
{
    return tabBar_->isTabEnabled(index);
}

void TabWidget::setTabEnabled(int index, bool enabled)
{
    tabBar_->setTabEnabled(index, enabled);
}

QString TabWidget::tabText(int index) const { return tabBar_->tabText(index); }

void TabWidget::setTabText(int index, const QString& text)
{
    tabBar_->setTabText(index, text);
}

QRect TabWidget::tabRect(int index) const { return tabBar_->tabRect(index); }

// --- Tab bar ---

bool TabWidget::tabsClosable() const noexcept { return tabsClosable_; }

void TabWidget::setTabsClosable(bool closable)
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

QSize TabWidget::minimumTabSize() const { return tabBar_->minimumTabSize(); }

QSize TabWidget::maximumTabSize() const { return tabBar_->maximumTabSize(); }

Qt::TextElideMode TabWidget::elideMode() const { return tabBar_->elideMode(); }

void TabWidget::setElideMode(Qt::TextElideMode mode)
{
    tabBar_->setElideMode(mode);
}

bool TabWidget::isMovable() const { return tabBar_->isMovable(); }

void TabWidget::setMovable(bool movable) { tabBar_->setMovable(movable); }

bool TabWidget::tabBarAutoHide() const { return tabBar_->autoHide(); }

void TabWidget::setTabBarAutoHide(bool enabled)
{
    tabBar_->setAutoHide(enabled);
}

QTabBar::Shape TabWidget::tabShape() const { return tabBar_->shape(); }

void TabWidget::setTabShape(QTabBar::Shape shape) { tabBar_->setShape(shape); }

bool TabWidget::usesScrollButtons() const
{
    return tabBar_->usesScrollButtons();
}

void TabWidget::setUsesScrollButtons(bool useButtons)
{
    tabBar_->setUsesScrollButtons(useButtons);
}

// --- Tab dragging ---

TabWidget::DragValidator TabWidget::dragValidator() const noexcept
{
    return dragValidator_;
}

void TabWidget::setDragValidator(const DragValidator& validator)
{
    dragValidator_ = validator;
}

bool TabWidget::tabsDraggable() const noexcept { return tabsDraggable_; }

void TabWidget::setTabsDraggable(bool draggable)
{
    if (tabsDraggable_ == draggable) return;
    tabsDraggable_ = draggable;
    setAcceptDrops(draggable);
}

// --- Public slots ---

void TabWidget::setCurrentIndex(int index) { tabBar_->setCurrentIndex(index); }

void TabWidget::setCurrentWidget(QWidget* widget)
{
    setCurrentIndex(indexOf(widget));
}

// --- Protected ---

void TabWidget::tabInserted([[maybe_unused]] int index) {}

void TabWidget::tabRemoved([[maybe_unused]] int index) {}

void TabWidget::dragEnterEvent(QDragEnterEvent* event)
{
    event->mimeData()->hasFormat(MIME_TYPE_) ? event->acceptProposedAction()
                                             : event->ignore();
}

void TabWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event->mimeData()->hasFormat(MIME_TYPE_)) {
        event->ignore();
        return;
    }

    /// TODO TS
    auto zone = dropZone_(event->position().toPoint());
    zone != DropZone_::Passthrough ? event->acceptProposedAction()
                                   : event->ignore();
}

void TabWidget::dropEvent(QDropEvent* event)
{
    auto mime_data = event->mimeData();

    if (!mime_data->hasFormat(MIME_TYPE_)) {
        event->ignore();
        return;
    }

    // Extract data from the mime data
    auto item_data = mime_data->data(MIME_TYPE_);
    auto tab_assembly = deserialize_(item_data);

    if (!tab_assembly.isValid()) {
        event->ignore();
        return;
    }

    // Check with validator if drag should be allowed
    if (dragValidator_ && !dragValidator_(tab_assembly.origin, this)) {
        event->ignore();
        return;
    }

    /// TODO TS
    switch (dropZone_(event->position().toPoint())) {
    case DropZone_::TabBar: {
        auto new_index = addTab(tab_assembly.tabSpec);
        setCurrentIndex(new_index);
        emit tabDragged(
            { tab_assembly.origin, tab_assembly.originIndex },
            { this, new_index });
        event->acceptProposedAction();
        break;
    }

    case DropZone_::SplitLeft:
        emit tabDraggedToSplitEdge(
            tab_assembly.origin,
            this,
            tab_assembly.tabSpec,
            SplitSide::Left);
        event->acceptProposedAction();
        break;

    case DropZone_::SplitRight:
        emit tabDraggedToSplitEdge(
            tab_assembly.origin,
            this,
            tab_assembly.tabSpec,
            SplitSide::Right);
        event->acceptProposedAction();
        break;

    case DropZone_::Passthrough:
        event->ignore();
        break;
    }
}

// --- Private ---

bool TabWidget::TabDragContext_::isValid() const noexcept
{
    return origin && originIndex > -1 && tabSpec.isValid();
}

void TabWidget::setup_()
{
    setAcceptDrops(tabsDraggable_);

    mainStack_->addWidget(underlay_);
    mainStack_->addWidget(widgetStack_);

    tabBar_->installEventFilter(this);
    tabBar_->setFixedHeight(TAB_BAR_HEIGHT_);
    tabBar_->setMaximumTabWidth(MIN_TAB_WIDTH_);
    tabBar_->setMaximumTabWidth(MAX_TAB_WIDTH_);

    addButton_->installEventFilter(this);
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
    connect(addButton_, &TabWidgetButton::clicked, this, [this] {
        emit addTabRequested();
    });

    connect(
        tabBar_,
        &TabWidgetTabBar::currentChanged,
        this,
        &TabWidget::onTabBarCurrentChanged_);

    connect(
        widgetStack_,
        &WidgetStack_::currentChanged,
        this,
        &TabWidget::onWidgetStackCurrentChanged_);

    initializeTabBarPropagatedSignals_();
}

void TabWidget::initializeTabBarPropagatedSignals_()
{
    connect(tabBar_, &QTabBar::tabBarClicked, this, [this](int index) {
        emit tabBarClicked(index);
    });

    connect(tabBar_, &QTabBar::tabBarDoubleClicked, this, [this](int index) {
        emit tabBarDoubleClicked(index);
    });

    connect(tabBar_, &QTabBar::tabMoved, this, [this](int from, int to) {
        if (dragPressIndex_ == from)
            dragPressIndex_ = to;
        else if (from < dragPressIndex_ && to >= dragPressIndex_)
            --dragPressIndex_;
        else if (from > dragPressIndex_ && to <= dragPressIndex_)
            ++dragPressIndex_;

        emit tabMoved(from, to);
    });
}

int
TabWidget::addOrInsertTab_(int index, QWidget* widget, const QString& tabText)
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

void TabWidget::addCloseButtonAt_(int index)
{
    if (index < 0 || index >= tabBar_->count()) return;

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
        [this](int i) { emit closeTabRequested(i); });
}

TabWidgetCloseButton* TabWidget::closeButtonAt_(int index) const
{
    auto button = tabBar_->tabButton(index, QTabBar::RightSide);
    return qobject_cast<TabWidgetCloseButton*>(button);
}

void TabWidget::updateMouseHoverAfterLayoutChange_()
{
    // Get current global mouse position
    auto global_mouse_pos = QCursor::pos();

    // Find which widget is actually under the mouse now
    auto widget_under_mouse = Application::widgetAt(global_mouse_pos);
    auto close_button_under_mouse =
        qobject_cast<TabWidgetCloseButton*>(widget_under_mouse);

    // Update all close buttons' hover states
    for (auto& button : closeButtons_) {
        if (!button) continue;

        if (button == close_button_under_mouse) {
            // This button should be in hover state but might not be
            if (!button->underMouse()) {
                auto local_pos = button->mapFromGlobal(global_mouse_pos);
                QEnterEvent enter_event(local_pos, local_pos, global_mouse_pos);
                Application::sendEvent(button, &enter_event);
            }
        } else {
            // This button should not be in hover state
            if (button->underMouse()) {
                QEvent leave_event(QEvent::Leave);
                Application::sendEvent(button, &leave_event);
            }
        }
    }
}

TabWidgetAlertWidget* TabWidget::alertWidgetAt_(int index) const
{
    auto widget = tabBar_->tabButton(index, QTabBar::LeftSide);
    return qobject_cast<TabWidgetAlertWidget*>(widget);
}

void TabWidget::startDrag_(int index)
{
    auto widget = widgetAt(index);
    if (!widget) return;

    auto drag = new QDrag(this);
    auto mime_data = new QMimeData;

    auto tab_rect = tabBar_->tabRect(index);
    auto offset_within_tab =
        QPoint(tab_rect.width() / 2, tab_rect.height() / 2);

    // Store tab data BEFORE removing
    auto spec = tabSpecAt(index);
    spec.offsetRatios = TabSpec::getOffsetRatios(tab_rect, offset_within_tab);

    TabDragContext_ drag_context{ this, index, spec };

    // Store necessary information
    mime_data->setData(MIME_TYPE_, serialize_(drag_context));
    drag->setMimeData(mime_data);

    /// TODO TS: Consider DragScrop_ RAII guard
    // dragStarted/dragEnded bracket the entire drag lifecycle. drag->exec()
    // runs a modal event loop, so the target's dropEvent (which may emit
    // tabDraggedToSplitEdge and create new splits) completes synchronously
    // before exec() returns. This guarantees suppressAutoCollapse is active
    // throughout: remove the tab here (possibly leaving source empty), the
    // drop creates new splits or windows, then dragEnded runs cleanup
    emit dragStarted();

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

    /// TODO TD:
    // Execute
    auto result = drag->exec(Qt::MoveAction);
    auto target = qobject_cast<TabWidget*>(drag->target());

    if (result != Qt::DropAction::MoveAction || !target) {
        // Drop was either rejected or accepted by something outside
        // Fernanda (e.g., a browser). Open in a new window.
        emit tabDraggedOutside(this, QCursor::pos(), drag_context.tabSpec);
    }

    /// TODO TS
    emit dragEnded();
}

QByteArray TabWidget::serialize_(const TabDragContext_& dragContext)
{
    if (!dragContext.isValid()) return {};

    QByteArray data{};
    QDataStream data_stream(&data, QIODevice::WriteOnly);

    data_stream << quintptr(dragContext.origin) << dragContext.originIndex
                << quintptr(dragContext.tabSpec.widget)
                << dragContext.tabSpec.userData << dragContext.tabSpec.text
                << dragContext.tabSpec.toolTip
                << dragContext.tabSpec.alertMessage
                << dragContext.tabSpec.isFlagged
                << dragContext.tabSpec.offsetRatios;

    return data;
}

TabWidget::TabDragContext_ TabWidget::deserialize_(QByteArray& data)
{
    QDataStream data_stream(&data, QIODevice::ReadOnly);

    quintptr origin{};
    int origin_index = -1;
    quintptr widget{};
    QVariant user_data{};
    QString text{};
    QString tool_tip{};
    QString alert_message{};
    bool is_flagged = false;
    QPointF offset_ratio{};

    data_stream >> origin >> origin_index >> widget >> user_data >> text
        >> tool_tip >> alert_message >> is_flagged >> offset_ratio;

    return { reinterpret_cast<TabWidget*>(origin),
             origin_index,

             TabSpec{ reinterpret_cast<QWidget*>(widget),
                      user_data,
                      text,
                      tool_tip,
                      alert_message,
                      is_flagged,
                      offset_ratio } };
}

QPixmap TabWidget::dragPixmap_(const QString& tabText) const
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
    bezel_border_gradient.setColorAt(0.0, QColor(190, 190, 190, 180)); // Light
    bezel_border_gradient.setColorAt(1.0, QColor(125, 125, 125, 180)); // Dark
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
    content_gradient.setColorAt(0.0, QColor(65, 65, 65, 220)); // Lighter at top
    content_gradient.setColorAt(0.05, QColor(55, 55, 55, 210)); // Top highlight
    content_gradient.setColorAt(0.5, QColor(35, 35, 35, 200)); // Middle
    content_gradient.setColorAt(0.95, QColor(20, 20, 20, 190)); // Bottom shadow
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

/// TODO TS
TabWidget::DropZone_ TabWidget::dropZone_(const QPoint& pos) const
{
    auto pos_in_tab_bar = tabBar_->mapFrom(this, pos);
    if (tabBar_->rect().contains(pos_in_tab_bar)) return DropZone_::TabBar;

    // Content area (below tab bar)
    auto content_rect = rect().adjusted(0, tabBar_->height(), 0, 0);
    if (!content_rect.contains(pos)) return DropZone_::Passthrough;

    auto edge_width = content_rect.width() / 4;

    if (pos.x() < content_rect.left() + edge_width) return DropZone_::SplitLeft;
    if (pos.x() > content_rect.right() - edge_width) {
        return DropZone_::SplitRight;
    }

    return DropZone_::Passthrough;
}

// --- Private slots ---

void TabWidget::onTabBarCurrentChanged_(int index)
{
    if (auto widget = widgetAt(index)) {
        widgetStack_->setCurrentWidget(widget);
        setFocusProxy(widget);
    } else {
        setFocusProxy(nullptr);
    }

    emit currentChanged(index);
}

void TabWidget::onWidgetStackCurrentChanged_(
    [[maybe_unused]] int index // We do NOT use widget stack index for anything!
)
{
    // We do NOT use widget stack index for anything!
    widgetStack_->count() ? mainStack_->setCurrentWidget(widgetStack_)
                          : mainStack_->setCurrentWidget(underlay_);
}

} // namespace Fernanda
