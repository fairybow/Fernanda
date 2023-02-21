/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
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
        entry.append("\nCurrent state: " + QString((widget_info.state == State::Collapsed) ? "Collapsed" : "Expanded"));
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
        setSizes(QVector<int>{ static_cast<int>(width * 0.2), static_cast<int>(width * 0.4), static_cast<int>(width * 0.4) });
    }
    else
        restoreState(state);
    setHandleWidth(5);
    devStartUpSizes = devRecordStartUpSizes();
}

SplitterHandle* Splitter::createHandle()
{
    auto handle = new SplitterHandle(orientation(), this);
    connect(handle, &SplitterHandle::askIsInitialized, this, &Splitter::initialize);
    connect(handle, &SplitterHandle::askStoreWidths, this, &Splitter::storeWidths);
    connect(handle, &SplitterHandle::askToggleExpansion, this, &Splitter::toggleExpansion);
    return handle;
}

void Splitter::collapse(Info& widgetInfo)
{
    widgetInfo.state = State::Collapsed;
    auto& handle_index = widgetInfo.handleIndex;
    moveSplitter(((handle_index < 2) ? 0 : (askWindowSize().width()) - handleWidth()), handle_index);
}

void Splitter::expand(Info& widgetInfo)
{
    widgetInfo.state = State::Expanded;
    auto& handle_index = widgetInfo.handleIndex;
    auto& stored_width = widgetInfo.width;
    auto window_width = askWindowSize().width();
    (widgetInfo.width < 1)
        ? moveSplitter(((handle_index < 2) ? (window_width * 0.2) : (window_width - (window_width * 0.4))), handle_index)
        : moveSplitter(((handle_index < 2) ? stored_width : (window_width - stored_width)), handle_index);
}

void Splitter::uncollapseAll()
{
    for (auto& widget_info : widgets)
    {
        if (widget_info.state != State::Collapsed) continue;
        auto& handle_index = widget_info.handleIndex;
        auto& stored_width = widget_info.width;
        if (stored_width != -1)
            moveSplitter(((handle_index < 2) ? stored_width : (askWindowSize().width() - stored_width)), handle_index);
    }
}

void Splitter::initialize()
{
    if (isInitialized) return;
    for (auto& widget_info : widgets)
    {
        if (widget_info.width) continue;
        widget_info.state = State::Collapsed;
    }
    storeWidths();
    isInitialized = true;
}

void Splitter::storeWidths()
{
    for (auto& widget_info : widgets)
    {
        if (widget_info.state != State::Expanded) continue;
        auto size = sizes().at(widget_info.index);
        if (widget_info.width && size)
            widget_info.width = size;
    }
}

void Splitter::toggleExpansion(SplitterHandle* handlePointer)
{
    for (auto& widget_info : widgets)
    {
        auto& handle_index = widget_info.handleIndex;
        if (handlePointer != handle(handle_index)) continue;
        (widget_info.state == State::Expanded)
            ? collapse(widget_info)
            : expand(widget_info);
    }
}

void Splitter::checkStates(int position, int index)
{
    for (auto& widget_info : widgets)
    {
        auto& handle_index = widget_info.handleIndex;
        if (index != handle_index) continue;
        (handle_index < 2)
            ? (position != 0)
                ? widget_info.state = State::Expanded
                : widget_info.state = State::Collapsed
            : (position != (askWindowSize().width() - handleWidth()))
                ? widget_info.state = State::Expanded
                : widget_info.state = State::Collapsed;
    }
}

// Splitter.cpp, Fernanda
