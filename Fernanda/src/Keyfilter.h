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

// Keyfilter.h, Fernanda

#pragma once

#include <QChar>
#include <QKeyEvent>
#include <QTextCursor>
#include <QVector>

#include <unordered_set>

class Keyfilter
{

public:
    struct ProximalChars {
        QChar current;
        QChar previous;
        QChar beforeLast;
    };

    QVector<QKeyEvent*> filter(QKeyEvent* event, ProximalChars chars);

private:
    QKeyEvent backspace{ QKeyEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier };
    QKeyEvent braceRight{ QKeyEvent::KeyPress, Qt::Key_BraceRight, Qt::NoModifier, "}" };
    QKeyEvent bracketRight{ QKeyEvent::KeyPress, Qt::Key_BracketRight, Qt::NoModifier, "]" };
    QKeyEvent emDash{ QKeyEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, "\U00002014" };
    QKeyEvent enDash{ QKeyEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, "\U00002013" };
    QKeyEvent left{ QKeyEvent::KeyPress, Qt::Key_Left, Qt::NoModifier };
    QKeyEvent parenRight{ QKeyEvent::KeyPress, Qt::Key_ParenRight, Qt::NoModifier, ")" };
    QKeyEvent quote{ QKeyEvent::KeyPress, Qt::Key_QuoteDbl, Qt::NoModifier, QString('"') };
    QKeyEvent right{ QKeyEvent::KeyPress, Qt::Key_Right, Qt::NoModifier };
    //QKeyEvent delete{ QKeyEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier };
    //QKeyEvent guillemetRight{ QKeyEvent::KeyPress, Qt::Key_guillemotright, Qt::NoModifier, "\U000000BB" };
    //QKeyEvent space{ QKeyEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " " };
    std::unordered_set<QChar> spaceSkips{ '}', ']', ',', '!', ')', '.', '?', '"' };
    std::unordered_set<QChar> commaSkips{ '}', ']', ')', '"' };

    QVector<QKeyEvent*> dontDuplicate(QKeyEvent* event, ProximalChars chars, char current);
    QVector<QKeyEvent*> commaSkip(QKeyEvent* event, ProximalChars chars);
    
    QVector<QKeyEvent*> autoClose(QKeyEvent* event, QKeyEvent* closer) { return QVector<QKeyEvent*>{ event, closer, & left }; }
    bool checkCurrent(ProximalChars chars, char current) { return (!chars.current.isNull() && chars.current == current); }
    bool checkPrevious(ProximalChars chars, char previous) { return (!chars.previous.isNull() && chars.previous == previous); }
    bool checkPrevAndBeforeLast(ProximalChars chars, char previous, char beforeLast) { return (!chars.previous.isNull() && !chars.beforeLast.isNull() && chars.previous == previous && chars.beforeLast == beforeLast); }
};

// Keyfilter.h, Fernanda
