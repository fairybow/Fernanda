#pragma once

#include "../common/Emoji.hpp"
#include "ToolButton.hpp"

#include <QMainWindow>
#include <QString>

class AlwaysOnTop : public ToolButton
{
	Q_OBJECT

public:
	AlwaysOnTop(QMainWindow* mainWindow, QWidget* parent = nullptr)
		: ToolButton(Emoji::PUSHPIN, parent), m_window(mainWindow)
	{
		connect(this, &AlwaysOnTop::toggled, this, &AlwaysOnTop::alwaysOnTop);
	}

private:
	QMainWindow* m_window;

private slots:
	void alwaysOnTop()
	{
		isChecked()
			? m_window->setWindowFlags(m_window->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
			: m_window->setWindowFlags(m_window->windowFlags() ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
		m_window->show();
	}
};
