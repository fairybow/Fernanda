#include "Meter.h"

Meter::Meter(const char* name, QWidget* parent)
	: Widget(name, parent)
{
	for (auto& widget : QWidgetList{ m_positions, m_separator, m_counts })
		widget->setObjectName(name);
	setupLabels();
	connections();
	Layout::box(Layout::Line::Horizontally,
		{ m_positions, m_separator, m_counts, m_refresh }, this);
	updateCounts();
	updatePositions();
}

void Meter::trigger(Type type, bool force)
{
	if ((!m_hasAutoCount && type == Type::Counts) && !force) return; // for positions, block for > x.
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
	default:
		updateCounts();
		updatePositions();
		break;
	}
}

void Meter::setVisible(bool visible)
{
	// store or recover previous states
	// (all need to be toggled on or off when toggling entire meter)
	QWidget::setVisible(visible);
}

void Meter::setupLabels()
{
	m_separator->setText("/");
	Fx::opacify(0.8, m_positions);
	Fx::opacify(0.3, m_separator);
	Fx::opacify(0.8, m_counts);
}

void Meter::connections()
{
	connect(this, &Meter::separatorVisibilityCheck, this, [&] {
		(m_positions->isHidden() || m_counts->isHidden())
			? m_separator->hide()
			: m_separator->show();
		});
	connect(this, &Meter::toggleAutoCount, this, [&](bool checked) {
		m_hasAutoCount = checked;
		m_refresh->setVisible(!checked);
		});
	connect(m_refresh, &UiButton::pressed, this, [&] {
		trigger(Type::Counts, true);
		emit editorFocusReturn();
		});
}

void Meter::updateOutput(bool& memberBool, bool state)
{
	memberBool = state;
	trigger();
}

void Meter::updateCounts(bool isSelection)
{
	if (!hasAnyCount()) return;
	emit askGiveCounts(isSelection);
}

void Meter::updatePositions()
{
	if (!hasEitherPosition()) return;
	emit askGivePositions();
}

void Meter::displayCounts(Counts counts)
{
	QStringList elements;
	if (m_hasLineCount) {
		auto block_count = Utility::greaterOrEqual(counts.blockCount, 1);
		elements << QString::number(block_count) + " lines";
	}
	if (m_hasWordCount) {
		auto words = counts.text.split(
			QRegularExpression(Regex::LEADING_WHITESPACE), Qt::SkipEmptyParts);
		elements << QString::number(words.count()) + " words";
	}
	auto character_count = counts.text.count();
	if (m_hasCharCount)
		elements << QString::number(character_count) + " chars";
	m_counts->setText(elements.join(", "));
	emit toggleAutoCount(!(character_count >= 20000));
}

void Meter::displayPositions(Positions positions)
{
	QStringList elements;
	if (m_hasLinePosition)
		elements << "ln " + QString::number(positions.cursorBlockNumber + 1);
	if (m_hasColumnPosition)
		elements << "col " + QString::number(positions.cursorPositionInBlock + 1);
	m_positions->setText(elements.join(", "));
}

bool Meter::toggleVisibility(QLabel* label, bool hasAnything)
{
	label->setVisible(hasAnything);
	emit separatorVisibilityCheck();
	return hasAnything;
}
