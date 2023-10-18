#pragma once

#include "../common/Utility.hpp"

#include <QChar>
#include <QColor>
#include <QFontMetrics>
#include <QPainter>
#include <QPlainTextEdit>
#include <QRect>
#include <QTextBlock>
#include <QTimer>

class BlockCursor : public QObject
{
	Q_OBJECT

public:
	BlockCursor(QPlainTextEdit* editor)
		: QObject(editor), m_editor(editor)
	{
		connections();
		m_blinkTimer->setTimerType(Qt::VeryCoarseTimer);
		Utility::delayCall(this, [&] { emit startBlinkTimer(); });
	}

	void paint()
	{
		QPainter painter(m_editor->viewport());
		auto current_char = currentChar();
		auto rect = shapeFromTrueCursor(current_char);
		painter.fillRect(rect, colorBasedOnState());
		if (!current_char.isNull() && emit getHasBlock()) {
			painter.setPen(colorBasedOnState(true));
			painter.setFont(m_editor->font());
			painter.drawText(rect, current_char);
		}
	}

	QString color() const { return m_colorHex; }
	QString underColor() const { return m_underColorHex; }
	void setColor(const QString& color) { m_colorHex = color; }
	void setUnderColor(const QString& color) { m_underColorHex = color; }

signals:
	void startBlinkTimer();

	// fix these!
	bool getHasBlock();
	bool getHasBlink();

private:
	QPlainTextEdit* m_editor;
	QString m_colorHex = "#000";
	QString m_underColorHex = "#fff";
	bool m_blinkVisible = true;
	QTimer* m_blinkTimer = new QTimer(this);

	void connections()
	{
		connect(this, &BlockCursor::startBlinkTimer, this, [&] {
			if (!emit getHasBlink()) return;
			m_blinkTimer->start(1000);
			});
		connect(m_blinkTimer, &QTimer::timeout, this, [&] {
			m_blinkVisible = !m_blinkVisible;
			emit startBlinkTimer();
			});
		connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, [&] {
			if (m_editor->textCursor().hasSelection() || !emit getHasBlink()) return;
			m_blinkVisible = true;
			emit startBlinkTimer();
			});
	}

	const QChar currentChar()
	{
		auto text = m_editor->textCursor().block().text();
		auto current_position = m_editor->textCursor().positionInBlock();
		return (current_position < text.size()) ? text.at(current_position) : QChar();
	}

	const QRect shapeFromTrueCursor(const QChar& currentChar)
	{
		if (emit getHasBlock()) {
			QFontMetrics metrics(m_editor->font());
			currentChar.isNull()
				? m_editor->setCursorWidth(metrics.averageCharWidth())
				: m_editor->setCursorWidth(metrics.horizontalAdvance(currentChar));
		}
		else
			m_editor->setCursorWidth(2);
		auto result = m_editor->cursorRect(m_editor->textCursor());
		m_editor->setCursorWidth(0);
		return result;
	}

	const QColor colorBasedOnState(bool under = false)
	{
		QColor color;
		(!m_blinkVisible && emit getHasBlink())
			? color = QColor(0, 0, 0, 0)
			: under ? color = QColor(m_underColorHex) : color = QColor(m_colorHex);
		return color;
	}
};
