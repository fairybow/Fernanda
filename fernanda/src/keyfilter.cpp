/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// keyfilter.cpp, Fernanda

#include "keyfilter.h"

QVector<QKeyEvent*> Keyfilter::filter(QKeyEvent* event, ProximalChars chars)
{
    QVector<QKeyEvent*> result;
    switch (event->key()) {
    case Qt::Key_BraceLeft:
        result << autoClose(event, &braceRight);
        break;
    case Qt::Key_BraceRight:
        result << dontDuplicate(event, chars, '}');
        break;
    case Qt::Key_BracketLeft:
        result << autoClose(event, &bracketRight);
        break;
    case Qt::Key_BracketRight:
        result << dontDuplicate(event, chars, ']');
        break;
    case Qt::Key_Comma:
        result << commaSkip(event, chars);
        break;
    case Qt::Key_Exclam:
        result << commaSkip(event, chars);
        break;
    case Qt::Key_Minus:
        (checkPrevious(chars, '-'))
            ? result << &backspace << &emDash
            : result << event;
        break;
    case Qt::Key_ParenLeft:
        result << autoClose(event, &parenRight);
        break;
    case Qt::Key_ParenRight:
        result << dontDuplicate(event, chars, ')');
        break;
    case Qt::Key_Period:
        result << commaSkip(event, chars);
        break;
    case Qt::Key_Question:
        result << commaSkip(event, chars);
        break;
    case Qt::Key_QuoteDbl:
        (checkCurrent(chars, '"'))
            ? result << &right
            : result << autoClose(event, event);
        break;
    case Qt::Key_Space:
    {
        if (!chars.current.isNull() && !chars.previous.isNull() && spaceSkips.count(chars.current) > 0 && chars.previous == ' ')
            result << &backspace << &right << event;
        else if (checkPrevAndBeforeLast(chars, '-', ' '))
            result << &backspace << &enDash << event;
        else
            result << event;
    }
    break;
    default:
        (checkPrevAndBeforeLast(chars, ' ', ' '))
            ? result << &backspace << event
            : result << event;
    }
    return result;
}

QVector<QKeyEvent*> Keyfilter::dontDuplicate(QKeyEvent* event, ProximalChars chars, char current)
{
    QVector<QKeyEvent*> result;
    (checkCurrent(chars, current))
        ? result << &right
        : result << event;
    return result;
}

QVector<QKeyEvent*> Keyfilter::commaSkip(QKeyEvent* event, ProximalChars chars)
{
    QVector<QKeyEvent*> result;
    if (!chars.current.isNull() && commaSkips.count(chars.current) > 0 && chars.previous == ',')
        result << &backspace << event << &right;
    else
        result << event;
    return result;
}

bool Keyfilter::checkCurrent(ProximalChars chars, char current)
{
    if (!chars.current.isNull() && chars.current == current)
        return true;
    return false;
}

bool Keyfilter::checkPrevious(ProximalChars chars, char previous)
{
    if (!chars.previous.isNull() && chars.previous == previous)
        return true;
    return false;
}

bool Keyfilter::checkPrevAndBeforeLast(ProximalChars chars, char previous, char beforeLast)
{
    if (!chars.previous.isNull() && !chars.beforeLast.isNull() && chars.previous == previous && chars.beforeLast == beforeLast)
        return true;
    return false;
}

// keyfilter.cpp, Fernanda
