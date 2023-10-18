#pragma once

#include <QTimer>

class Delayer : public QObject
{
	Q_OBJECT

public:
	Delayer(QObject* parent, int linearDelay, int threshhold = 10000)
		: QObject(parent), m_linearDelay(linearDelay), m_threshhold(threshhold)
	{
		m_delay->setSingleShot(true);
		connect(m_delay, &QTimer::timeout, this, [&] {
			emit signal();
			});
	}

	void delayedEmit(int input)
	{
		(input < m_threshhold)
			? m_delay->setInterval(0)
			: m_delay->setInterval((input / m_threshhold) * m_linearDelay);
		m_delay->start();
	}

signals:
	void signal();

private:
	const int m_linearDelay;
	const int m_threshhold;
	QTimer* m_delay = new QTimer(this);
};
