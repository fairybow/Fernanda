#pragma once

#include "../common/HtmlString.hpp"
#include "../common/Io.hpp"
#include "../common/Layout.hpp"
#include "../common/Path.hpp"

#include <QDialog>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QVector>

#include <filesystem>

class LicenseDialog : public QDialog
{
public:
	using StdFsPath = std::filesystem::path;
	using PathLabelList = QVector<std::pair<StdFsPath, QString>>;

	explicit LicenseDialog(QWidget* parent = nullptr)
		: QDialog(parent)
	{
		setWindowTitle("Licenses");

		auto scroll_area = new QScrollArea;
		scroll_area->setWidgetResizable(true);

		auto licenses = PathLabelList{
			{ ":/licenses/LICENSE", "Fernanda" },
			{ ":/licenses/Fernanda/external/qt/LICENSE", "Qt" },
			{ ":/licenses/Fernanda/external/material-icons/LICENSE", "Material Icons"},
			{ ":/licenses/Fernanda/external/mononoki/LICENSE", "mononoki" },
			{ ":/licenses/Fernanda/external/solarized/LICENSE", "Solarized" }
		};

		auto container = new QWidget;
		auto container_layout = Layout::box(Layout::Line::Vertically, nullptr, container, { 66, 0, 66, 0 });
		scroll_area->setWidget(container);
		scroll_area->setFrameStyle(QFrame::NoFrame);

		container_layout->addSpacing(50);
		scroll_area->setWidget(container);

		for (auto& license : licenses) {
			auto text_display = new QPlainTextEdit;
			text_display->setReadOnly(true);
			text_display->setMinimumHeight(300);
			text_display->setPlainText(Io::readFile(license.first));
			auto label_text = HtmlString::heading(license.second, 2);
			container_layout->addWidget(Layout::labeledContainer(text_display, nullptr, label_text));
			container_layout->addSpacing(50);
		}

		Layout::box(Layout::Line::Vertically, scroll_area, this);
		setMinimumSize(800, 600);
	}
};
