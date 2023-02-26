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

// Splitter.h, Fernanda

#pragma once

#include "SplitterHandle.h"
#include "UserData.h"

#include <QByteArray>
#include <QSplitter>
#include <QVector>
#include <QWidget>

class Splitter : public QSplitter
{
    Q_OBJECT

public:
    Splitter(QWidget* parent = nullptr);

    const QStringList devPrintInfos();
    const QStringList devPrintInitialSizes();
    void addWidgets(QVector<QWidget*> widgets);
    void saveConfig();
    void loadConfig();

public slots:
    void surfaceDoubleClicked(QWidget* widgetPtr);

protected:
    SplitterHandle* createHandle() override;

private:
    enum class Alignment {
        Left,
        Right
    };
    enum class State {
        Collapsed,
        Expanded,
        Hovering
    };

    struct Info {
        Alignment alignment;
        int index;
        int handleIndex;
        QWidget* widget;
        QWidget* handle;
        int width = -1;
        State state = State::Expanded;
        bool alignedLeft() const { return (alignment == Alignment::Left); }
        bool alignedRight() const { return (alignment == Alignment::Right); }
        bool isCollapsed() const { return (state == State::Collapsed); }
        bool isExpanded() const { return (state == State::Expanded); }
        bool isHovering() const { return (state == State::Hovering); }
        bool hasWidgetHover() const { return (widget->underMouse() || handle->underMouse()); }
    };

    QVector<int> initialSizes;
    QVector<Info> infos;
    bool isInitialized = false;

    QVector<int> recordInitialSizes();
    void collapse(Info& info);
    void expand(Info& info, bool isHover = false);
    void uncollapseAll();
    bool eventFilter(QObject* watched, QEvent* event);
    void initialize();

    int toWindowX(Info& info, int size) { return info.alignedLeft() ? size : askWindowSize().width() - size; }

private slots:
    void checkStates(int position, int handleIndex);
    void hoverExpand(SplitterHandle* handlePtr);
    void storeWidths();
    void toggleExpansion(SplitterHandle* handlePtr);
    void unhoverAll();

signals:
    QRect askWindowSize();
};

// Splitter.h, Fernanda
