#pragma once

#include "../common/UiButton.hpp"

#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QString>

class ToolButton : public UiButton
{
	Q_OBJECT

public:
	ToolButton(const QString& label, QWidget* parent = nullptr, double idleOpacity = 0.3)
		: UiButton("ToolButton", label, parent), m_idleOpacity(idleOpacity)
	{
		setup();
	}

	void setStateOpacity()
	{
		m_effect->setOpacity(isChecked() ? 1.0 : m_idleOpacity);
	}

public slots:
	virtual void setVisible(bool visible) override
	{
		if (isChecked())
			setChecked(false);
		UiButton::setVisible(visible);
	}

protected:
	virtual bool eventFilter(QObject* object, QEvent* event) override
	{
		if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave) {
			hoveredOver()
				? m_effect->setOpacity(1.0)
				: setStateOpacity();
		}
		return UiButton::eventFilter(object, event);
	}

	virtual void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::RightButton) {
			setChecked(!isChecked());
			setStateOpacity();
			return;
		}
		UiButton::mousePressEvent(event);
	}

private:
	double m_idleOpacity;
	QGraphicsOpacityEffect* m_effect = new QGraphicsOpacityEffect(this);

	void setup()
	{
		setCheckable(true);
		setStateOpacity();
		setGraphicsEffect(m_effect);
		installEventFilter(this);
	}
};
