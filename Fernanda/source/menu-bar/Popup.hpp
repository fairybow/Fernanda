#pragma once

#include "../common/StringTools.hpp"
#include "../Version.hpp"
#include "LicenseDialog.hpp"
#include "PopupText.hpp"
#include "VersionChecker.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QVector>

namespace Popup
{
	namespace
	{
		inline void standardize(QMessageBox& box, const QString& text, bool hasOk = true, bool hasIcon = false, const QString& title = "Fernanda")
		{
			box.setWindowTitle(title);
			box.setText(text);
			if (hasIcon)
				box.setIconPixmap(QPixmap(":/menu-bar/Fernanda_64.png"));
			if (hasOk)
				box.setDefaultButton(box.addButton(QMessageBox::Ok));
		}
	}

	inline void checkVersion(QWidget* parent)
	{
		QMessageBox box(parent);
		standardize(box, VersionChecker::check(VER_COMPANYNAME_STR, VER_PRODUCTNAME_STR, VER_FILEVERSION_STR, parent), true, true);
		box.exec();
	}

	inline void about(QWidget* parent)
	{
		QMessageBox box(parent);
		standardize(box, PopupText::about(), true, true);
		auto update_text = StringTools::pad(3, "Check for updates").toLocal8Bit();
		auto update = box.addButton(QObject::tr(update_text), QMessageBox::AcceptRole);
		auto licenses = box.addButton(QObject::tr("Licenses"), QMessageBox::AcceptRole);
		auto qt = box.addButton(QObject::tr("About Qt"), QMessageBox::AcceptRole);
		QObject::connect(qt, &QPushButton::clicked, parent, QApplication::aboutQt);
		box.exec();
		if (box.clickedButton() == update)
			checkVersion(parent);
		else if (box.clickedButton() == licenses) {
			LicenseDialog dialog(parent);
			dialog.exec();
		}
	}
}
