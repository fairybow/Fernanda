/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

// indicator.cpp, Fernanda

#include "indicator.h"

Indicator::Indicator(QWidget* parent)
    : QWidget(parent)
{
    separator->setText(QStringLiteral("/"));
    positions->setGraphicsEffect(opacity(0.8));
    separator->setGraphicsEffect(opacity(0.3));
    counts->setGraphicsEffect(opacity(0.8));
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    for (const auto& widget : { positions, separator, counts })
    {
        widget->setObjectName(QStringLiteral("indicator"));
        layout->addWidget(widget);
    }
    connect(this, &Indicator::toggled, this, [&]()
        {
            if (positions->isHidden() || counts->isHidden())
            separator->hide();
            else
                separator->show();
        });
}

void Indicator::toggle(bool checked, Has has)
{
    switch (has) {
    case Has::CharCount:
        hasCharCount = checked;
        askSignalTextChanged();
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::CharCount, checked);
        break;
    case Has::ColumnPosition:
        hasColumnPosition = checked;
        askSignalCursorPositionChanged();
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::ColumnPosition, checked);
        break;
    case Has::LineCount:
        hasLineCount = checked;
        askSignalTextChanged();
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::LineCount, checked);
        break;
    case Has::LinePosition:
        hasLinePosition = checked;
        askSignalCursorPositionChanged();
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::LinePosition, checked);
        break;
    case Has::WordCount:
        hasWordCount = checked;
        askSignalTextChanged();
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WordCount, checked);
        break;
    }
}

void Indicator::updatePositions(const int cursorBlockNumber, const int cursorPositionInBlock)
{
    if (!hideOrShow(positions, hasLinePosition, hasColumnPosition)) return;
    QStringList elements;
    if (hasLinePosition)
        elements << QStringLiteral("ln ") + QString::number(cursorBlockNumber + 1);
    if (hasColumnPosition)
        elements << QStringLiteral("col ") + QString::number(cursorPositionInBlock + 1);
    positions->setText(elements.join(QStringLiteral(", ")));
}

void Indicator::updateCounts(const QString text, const int blockCount)
{
    if (!hideOrShow(counts, hasLineCount, hasWordCount, hasCharCount)) return;
    const auto word_count = text.split(Text::regex(Text::Regex::Split), Qt::SkipEmptyParts).count();
    const auto char_count = text.count();
    QStringList elements;
    if (hasLineCount)
        elements << QString::number(blockCount) + QStringLiteral(" lines");
    if (hasWordCount)
        elements << QString::number(word_count) + QStringLiteral(" words");
    if (hasCharCount)
        elements << QString::number(char_count) + QStringLiteral(" chars");
    counts->setText(elements.join(QStringLiteral(", ")));
}

bool Indicator::hideOrShow(QLabel* label, bool feature1, bool feature2, bool feature3)
{
    auto result = false;
    if (!feature1 && !feature2 && !feature3)
    {
        label->hide();
        toggled();
    }
    else
    {
        label->show();
        toggled();
        result = true;
    }
    return result;
}

QGraphicsOpacityEffect* Indicator::opacity(double qreal)
{
    auto result = new QGraphicsOpacityEffect(this);
    result->setOpacity(qreal);
    return result;
}

// indicator.cpp, Fernanda
