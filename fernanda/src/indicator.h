/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// indicator.h, Fernanda

#pragma once

#include "icon.h"
#include "statusbarbutton.h"
#include "text.h"
#include "userdata.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

class Indicator : public QWidget
{
    Q_OBJECT

public:
    Indicator(QWidget* parent = nullptr);

    enum class Type {
        Counts,
        Positions,
        Selection
    };

    struct Counts {
        QString text;
        int blockCount;
    };
    struct Positions {
        int cursorBlockNumber;
        int cursorPositionInBlock;
    };

    void toggle(bool checked, UserData::IniValue value);
    void signalFilter(Type type, bool force = false);
    
    bool autoCountActive() { return hasAutoCount; }

private:
    QHBoxLayout* layout = new QHBoxLayout(this);
    QLabel* positions = new QLabel(this);
    QLabel* separator = new QLabel(this);
    QLabel* counts = new QLabel(this);
    StatusBarButton* refresh = new StatusBarButton(this);

    bool hasAutoCount = true;
    bool hasLinePosition = true;
    bool hasColumnPosition = true;
    bool hasLineCount = true;
    bool hasWordCount = true;
    bool hasCharCount = true;

    QGraphicsOpacityEffect* opacity(double qreal);
    bool toggleVisibility(QLabel* label, bool hasAnything);
    void updateCounts(bool isSelection = false);
    void updatePositions();

    bool hasAnyCount() { return toggleVisibility(counts, (hasLineCount || hasWordCount || hasCharCount)); }
    bool hasEitherPosition() { return toggleVisibility(positions, (hasLinePosition || hasColumnPosition)); }

signals:
    void toggled();
    void toggleAutoCount(bool checked);
    void askEditorFocusReturn();
    Indicator::Counts askForCounts(bool isSelection);
    Indicator::Positions askForPositions();
};

// indicator.h, Fernanda
