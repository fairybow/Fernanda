/*#pragma once

#include <QPainter>
#include <QSplitterHandle>

class SplitterHandle : public QSplitterHandle
{
public:
	SplitterHandle(Qt::Orientation orientation, QSplitter* parent)
		: QSplitterHandle(orientation, parent) {}

	virtual QSize sizeHint() const override
	{
		auto size = QSplitterHandle::sizeHint();
		auto i = 5;
		(orientation() != Qt::Horizontal) ? size.setHeight(i) : size.setWidth(i);
		return size;
	}
};
*/
