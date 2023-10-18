#pragma once

#include <QDirIterator>
#include <QString>
#include <QVariant>
#include <QVector>

#include <algorithm>
#include <filesystem>
#include <type_traits>

namespace Path
{
	namespace
	{
		namespace StdFs = std::filesystem;
		using StdFsPath = StdFs::path;
		using StdFsPathList = QVector<StdFs::path>;

		template <typename T>
		using IsFsPathOrQString = std::disjunction<std::is_same<T, StdFsPath>, std::is_same<T, QString>>;

		inline StdFsPath pathOrParent(StdFsPath path, bool hasFileName)
		{
			return hasFileName ? path.parent_path() : path;
		}

		// combine?:

		/*inline QStringList alphabetize(const QStringList& qStringPaths)
		{
			QStringList sorted_paths = qStringPaths;
			std::sort(sorted_paths.begin(), sorted_paths.end(), [](auto& lhs, auto& rhs) {
				return lhs < rhs;
				});
			return sorted_paths;
		}*/

		inline StdFsPathList alphabetize(const StdFsPathList& paths)
		{
			StdFsPathList sorted_paths = paths;
			std::sort(sorted_paths.begin(), sorted_paths.end(), [](auto& lhs, auto& rhs) {
				return lhs < rhs;
				});
			return sorted_paths;
		}
	}

	inline bool isValid(StdFsPath path)
	{
		return (!path.empty() && StdFs::exists(path));
	}

	template<typename... Paths>
	inline bool areValid(Paths... path)
	{
		return (isValid(path) && ...);
	}

	inline StdFsPath toStdFs(QString qStringPath)
	{
		return StdFsPath(qStringPath.toStdString());
	}

	inline StdFsPath toStdFs(QVariant qVariantPath)
	{
		return toStdFs(qVariantPath.toString());
	}

	inline StdFsPath toStdFs(const char* cStringPath)
	{
		return StdFsPath(cStringPath);
	}

	inline QString toQString(StdFsPath path, bool sanitize = true)
	{
		auto q_path = QString::fromStdString(path.make_preferred().string());
		if (sanitize)
			q_path.replace(R"(\)", R"(/)");
		return q_path;
	}

	inline void make(const StdFsPath& path, bool includesFileName = false)
	{
		auto directory = pathOrParent(path, includesFileName);
		if (!StdFs::exists(directory))
			StdFs::create_directories(directory);
	}

	inline void clear(const StdFsPath& path, bool clearSelf = false, bool includesFileName = false)
	{
		auto directory = pathOrParent(path, includesFileName);
		if (!StdFs::exists(directory)) return;
		for (auto& item : StdFs::directory_iterator(directory))
			StdFs::remove_all(item);
		if (clearSelf)
			StdFs::remove(directory);
	}

	inline StdFsPath name(const StdFsPath& path, bool keepExtension = false)
	{
		return keepExtension ? path.filename() : path.stem();
	}

	inline StdFsPath name(const QString& qStringPath, bool keepExtension = false)
	{
		return name(toStdFs(qStringPath), keepExtension);
	}

	inline StdFsPath parentName(const StdFsPath& path, bool keepExtension = false)
	{
		return name(path.parent_path(), keepExtension);
	}

	inline StdFsPath parentName(const QString& qStringPath, bool keepExtension = false)
	{
		return name(toStdFs(qStringPath).parent_path(), keepExtension);
	}

	template<typename T>
	inline typename std::enable_if<IsFsPathOrQString<T>::value, QString>::type
		qStringName(const T& path, bool keepExtension = false)
	{
		return QString::fromStdString(name(path, keepExtension).string());
	}

	inline QString qStringParentName(const StdFsPath& path, bool keepExtension = false)
	{
		return qStringName(path.parent_path(), keepExtension);
	}

	inline QString qStringParentName(const QString& qStringPath, bool keepExtension = false)
	{
		return qStringName(toStdFs(qStringPath).parent_path(), keepExtension);
	}

	inline QStringList gatherQStringFilePaths(const QStringList& qStringPaths, const QStringList& extensions)
	{
		QStringList entries;
		for (auto& path : qStringPaths) {
			QDirIterator it(path, extensions, QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				it.next();
				entries << it.filePath();
			}
		}
		return entries;
	}

	inline QStringList gatherQStringFilePaths(const QStringList& qStringPaths, const QString& extension)
	{
		return gatherQStringFilePaths(qStringPaths, QStringList{ extension });
	}

	inline QStringList gatherQStringFilePaths(const QString& qStringPath, const QStringList& extensions)
	{
		return gatherQStringFilePaths(QStringList{ qStringPath }, extensions);
	}

	inline QStringList gatherQStringFilePaths(const QString& qStringPath, const QString& extension)
	{
		return gatherQStringFilePaths(QStringList{ qStringPath }, QStringList{ extension });
	}

	inline QStringList gatherQStringFilePaths(const StdFsPathList& paths, const QStringList& extensions)
	{
		QStringList path_list;
		for (auto& path : paths)
			path_list << toQString(path);
		return gatherQStringFilePaths(path_list, extensions);
	}

	inline QStringList gatherQStringFilePaths(const StdFsPathList& paths, const QString& extension)
	{
		return gatherQStringFilePaths(paths, QStringList{ extension });
	}

	inline QStringList gatherQStringFilePaths(const StdFsPath& path, const QStringList& extensions)
	{
		return gatherQStringFilePaths(StdFsPathList{ path }, extensions);
	}

	inline QStringList gatherQStringFilePaths(const StdFsPath& path, const QString& extension)
	{
		return gatherQStringFilePaths(StdFsPathList{ path }, QStringList{ extension });
	}

	inline StdFsPathList gatherFilePaths(const StdFsPathList& paths, const QStringList& extensions)
	{
		auto path_list = gatherQStringFilePaths(paths, extensions);
		StdFsPathList converted_path_list;
		for (auto& path : path_list)
			converted_path_list << toStdFs(path);
		return alphabetize(converted_path_list);
	}

	inline StdFsPathList gatherFilePaths(const StdFsPathList& paths, const QString& extension)
	{
		return gatherFilePaths(paths, QStringList{ extension });
	}

	inline StdFsPathList gatherFilePaths(const StdFsPath& path, const QStringList& extensions)
	{
		return gatherFilePaths(StdFsPathList{ path }, extensions);
	}

	inline StdFsPathList gatherFilePaths(const StdFsPath& path, const QString& extension)
	{
		return gatherFilePaths(StdFsPathList{ path }, QStringList{ extension });
	}

#if QT_VERSION > QT_VERSION_CHECK(6, 2, 4)

	inline void copy(const StdFsPath& extantPath, const StdFsPath& newPath)
	{
		QFile::copy(extantPath, newPath);
	}

#else

	inline void copy(const StdFsPath& extantPath, const StdFsPath& newPath)
	{
		QFile::copy(toQString(extantPath), toQString(newPath));
	}

#endif

	inline bool move(const StdFsPath& extantPath, const StdFsPath& newPath, bool overwrite = false)
	{
		if (StdFs::exists(newPath) && overwrite)
			StdFs::remove(newPath);
		return QFile::rename(extantPath, newPath);
	}
}
