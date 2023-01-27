// path.h, Fernanda

#pragma once

#include <filesystem>
#include <string>
#include <type_traits>

#include <qsystemdetection.h>

#include <QDir>
#include <QtGlobal>
#include <QString>

namespace Path
{
	namespace StdFs = std::filesystem;

	enum class Type {
		Dir,
		File
	};

	inline StdFs::path toStdFs(QString qStringPath)
	{
		return StdFs::path(qStringPath.toStdString());
	}

	inline StdFs::path toStdFs(QVariant qVariantPath)
	{
		return StdFs::path(qVariantPath.toString().toStdString());
	}

	inline QString toQString(StdFs::path path, bool sanitize = false)
	{
		auto result = QString::fromStdString(path.make_preferred().string());
		if (sanitize)
			result.replace(R"(\)", R"(/)");
		return result;
	}

#if QT_VERSION > QT_VERSION_CHECK(6, 2, 4)

	inline void copy(StdFs::path fileName, StdFs::path newName)
	{
		QFile::copy(fileName, newName);
	}

#else

	inline void copy(StdFs::path fileName, StdFs::path newName)
	{
		QFile::copy(toQString(fileName), toQString(newName));
	}

#endif

#ifdef Q_OS_LINUX

	inline std::string toBit7z(StdFs::path path)
	{
		return path.string();
	}

#else

	inline std::string toBit7z(StdFs::path path)
	{
		auto result = toQString(path);
		result.replace(R"(/)", R"(\)");
		return result.toStdString();
	}

#endif

	inline void makeParent(StdFs::path path)
	{
		auto parent = path.parent_path();
		if (QDir(parent).exists()) return;
		StdFs::create_directories(parent);
	}

	template<typename T, typename U>
	inline const T getName(U path)
	{
		if constexpr (std::is_same<T, QString>::value && std::is_same<U, QString>::value)
			return QString::fromStdString(toStdFs(path).stem().string());
		if constexpr (std::is_same<T, QString>::value && std::is_same<U, StdFs::path>::value)
			return QString::fromStdString(path.stem().string());
		if constexpr (std::is_same<T, StdFs::path>::value && std::is_same<U, QString>::value)
			return toStdFs(path).stem();
		if constexpr (std::is_same<T, StdFs::path>::value && std::is_same<U, StdFs::path>::value)
			return path.stem();
	}

	inline void makeDirs(StdFs::path dirPath)
	{
		if (QDir(dirPath).exists()) return;
		StdFs::create_directories(dirPath);
	}
}

// path.h, Fernanda
