#include "LineNumberArea.h"
#include "TrueEditor.h"

LineNumberArea::LineNumberArea(TrueEditor* parent)
	: QWidget(parent), m_parent(parent) {}

QSize LineNumberArea::sizeHint() const
{
	return QSize(m_parent->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event)
{
	m_parent->lineNumberAreaPaintEvent(event);
}
