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

    void collapse(Info& widgetInfo);
    void expand(Info& widgetInfo, bool isHover = false);
    void uncollapseAll();
    void unhoverAll();
    bool eventFilter(QObject* watched, QEvent* event);

private slots:
    void initialize();
    void storeWidths();
    void toggleExpansion(SplitterHandle* handlePointer);
    void hoverExpand(SplitterHandle* handlePointer);
    void checkStates(int position, int index);

signals:
    QRect askWindowSize();
};

// Splitter.h, Fernanda
