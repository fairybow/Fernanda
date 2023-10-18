/*#pragma once

#include <QEventLoop>
#include <QProgressDialog>

class EventLoop : public QEventLoop
{
public:
	EventLoop(QWidget* parent = nullptr)
		: QEventLoop(parent),
		m_dialog("Checking...", QString(), 0, 0, parent)
	{
		m_dialog.setWindowModality(Qt::WindowModal);
		m_dialog.setCancelButton(nullptr);
	}

	~EventLoop()
	{
		m_dialog.deleteLater();
	}

	int exec(ProcessEventsFlags flags = AllEvents)
	{
		m_dialog.show();
		return QEventLoop::exec(flags);
	}

public slots:
	void quit()
	{
		m_dialog.hide();
		QEventLoop::quit();
	}

private:
	QProgressDialog m_dialog;
};
*/
