#pragma once

#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

#include <filesystem>

namespace Io
{
	namespace
	{
		namespace StdFs = std::filesystem;
		using StdFsPath = StdFs::path;
	}

	inline const QString readFile(StdFsPath filePath)
	{
		QString text;
		QFile file(filePath);
		if (file.open(QFile::ReadOnly | QIODevice::Text)) {
			QTextStream in(&file);
			text = in.readAll();
		}
		return text;
	}

	template<typename... Strings>
	inline void toStrings(StdFsPath filePath, Strings&... string)
	{
		auto text = readFile(filePath);
		((string = text), ...);
	}

	inline bool writeFile(StdFsPath filePath, QString text = QString(), bool createDirectories = true)
	{
		if (createDirectories) {
			auto parent = filePath.parent_path();
			if (!StdFs::exists(parent))
				StdFs::create_directories(parent);
		}
		QFile file(filePath);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&file);
			out << text;
			return true;
		}
		return false;
	}
}
