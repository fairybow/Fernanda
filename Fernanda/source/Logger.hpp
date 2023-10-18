#pragma once

#include "common/StringTools.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

#include <filesystem>

namespace Logger
{
	namespace
	{
		using StdFsPath = std::filesystem::path;

		static bool isInitialized = false;
		static QFile log;
		static QTextStream logStream(&log);

		inline void toConsole(const QString& message)
		{

#ifdef Q_OS_WINDOWS

			auto wstring = message.toStdWString();
			OutputDebugStringW(wstring.c_str());
			OutputDebugStringW(L"\n");

#endif

			std::fprintf(stderr, "%s\n", message.toLocal8Bit().constData());
		}

		inline void clearOnFirstWrite()
		{
			log.close();
			log.open(QIODevice::WriteOnly);
			log.close();
			log.open(QIODevice::WriteOnly | QIODevice::Append);
			isInitialized = true;
		}

		inline void timestamp(bool force = false)
		{
			static QString last_timestamp;
			auto timestamp = StringTools::time();

			if (last_timestamp != timestamp || force) {
				logStream << StringTools::flank(timestamp, 50,
					StringTools::Side::Left, '=', true)
					<< "\n" << Qt::endl;
				last_timestamp = timestamp;
			}
		}

		inline void toLog(const QString& message)
		{
			if (!isInitialized)
				clearOnFirstWrite();
			timestamp();
			logStream << StringTools::fixEscapes(message)
				<< "\n" << Qt::endl;
		}

		inline void passthrough(QtMsgType type, const QMessageLogContext& context, const QString& message)
		{
			toConsole(message);
			toLog(message);
		}
	}

	void install(const StdFsPath& path)
	{
		log.setFileName(path / "Debug.log");
		log.open(QIODevice::WriteOnly | QIODevice::Append);
		qInstallMessageHandler(passthrough);
	}
}
