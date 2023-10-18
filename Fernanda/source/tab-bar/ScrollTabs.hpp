#pragma once

#include "../common/UiButton.hpp"

#include <QTabBar>

class ScrollTabs : public UiButton
{
	Q_OBJECT

public:
	enum class Side { Left, Right };

	ScrollTabs(QTabBar* tabBar, Side side, QWidget* parent = nullptr)
		: UiButton("TabButton", icon(side), parent), m_tabBar(tabBar)
	{
		connect(this, &ScrollTabs::clicked, this, [&, side] {
			auto delta = (side == Side::Left) ? -1 : 1;
			m_tabBar->setCurrentIndex(m_tabBar->currentIndex() + delta);
			});

		// scroll wheel on tab bar should scroll
	}

private:
	QTabBar* m_tabBar;

	Ui icon(Side side)
	{
		return (side == Side::Left) ? Ui::ChevronLeft : Ui::ChevronRight;
	}
};
