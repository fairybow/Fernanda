#pragma once

#include "../common/Emoji.hpp"
#include "../common/StringTools.hpp"
#include "../common/Utility.hpp"
#include "ToolButton.hpp"

#include <QEnterEvent>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QString>
#include <QTimer>

/* Changing style sheets stops the timer display and/or timer */

class PomodoroTimer : public ToolButton
{
	Q_OBJECT

public:
	PomodoroTimer(QMainWindow* mainWindow,
		QWidget* parent = nullptr, int defaultSecondsCountdown = defaultInterval());

	static int defaultInterval() { return 1500; }

	void setCountdown(int seconds) { m_interval = qBound(30, seconds, 3600); }

protected:
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;

private:
	int m_interval;
	int m_countdown = 0;
	QMainWindow* m_window;
	QTimer* m_timer = new QTimer(this);

	void timeUp(QMainWindow* parentWindow);
	bool isStopping(bool checked);
	bool isMidCountdown();
	bool pauseOrResumeIfRunning();

private slots:
	void countdownDisplay();
	void startCountdown(bool checked);
};
