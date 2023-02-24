/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Splitter.cpp, Fernanda

#include "Splitter.h"

Splitter::Splitter(QWidget* parent)
    : QSplitter(parent)
{
    setObjectName(QStringLiteral("splitter"));
    connect(this, &Splitter::splitterMoved, this, &Splitter::checkStates);
}

const QStringList Splitter::devRecordStartUpSizes()
{
    auto widget_sizes = sizes();
    QStringList result;
    auto window_size = askWindowSize();
    auto window_entry = QString("Window size on startup: ");
    window_entry.append(QString::number(window_size.height()));
    window_entry.append(", " + QString::number(window_size.width()));
    result << window_entry;
    for (auto i = 0; i < count(); ++i)
    {
        auto entry = QString("Widget index " + QString::number(i) + ": ");
        entry.append(QString::number(widget(i)->height()));
        entry.append(", " + QString::number(widget_sizes.at(i)));
        result << entry;
    }
    return result;
}

const QStringList Splitter::devGetStates()
{
    QStringList result;
    result << "(-1 = unset)";
    for (auto& widget_info : widgets)
    {
        auto entry = "Widget index: " + QString::number(widget_info.index);
        entry.append("\nAssociated handle index: " + QString::number(widget_info.handleIndex));
        entry.append("\nLast saved width: " + QString::number(widget_info.width));
        QString state = isCollapsed(widget_info) ? "Collapsed" : isExpanded(widget_info) ? "Expanded" : "HoverExpanded";
        entry.append("\nCurrent state: " + state);
        result << entry;
    }
    return result;
}

void Splitter::addWidgets(QVector<QWidget*> widgets)
{
    for (auto& widget : widgets)
        addWidget(widget);
    setCollapsible(pane.index, true);
    setCollapsible(editor.index, false);
    setCollapsible(preview.index, true);
    setStretchFactor(pane.index, 0);
    setStretchFactor(editor.index, 1);
    setStretchFactor(preview.index, 1);
    widgets.at(0)->installEventFilter(this);
    widgets.at(2)->installEventFilter(this);
}

void Splitter::saveConfig()
{
    uncollapseAll();
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::SplitterPosition, saveState());
}

void Splitter::loadConfig()
{
    auto state = UserData::loadConfig(UserData::IniGroup::Window, UserData::IniValue::SplitterPosition, QVariant()).toByteArray();
    if (state.isEmpty() || state.isNull())
    {
        auto width = askWindowSize().width();
        setSizes(QVector<int>{ width * 2/10, width * 4/10, width * 4/10 });
    }
    else
        restoreState(state);
    setHandleWidth(6);
    devStartUpSizes = devRecordStartUpSizes();
}

SplitterHandle* Splitter::createHandle()
{
    auto handle = new SplitterHandle(orientation(), this);
    connect(handle, &SplitterHandle::askHoverExpand, this, &Splitter::hoverExpand);
    connect(handle, &SplitterHandle::askStoreWidths, this, &Splitter::storeWidths);
    connect(handle, &SplitterHandle::askToggleExpansion, this, &Splitter::toggleExpansion);
    connect(handle, &SplitterHandle::askUnhoverAll, this, &Splitter::unhoverAll);
    return handle;
}

int Splitter::toDefault(int index)
{
    auto window_width = askWindowSize().width();
    return (index < 2) ? (window_width * 2/10) : (window_width - (window_width * 4/10));
}

void Splitter::collapse(Info& widgetInfo)
{
    widgetInfo.state = State::Collapsed;
    auto& handle_index = widgetInfo.handleIndex;
    moveSplitter(toWindowX(handle_index, 0), handle_index);
}

void Splitter::expand(Info& widgetInfo, bool isHover)
{
    isHover ? widgetInfo.state = State::HoverExpanded : widgetInfo.state = State::Expanded;
    auto& handle_index = widgetInfo.handleIndex;
    auto& stored_width = widgetInfo.width;
    (stored_width < 1)
        ? moveSplitter(toDefault(handle_index), handle_index)
        : moveSplitter(toWindowX(handle_index, stored_width), handle_index);
}

void Splitter::uncollapseAll()
{
    for (auto& widget_info : widgets)
    {
        if (!isCollapsed(widget_info)) continue;
        auto& handle_index = widget_info.handleIndex;
        auto& stored_width = widget_info.width;
        if (stored_width != -1)
            moveSplitter(toWindowX(handle_index, stored_width), handle_index);
    }
}

bool Splitter::eventFilter(QObject* watched, QEvent* event)
{
    QWidget* object = qobject_cast<QWidget*>(watched);
    if (!object) return false;
    if (event->type() == QEvent::Leave)
    {
        unhoverAll();
        return true;
    }
    return false;
}

void Splitter::initialize()
{
    for (auto& widget_info : widgets)
    {
        if (widget_info.width) continue;
        widget_info.state = State::Collapsed;
        isInitialized = true;
    }
}

void Splitter::checkStates(int position, int index)
{
    for (auto& widget_info : widgets)
    {
        auto& handle_index = widget_info.handleIndex;
        if (index != handle_index) continue;
        if (isHoverExpanded(widget_info)) continue;
        auto& widget_state = widget_info.state;
        (handle_index < 2)
            ? (position != 0)
                ? widget_state = State::Expanded
                : widget_state = State::Collapsed
            : (position != (askWindowSize().width() - handleWidth()))
                ? widget_state = State::Expanded
                : widget_state = State::Collapsed;
    }
}

void Splitter::hoverExpand(SplitterHandle* handlePtr)
{
    for (auto& widget_info : widgets)
    {
        if (!match(handlePtr, widget_info)) continue;
        if (isCollapsed(widget_info))
            expand(widget_info, true);
    }
}

void Splitter::storeWidths()
{
    if (!isInitialized)
        initialize();
    for (auto& widget_info : widgets)
    {
        if (!isExpanded(widget_info)) continue;
        auto size = sizes().at(widget_info.index);
        if (widget_info.width && size)
            widget_info.width = size;
    }
}

void Splitter::toggleExpansion(SplitterHandle* handlePtr)
{
    for (auto& widget_info : widgets)
    {
        if (!match(handlePtr, widget_info)) continue;
        (isExpanded(widget_info))
            ? collapse(widget_info)
            : expand(widget_info);
    }
}

void Splitter::unhoverAll()
{
    for (auto& widget_info : widgets)
    {
        if (!isHoverExpanded(widget_info)) continue;
        QTimer::singleShot(250, this, [&]()
            {
                if (!hasHover(widget_info))
                    collapse(widget_info);
            });
    }
}

// Splitter.cpp, Fernanda
