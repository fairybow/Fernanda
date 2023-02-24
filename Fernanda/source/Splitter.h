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

    const QStringList devRecordStartUpSizes();
    const QStringList devGetStates();
    void addWidgets(QVector<QWidget*> widgets);
    void saveConfig();
    void loadConfig();

    const QStringList devGetStartUpSizes() { return devStartUpSizes; }

protected:
    SplitterHandle* createHandle() override;

private:
    enum class State {
        Collapsed,
        Expanded,
        HoverExpanded
    };

    struct Info {
        int index = -1;
        int handleIndex = -1;
        int width = -1;
        State state = State::Expanded;
    };

    QStringList devStartUpSizes = QStringList();
    Info pane = Info{ 0, 1 };
    Info editor = Info{ 1 };
    Info preview = Info{ 2, 2 };
    QVector<Info> widgets = { pane, editor, preview };

    bool isInitialized = false;

    int toDefault(int index);
    void collapse(Info& widgetInfo);
    void expand(Info& widgetInfo, bool isHover = false);
    void uncollapseAll();
    bool eventFilter(QObject* watched, QEvent* event);
    void initialize();

    int toWindowX(int index, int size) { return (index < 2) ? size : askWindowSize().width() - size; }
    bool match(SplitterHandle* handlePtr, Info& widgetInfo) const { return (handlePtr == handle(widgetInfo.handleIndex)); }
    bool isCollapsed(Info& widgetInfo) const { return (widgetInfo.state == State::Collapsed); }
    bool isExpanded(Info& widgetInfo) const { return (widgetInfo.state == State::Expanded); }
    bool isHoverExpanded(Info& widgetInfo) const { return (widgetInfo.state == State::HoverExpanded); }
    bool hasHover(Info& widgetInfo) const { return (widget(widgetInfo.index)->underMouse() || handle(widgetInfo.handleIndex)->underMouse()); }


private slots:
    void checkStates(int position, int index);
    void hoverExpand(SplitterHandle* handlePtr);
    void storeWidths();
    void toggleExpansion(SplitterHandle* handlePtr);
    void unhoverAll();

signals:
    QRect askWindowSize();
};

// Splitter.h, Fernanda
