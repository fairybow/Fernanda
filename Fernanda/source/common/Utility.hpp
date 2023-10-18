#pragma once

#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>
#include <QTimer>
#include <QVector>

namespace Utility
{
	template<typename Lambda>
	inline void delayCall(const QObject* context, Lambda lambda)
	{
		QTimer::singleShot(0, context, lambda);
	}

	template<typename... Calls>
	inline void delayCalls(const QObject* context, Calls... call)
	{
		(QTimer::singleShot(0, context, call), ...);
	}

	inline int greaterOrEqual(int value, int mustExceed)
	{
		return (value < mustExceed) ? mustExceed : value;
	}

	inline void ensureAppVisible(QMainWindow& mainWindow)
	{
		auto screens = QGuiApplication::screens();
		if (screens.isEmpty()) return;
		auto visible = false;
		for (auto& screen : screens) {
			auto rect = screen->geometry();
			if (rect.contains(mainWindow.geometry())) {
				visible = true;
				break;
			}
		}
		if (visible) return;
		auto rect = screens.first()->geometry();
		mainWindow.move(rect.topLeft());
	}
}
