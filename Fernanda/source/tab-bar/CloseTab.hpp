#pragma once

#include "../common/UiButton.hpp"

class CloseTab : public UiButton
{
	Q_OBJECT

public:
	CloseTab(QWidget* parent = nullptr)
		: UiButton("TabButton", Ui::Close, parent, Ui::Ellipse) {}

	bool edited() const { return flagged(); }
	void setEdited(bool edited) { setFlagged(edited); }
};
