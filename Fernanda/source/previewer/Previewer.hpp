#pragma once

#include "../common/Layout.hpp"
#include "../common/Widget.hpp"
#include "WebEngineView.hpp"

class Previewer : public Widget<>
{
public:
	Previewer(const char* name, QWidget* parent = nullptr)
		: Widget(name, parent)
	{
		Layout::box(Layout::Line::Vertically, m_view, this);
	}

private:
	WebDocument m_content;
	WebEngineView* m_view = new WebEngineView(m_content, this);

	QString url() { return QString("qrc:/Test.html"); }
};
