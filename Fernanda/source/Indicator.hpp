#pragma once

#include "common/Io.hpp"
#include "common/Layout.hpp"
#include "common/Widget.hpp"

#include <QProgressBar>
#include <QTimeLine>
#include <QTimer>

#include <filesystem>

class Indicator : public Widget<>
{
	Q_OBJECT

public:
	using StdFsPath = std::filesystem::path;

	Indicator(const char* name, QWidget* parent = nullptr)
		: Widget(name, parent)
	{
		buildProgressBar(name);
		Layout::transpareForMouse({ this, m_progressBar });
		Layout::box(Layout::Line::Vertically, m_progressBar, this);
	}

	void pastel(int delay = 0) { run(StdFsPath(":/indicator/Pastels.qss"), delay); }
	void green(int delay = 0) { run(StdFsPath(":/indicator/Green.qss"), delay); }
	void red(int delay = 0) { run(StdFsPath(":/indicator/Red.qss"), delay); }

	bool onResult(bool result, int delay = 0)
	{
		result ? green(delay) : red(delay);
		return result;
	}

	void setAlignment(const QString& alignment)
	{
		(alignment == QString("Top"))
			? layout()->setAlignment(Qt::AlignTop)
			: layout()->setAlignment(Qt::AlignBottom);
	}

private:
	QProgressBar* m_progressBar = new QProgressBar(this);
	QTimer* m_timer = new QTimer(this);

	void buildProgressBar(const char* name)
	{
		m_progressBar->setObjectName(name);
		m_progressBar->setMaximumHeight(3);
		m_progressBar->setTextVisible(false);
		m_progressBar->setRange(0, 100);
		m_progressBar->hide();
		connect(m_timer, &QTimer::timeout, this, [&] {
			m_progressBar->hide();
			m_progressBar->reset();
			});
	}

	void run(const StdFsPath& styleSheetPath, int delay)
	{
		if (!isVisible()) return;
		m_progressBar->setStyleSheet(Io::readFile(styleSheetPath));
		auto fill = new QTimeLine(300, this);
		connect(fill, &QTimeLine::frameChanged, m_progressBar, &QProgressBar::setValue);
		fill->setFrameRange(0, 100);
		QTimer::singleShot(qBound(0, delay, 3000), [&, fill] {
			m_progressBar->show();
			m_timer->start(1500);
			fill->start();
			});
	}
};
