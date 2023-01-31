/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

// path.h, Fernanda

#pragma once

#include <QDir>
#include <QtGlobal>
#include <QString>
#include <qsystemdetection.h>
#include <QVariant>

#include <filesystem>
#include <string>
#include <type_traits>

namespace Path
{
	namespace StdFs = std::filesystem;

	enum class Type {
		Dir,
		File
	};

	inline QString toQString(StdFs::path path, bool sanitize = false)
	{
		auto result = QString::fromStdString(path.make_preferred().string());
		if (sanitize)
			result.replace(R"(\)", R"(/)");
		return result;
	}

#ifdef Q_OS_LINUX

	inline std::string toBit7z(StdFs::path path) { return path.string(); }

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

	inline void makeDirs(StdFs::path dirPath)
	{
		if (QDir(dirPath).exists()) return;
		StdFs::create_directories(dirPath);
	}

	inline StdFs::path toStdFs(QString qStringPath) { return StdFs::path(qStringPath.toStdString()); }
	inline StdFs::path toStdFs(QVariant qVariantPath) { return StdFs::path(qVariantPath.toString().toStdString()); }

#if QT_VERSION > QT_VERSION_CHECK(6, 2, 4)

	inline void copy(StdFs::path fileName, StdFs::path newName) { QFile::copy(fileName, newName); }

#else

	inline void copy(StdFs::path fileName, StdFs::path newName) { QFile::copy(toQString(fileName), toQString(newName)); }

#endif

	template<typename T, typename U>
	inline const T getName(U path, bool keepExtension = false)
	{
		T result{};
		if constexpr (std::is_same<T, QString>::value && std::is_same<U, QString>::value)
		{
			(keepExtension)
				? result = QString::fromStdString(toStdFs(path).filename().string())
				: result = QString::fromStdString(toStdFs(path).stem().string());
		}
		if constexpr (std::is_same<T, QString>::value && std::is_same<U, StdFs::path>::value)
		{
			(keepExtension)
				? result = QString::fromStdString(path.filename().string())
				: result = QString::fromStdString(path.stem().string());
		}
		if constexpr ((std::is_same<T, StdFs::path>::value && std::is_same<U, QString>::value) || (std::is_same<T, StdFs::path>::value && std::is_same<U, QVariant>::value))
		{
			(keepExtension)
				? result = toStdFs(path).filename()
				: result = toStdFs(path).stem();
		}
		if constexpr (std::is_same<T, StdFs::path>::value && std::is_same<U, StdFs::path>::value)
		{
			(keepExtension)
				? result = path.filename()
				: result = path.stem();
		}
		return result;
	}
}

// path.h, Fernanda
