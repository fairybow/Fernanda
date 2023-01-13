// sample.h, Fernanda

#pragma once

#include "io.h"

#include <QDirIterator>
#include <QStringList>
#include <QVector>

namespace Sample
{
	namespace Fs = std::filesystem;

	struct SampleRCPair {
		Fs::path resourcePath;
		Fs::path userDataPath;
	};

	inline QVector<Io::ArcWRPaths> make()
	{
		QVector<Io::ArcWRPaths> result;
		auto rootPath = ":/sample/Candide/";
		QDirIterator it(rootPath, QStringList() << "*.*", QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			it.next();
			auto read_path = Path::toFs(it.filePath());
			auto rel_path = Fs::relative(read_path, rootPath);
			(it.fileInfo().isDir())
				? result << Io::ArcWRPaths{ Io::storyRoot / rel_path }
				: result << Io::ArcWRPaths{ Io::storyRoot / rel_path, read_path };
		}
		return result;
	}

	inline void makeRc(Fs::path path)
	{
		QVector<SampleRCPair> pairs{
			SampleRCPair{ ":/sample/Dot Matrix.ttf", Fs::path(path / "Dot Matrix.ttf") },
			SampleRCPair{ ":/sample/Sample.fernanda_theme", Fs::path(path / "Sample.fernanda_theme") },
			SampleRCPair{ ":/sample/Sample.fernanda_wintheme", Fs::path(path / "Sample.fernanda_wintheme") }
		};
		for (auto& pair : pairs)
		{
			auto& source = pair.resourcePath;
			auto& target = pair.userDataPath;
			auto qf_target = QFile(target);
			if (qf_target.exists())
				qf_target.moveToTrash();
			QFile::copy(source, target);
			qf_target.setPermissions(QFile::ReadUser | QFile::WriteUser);
		}
	}
}

// sample.h, Fernanda
