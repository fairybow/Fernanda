#pragma once

#include "../common/UiButton.hpp"

class AddTab : public UiButton
{
public:
	AddTab(QWidget* parent = nullptr)
		: UiButton("TabButton", Ui::Add, parent) {}
};
