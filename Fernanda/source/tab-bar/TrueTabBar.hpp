#pragma once

#include <QHoverEvent>
#include <QMouseEvent>
#include <QSize>
#include <QStyle>
#include <QTabBar>

#include <utility>

class TrueTabBar : public QTabBar
{
	Q_OBJECT

public:
	TrueTabBar(int minTabSize, int maxTabSize, QWidget* parent = nullptr)
		: QTabBar(parent), m_tabSizeRange(minTabSize, maxTabSize)
	{
		setElideMode(Qt::ElideRight);
		setAutoHide(false);
		setMovable(true);
		setExpanding(false);
		setUsesScrollButtons(true);
		setDrawBase(false);
		installEventFilter(this);
	}

signals:
	void resized();
	void inserted();
	void removed();

protected:
	virtual bool eventFilter(QObject* object, QEvent* event) override
	{
		if (event->type() == QEvent::MouseMove && !m_isDragging)
		{
			auto mouse_event = static_cast<QMouseEvent*>(event);
			auto index = tabAt(mouse_event->pos());
			if (index != -1) {
				m_lastHoveredIndex = m_hoveredIndex;
				m_hoveredIndex = index;
				setButtonProperty(m_lastHoveredIndex, false);
				setButtonProperty(m_hoveredIndex, true);
			}
			else
				setAllButtonProperties(false);
		}
		else if (event->type() == QEvent::HoverLeave)
			setAllButtonProperties(false);

		return QTabBar::eventFilter(object, event);
	}

	virtual void mousePressEvent(QMouseEvent* event) override
	{
		QTabBar::mousePressEvent(event);
		if (event->button() == Qt::LeftButton) {
			m_isDragging = true;
			setAllButtonProperties(false);
			setButtonProperty(currentIndex(), true);
		}
	}

	virtual void mouseReleaseEvent(QMouseEvent* event) override
	{
		QTabBar::mouseReleaseEvent(event);
		if (event->button() == Qt::LeftButton) {
			m_isDragging = false;
			setAllButtonProperties(false);
		}
		simulatedMouseMovement(event);
	}

	virtual void resizeEvent(QResizeEvent* event) override
	{
		QTabBar::resizeEvent(event);
		emit resized();
	}

	virtual void tabInserted(int index) override
	{
		QTabBar::tabInserted(index);
		emit inserted();
	}

	virtual void tabRemoved(int index) override
	{
		QTabBar::tabRemoved(index);
		emit removed();
	}

	virtual QSize tabSizeHint(int index) const override
	{
		auto tab_width = qMin(qMax(width() / count(), m_tabSizeRange.first), m_tabSizeRange.second);
		return QSize(tab_width, QTabBar::tabSizeHint(index).height());
	}

private:
	const std::pair<int, int> m_tabSizeRange;
	int m_hoveredIndex = -1;
	int m_lastHoveredIndex = -1;
	bool m_isDragging = false;

	void setButtonProperty(int index, bool value)
	{
		if (index < 0) return;
		auto button = tabButton(index, QTabBar::RightSide);
		if (!button) return;
		button->setProperty("tab-hover", value);
		button->style()->unpolish(button);
		button->style()->polish(button);
	}

	void setAllButtonProperties(bool value)
	{
		for (auto i = 0; i < count(); ++i)
			setButtonProperty(i, value);
		m_lastHoveredIndex = -1;
		m_hoveredIndex = -1;
	}

	void simulatedMouseMovement(QMouseEvent* event)
	{
		QMouseEvent mouseMoveEvent(QEvent::MouseMove,
			event->localPos(), event->windowPos(), event->screenPos(),
			Qt::NoButton, event->buttons(), event->modifiers());
		eventFilter(this, &mouseMoveEvent);
	}
};
