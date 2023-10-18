#pragma once

#include <QFontDialog>

class LiveFontDialog : public QFontDialog
{
public:
	using QFontDialog::QFontDialog;

	void accept() override {}
};
