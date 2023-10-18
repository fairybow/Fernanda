#pragma once

#include "../common/Emoji.hpp"
#include "ToolButton.hpp"

#include <QString>
#include <QTimer>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

class StayAwake : public ToolButton
{
	Q_OBJECT

public:
	StayAwake(QWidget* parent = nullptr)
		: ToolButton(Emoji::TEACUP, parent)
	{
		m_timer->setTimerType(Qt::VeryCoarseTimer);
		connect(this, &StayAwake::toggled, this, &StayAwake::stayAwake);
		connect(this, &StayAwake::startTimer, this, [&] { m_timer->start(29000); });
		connect(m_timer, &QTimer::timeout, this, &StayAwake::stayAwake);
	}

signals:
	void startTimer();

private:
	QTimer* m_timer = new QTimer(this);

private slots:
	void stayAwake()
	{

#ifdef Q_OS_WINDOWS

		if (!isChecked())
			SetThreadExecutionState(ES_CONTINUOUS);
		else {
			SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
			emit startTimer();
		}

#endif

	}
};
