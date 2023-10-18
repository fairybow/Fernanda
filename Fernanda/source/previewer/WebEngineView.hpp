#pragma once

#include "WebDocument.hpp"
#include "WebEnginePage.hpp"

#include <QWebChannel>
#include <QWebEngineView>

class WebEngineView : public QWebEngineView
{
public:
	WebEngineView(WebDocument& content, QWidget* parent)
		: QWebEngineView(parent)
	{
		setContextMenuPolicy(Qt::NoContextMenu);
		m_channel->registerObject("content", &content);
		m_page->setWebChannel(m_channel);
		setPage(m_page);
	}

private:
	WebEnginePage* m_page = new WebEnginePage(this);
	QWebChannel* m_channel = new QWebChannel(this);
};
