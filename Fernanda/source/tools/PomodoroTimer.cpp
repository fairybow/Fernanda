#include "PomodoroTimer.h"

PomodoroTimer::PomodoroTimer(QMainWindow* mainWindow,
	QWidget* parent, int defaultSecondsCountdown)
	: ToolButton(Emoji::TOMATO, parent),
	m_window(mainWindow),
	m_interval(defaultSecondsCountdown)
{
	m_timer->setTimerType(Qt::PreciseTimer);
	connect(m_timer, &QTimer::timeout, this, &PomodoroTimer::countdownDisplay);
	connect(this, &PomodoroTimer::toggled, this, &PomodoroTimer::startCountdown);
}

void PomodoroTimer::enterEvent(QEnterEvent* event)
{
	if (!m_timer->isActive() && !isMidCountdown())
		ToolButton::enterEvent(event);
}

void PomodoroTimer::leaveEvent(QEvent* event)
{
	if (!m_timer->isActive() && !isMidCountdown())
		ToolButton::leaveEvent(event);
}

void PomodoroTimer::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::RightButton)
		if (pauseOrResumeIfRunning()) return;
	ToolButton::mousePressEvent(event);
}

void PomodoroTimer::timeUp(QMainWindow* parentWindow)
{
	QMessageBox popup(parentWindow);
	popup.setWindowTitle(parentWindow->windowTitle());
	popup.setText(
		StringTools::flank("Time's up!", 30));
	auto ok = popup.addButton(QMessageBox::Ok);
	popup.setDefaultButton(ok);
	popup.exec();
	Utility::delayCall(this, [&] { setStateOpacity(); });
}

bool PomodoroTimer::isStopping(bool checked)
{
	if (!checked) {
		setText(label());
		m_timer->stop();
		m_countdown = m_interval;
		return true;
	}
	return false;
}

bool PomodoroTimer::isMidCountdown()
{
	return (m_countdown > 0 && m_countdown < m_interval);
}

bool PomodoroTimer::pauseOrResumeIfRunning()
{
	if (isMidCountdown()) {
		m_timer->isActive()
			? m_timer->stop()
			: m_timer->start(1000);
		return true;
	}
	return false;
}

void PomodoroTimer::countdownDisplay()
{
	auto time = StringTools::secondsToMinutes(m_countdown, ".");
	auto text = StringTools::padAll(2, label(), time);
	setText(text);
	if (m_countdown < 1) {
		timeUp(m_window);
		setChecked(false);
		return;
	}
	--m_countdown;
}

void PomodoroTimer::startCountdown(bool checked)
{
	if (isStopping(checked)) return;
	m_countdown = m_interval;
	m_timer->start(1000);
}
