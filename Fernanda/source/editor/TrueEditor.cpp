#include "TrueEditor.h"

TrueEditor::TrueEditor(QWidget* parent)
	: QPlainTextEdit(parent), m_lineNumberArea(nullptr)
{
	connections();

	Utility::delayCall(this, [&] {
		highlightCurrentLine();
		updateLineNumberAreaWidth();
		});
}

void TrueEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
	QPainter painter(m_lineNumberArea);

	auto block = firstVisibleBlock();
	auto block_number = block.blockNumber();
	auto top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
	auto bottom = top + qRound(blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			auto number = QString::number(block_number + 1);
			painter.drawText(0, top, m_lineNumberArea->width() - 3,
				fontMetrics().height(), Qt::AlignRight, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + qRound(blockBoundingRect(block).height());
		++block_number;
	}
}

int TrueEditor::lineNumberAreaWidth()
{
	return (m_lineNumberArea == nullptr || !m_lineNumberArea->isVisible()) ? 0 : [&] {
		auto digits = 1;
		auto max = qMax(1, blockCount());

		while (max >= 10) {
			max /= 10;
			++digits;
		}

		auto space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
		return space;
	}();
}

void TrueEditor::setFont(const QFont& font)
{
	QFont q_font = font;
	q_font.setStyleStrategy(QFont::PreferAntialias);
	q_font.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
	QPlainTextEdit::setFont(q_font);
	m_lineNumberArea->setFont(q_font);
}

void TrueEditor::paintEvent(QPaintEvent* event)
{
	QPlainTextEdit::paintEvent(event);
	m_cursor->paint();
}

void TrueEditor::resizeEvent(QResizeEvent* event)
{
	QPlainTextEdit::resizeEvent(event);
	auto contents_rect = contentsRect();
	m_lineNumberArea->setGeometry(QRect(contents_rect.left(),
		contents_rect.top(), lineNumberAreaWidth(), contents_rect.height()));
}

void TrueEditor::setLineNumberArea(LineNumberArea* lineNumberArea) // can't define in header?
{
	m_lineNumberArea = lineNumberArea;
}

void TrueEditor::setCursorStyle(const QString& styleSheet)
{
	auto it = QRegularExpression(
		StyleRegex::CURSOR_BLOCK).globalMatch(styleSheet);

	while (it.hasNext()) {
		auto match = it.next();

		QString css_block = match.capturedTexts().at(0);

		auto match_cursor = QRegularExpression(
			StyleRegex::CURSOR_COLOR_LINE).match(css_block).captured(2);
		auto match_under_cursor = QRegularExpression(
			StyleRegex::CURSOR_UNDER_COLOR_LINE).match(css_block).captured(2);

		if (QColor(match_cursor).isValid())
			m_cursor->setColor(match_cursor);
		if (QColor(match_under_cursor).isValid())
			m_cursor->setUnderColor(match_under_cursor);
	}
}

int TrueEditor::selectedLineCount()
{
	auto cursor = textCursor();
	if (!cursor.hasSelection()) return 1;
	return cursor.selectedText().count(QChar(0x2029)) + 1;
}

void TrueEditor::highlightCurrentLine()
{
	QVector<QTextEdit::ExtraSelection> extra_selections;

	if (!isReadOnly()) {
		QTextEdit::ExtraSelection selection;
		selection.format.setBackground(highlight());
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extra_selections.append(selection);
	}

	setExtraSelections(extra_selections);
}

void TrueEditor::connections()
{
	connect(this, &TrueEditor::blockCountChanged, this, [&](int) {
		if (m_lineNumberArea == nullptr) return;
		updateLineNumberAreaWidth();
		});
	connect(this, &TrueEditor::updateRequest, this, [&](const QRect& rect, int dy) {
		if (m_lineNumberArea == nullptr) return;
		updateLineNumberArea(rect, dy);
		});
	connect(this, &TrueEditor::cursorPositionChanged, this, [&] {
		highlightCurrentLine();
		});

	cursorConnections();
}

void TrueEditor::cursorConnections()
{
	connect(m_cursor, &BlockCursor::getHasBlink, this, [&] {
		return emit getHasCursorBlink();
		});
	connect(m_cursor, &BlockCursor::getHasBlock, this, [&] {
		return emit getHasCursorBlock();
		});
}

void TrueEditor::updateLineNumberArea(const QRect& rect, int dy)
{
	dy
		? m_lineNumberArea->scroll(0, dy)
		: m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

	if (rect.contains(viewport()->rect()))
		updateLineNumberAreaWidth();
}

const QColor TrueEditor::highlight()
{
	QColor color;
	emit getHasLineHighlight()
		? color = QColor(255, 255, 255, 30)
		: color = QColor(0, 0, 0, 0);
	return color;
}
