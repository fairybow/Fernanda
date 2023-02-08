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

// Indicator.cpp, Fernanda

#include "Indicator.h"

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
    refresh->setText(Icon::draw(Icon::Name::Refresh));
    layout->addWidget(refresh);
    connect(this, &Indicator::toggled, this, [&]()
        {
            (positions->isHidden() || counts->isHidden())
                ? separator->hide()
                : separator->show();
        });
    connect(this, &Indicator::toggleAutoCount, this, [&](bool checked)
        {
            hasAutoCount = checked;
            refresh->setVisible(!checked);
        });
    connect(refresh, &QPushButton::pressed, this, [&]()
        {
            signalFilter(Type::Counts, true);
            askEditorFocusReturn();
        });
}

void Indicator::toggle(bool checked, UserData::IniValue value)
{
    switch (value) {
    case UserData::IniValue::CharCount:
        hasCharCount = checked;
        break;
    case UserData::IniValue::ColumnPosition:
        hasColumnPosition = checked;
        break;
    case UserData::IniValue::LineCount:
        hasLineCount = checked;
        break;
    case UserData::IniValue::LinePosition:
        hasLinePosition = checked;
        break;
    case UserData::IniValue::WordCount:
        hasWordCount = checked;
        break;
    }
    updateCounts();
    updatePositions();
    UserData::saveConfig(UserData::IniGroup::Window, value, checked);
}

void Indicator::signalFilter(Type type, bool force)
{
    if ((!hasAutoCount && type == Type::Counts) && !force) return;
    switch (type) {
    case Type::Counts:
        updateCounts();
        break;
    case Type::Positions:
        updatePositions();
        break;
    case Type::Selection:
        updateCounts(true);
        break;
    }
}

QGraphicsOpacityEffect* Indicator::opacity(double qreal)
{
    auto result = new QGraphicsOpacityEffect(this);
    result->setOpacity(qreal);
    return result;
}

bool Indicator::toggleVisibility(QLabel* label, bool hasAnything)
{
    label->setVisible(hasAnything);
    toggled();
    return hasAnything;
}

void Indicator::updateCounts(bool isSelection)
{
    auto has_any_count = hasAnyCount();
    if (!has_any_count) return;
    auto counts_info = askForCounts(isSelection);
    QStringList elements;
    if (hasLineCount)
        elements << QString::number(counts_info.blockCount) + QStringLiteral(" lines");
    if (hasWordCount)
        elements << QString::number(counts_info.text.split(Text::regex(Text::Regex::Split), Qt::SkipEmptyParts).count()) + QStringLiteral(" words");
    auto character_count = counts_info.text.count();
    if (hasCharCount)
        elements << QString::number(character_count) + QStringLiteral(" chars");
    counts->setText(elements.join(QStringLiteral(", ")));
    toggleAutoCount(!(character_count >= 20000));
}

void Indicator::updatePositions()
{
    auto has_either_position = hasEitherPosition();
    if (!has_either_position) return;
    auto positions_info = askForPositions();
    QStringList elements;
    if (hasLinePosition)
        elements << QStringLiteral("ln ") + QString::number(positions_info.cursorBlockNumber + 1);
    if (hasColumnPosition)
        elements << QStringLiteral("col ") + QString::number(positions_info.cursorPositionInBlock + 1);
    positions->setText(elements.join(QStringLiteral(", ")));
}

// Indicator.cpp, Fernanda
