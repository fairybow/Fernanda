#include "common/Path.hpp"
#include "common/Utility.hpp"
#include "LaunchCop.hpp"
#include "Logger.hpp"
#include "MainWindow.h"

#include <QApplication>
#include <QFont>

#include <filesystem>

using StdFsPath = std::filesystem::path;

void appSetup();
StdFsPath pathArg(QApplication& application);
void setFont(QApplication& application);

int main(int argc, char* argv[])
{
	auto main_window_name = "MainWindow";

	LaunchCop launch_cop("Fernanda", main_window_name);
	if (launch_cop.isRunning())
		return 0;

	appSetup();
	QApplication fernanda(argc, argv);
	setFont(fernanda);

	MainWindow main_window(main_window_name,
		fernanda.arguments().contains("-dev"),
		pathArg(fernanda));

	QObject::connect(&launch_cop, &LaunchCop::launchedAgain,
		&main_window, &MainWindow::onSecondLaunch);

	main_window.show();

	Logger::install(main_window.userData());
	Utility::ensureAppVisible(main_window);

	return fernanda.exec();
}

void appSetup()
{
	QApplication::setHighDpiScaleFactorRoundingPolicy(
		Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
	QApplication::setDesktopSettingsAware(true);
}

StdFsPath pathArg(QApplication& application)
{
	StdFsPath path_arg;
	for (auto& arg : application.arguments())
		if (arg.endsWith(".txt")) // handle projects, too
			path_arg = Path::toStdFs(arg);
	return path_arg;
}

void setFont(QApplication& application)
{
	auto font = application.font();
	font.setStyleStrategy(QFont::PreferAntialias);
	font.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
	font.setPointSizeF(9);
	application.setFont(font);
}
