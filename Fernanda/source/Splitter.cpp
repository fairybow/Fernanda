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

const QStringList Splitter::devPrintInfos()
{
    QStringList result;
    result << "(-1 = unset)";
    for (auto& info : infos)
    {
        auto entry = "Alignment: " + QString(info.alignedLeft() ? "Left" : "Right");
        auto state = QString(info.isCollapsed() ? "Collapsed" : info.isExpanded() ? "Expanded" : "Hovering");
        entry.append("\nWidget index: " + QString::number(info.index))
            .append("\nAssociated handle index: " + QString::number(info.handleIndex))
            .append("\nLast saved width: " + QString::number(info.width))
            .append("\nCurrent state: " + state);
        result << entry;
    }
    return result;
}

const QStringList Splitter::devPrintInitialSizes()
{
    QStringList result;
    for (auto i = 0; i < initialSizes.count(); ++i)
        result << QString(QString::number(i) + ": " + QString::number(initialSizes.at(i)));
    return result;
}

void Splitter::addWidgets(QVector<QWidget*> widgets)
{
    for (auto& widget : widgets)
        addWidget(widget);
    for (auto i = 0; i < count(); ++i)
    {
        if (i != 1)
        {
            setCollapsible(i, true);
            widgets.at(i)->installEventFilter(this);
            auto alignment = (i < 1) ? Alignment::Left : Alignment::Right;
            auto is_left = (alignment == Alignment::Left);
            is_left
                ? setStretchFactor(i, 0)
                : setStretchFactor(i, 1);
            auto handle_index = is_left ? i + 1 : i;
            infos << Info{
                alignment,
                i,
                handle_index,
                widget(i),
                handle(handle_index)
            };
        }
        else
        {
            setStretchFactor(i, 1);
            setCollapsible(i, false);
        }
    }
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
        setSizes(QVector<int>{ width * (2 / 10), width * (4 / 10), width * (4 / 10) });
    }
    else
        restoreState(state);
    setHandleWidth(6);
    QTimer::singleShot(0, this, [&]()
        {
            initialSizes = recordInitialSizes();
            storeWidths();
        });
}

void Splitter::surfaceDoubleClicked(QWidget* widgetPtr)
{
    for (auto& info : infos)
    {
        if (info.widget != widgetPtr) continue;
        info.isExpanded()
            ? collapse(info)
            : expand(info);
    }
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

QVector<int> Splitter::recordInitialSizes()
{
    QVector<int> result;
    for (auto i = 0; i < count(); ++i)
        result << sizes().at(i);
    return result;
}

void Splitter::collapse(Info& info)
{
    info.state = State::Collapsed;
    moveSplitter(toWindowX(info, 0), info.handleIndex);
}

void Splitter::expand(Info& info, bool isHover)
{
    isHover
        ? info.state = State::Hovering
        : info.state = State::Expanded;
    moveSplitter(toWindowX(info, info.width), info.handleIndex);
}

void Splitter::uncollapseAll()
{
    for (auto& info : infos)
    {
        if (!info.isCollapsed()) continue;
        auto width = info.width;
        if (width >= 0)
            moveSplitter(toWindowX(info, width), info.handleIndex);
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
    for (auto& info : infos)
    {
        if (info.width) continue;
        info.state = State::Collapsed;
        isInitialized = true;
    }
}

void Splitter::checkStates(int position, int handleIndex)
{
    for (auto& info : infos)
    {
        if (handleIndex != info.handleIndex) continue;
        if (info.isHovering()) continue;
        info.alignedLeft()
            ? position
                ? info.state = State::Expanded
                : info.state = State::Collapsed
            : (position != (askWindowSize().width() - handleWidth()))
                ? info.state = State::Expanded
                : info.state = State::Collapsed;
    }
}

void Splitter::hoverExpand(SplitterHandle* handlePtr)
{
    for (auto& info : infos)
    {
        if (handlePtr != info.handle) continue;
        if (info.isCollapsed())
            expand(info, true);
    }
}

void Splitter::storeWidths()
{
    if (!isInitialized)
        initialize();
    for (auto& info : infos)
    {
        if (!info.isExpanded()) continue;
        auto size = sizes().at(info.index);
        if (info.width && size)
            info.width = size;
    }
}

void Splitter::toggleExpansion(SplitterHandle* handlePtr)
{
    for (auto& info : infos)
    {
        if (handlePtr != info.handle) continue;
        info.isExpanded()
            ? collapse(info)
            : expand(info);
    }
}

void Splitter::unhoverAll()
{
    for (auto& info : infos)
    {
        if (!info.isHovering()) continue;
        QTimer::singleShot(250, this, [&]()
            {
                if (!info.hasWidgetHover())
                    collapse(info);
            });
    }
}

// Splitter.cpp, Fernanda
