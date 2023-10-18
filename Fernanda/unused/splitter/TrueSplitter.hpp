/*#pragma once

#include "../common/Utility.hpp"
#include "SplitterHandle.hpp"

#include <QChildEvent>
#include <QSplitter>
#include <QTimer>

class TrueSplitter : public QSplitter
{
	Q_OBJECT

public:
	enum class WidgetWas {
		Hidden,
		Shown
	};

	TrueSplitter(QWidget* parent)
		: QSplitter(parent) {}

signals:
	void resized();
	void widgetVisibilityChanged(int widgetIndex, WidgetWas visibility);

protected:
	virtual void childEvent(QChildEvent* event) override
	{
		if (event->type() == QEvent::ChildAdded && event->child()->isWidgetType())
			event->child()->installEventFilter(this);
		QSplitter::childEvent(event);
	}

	virtual bool eventFilter(QObject* object, QEvent* event) override
	{
		if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
			for (auto i = 0; i < count(); ++i) {
				(!widget(i)->isVisible())
					? emit widgetVisibilityChanged(i, WidgetWas::Hidden)
					: emit widgetVisibilityChanged(i, WidgetWas::Shown);
			}
			Event::delayCall(this, [&] { emit resized(); });
		}
		return QSplitter::eventFilter(object, event);
	}

	virtual void resizeEvent(QResizeEvent* event) override
	{
		QSplitter::resizeEvent(event);
		emit resized();
	}

	virtual QSplitterHandle* createHandle() override { return new SplitterHandle(orientation(), this); }
};
*/
