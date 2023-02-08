/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Sample.h, Fernanda

#pragma once

#include "Io.h"

#include <QDirIterator>
#include <QStringList>
#include <QVector>

namespace Sample
{
	namespace StdFs = std::filesystem;

	struct SampleRCPair {
		StdFs::path resourcePath;
		StdFs::path userDataPath;
	};

	inline QVector<Io::ArchiveWriteReadPaths> make()
	{
		QVector<Io::ArchiveWriteReadPaths> result;
		auto rootPath = ":/sample/Candide/";
		QDirIterator it(rootPath, QStringList() << "*.*", QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			it.next();
			auto read_path = Path::toStdFs(it.filePath());
			auto relative_path = StdFs::relative(read_path, rootPath);
			it.fileInfo().isDir()
				? result << Io::ArchiveWriteReadPaths{ Io::storyRoot / relative_path }
				: result << Io::ArchiveWriteReadPaths{ Io::storyRoot / relative_path, read_path };
		}
		return result;
	}

	inline void makeRc(StdFs::path path)
	{
		QVector<SampleRCPair> pairs{
			SampleRCPair{ ":/sample/Dot Matrix.ttf", StdFs::path(path / "Dot Matrix.ttf") },
			SampleRCPair{ ":/sample/Sample.fernanda_editor", StdFs::path(path / "Sample.fernanda_editor") },
			SampleRCPair{ ":/sample/Sample.fernanda_window", StdFs::path(path / "Sample.fernanda_window") }
		};
		for (auto& pair : pairs)
		{
			auto& source = pair.resourcePath;
			auto& target = pair.userDataPath;
			auto q_file_target = QFile(target);
			if (q_file_target.exists())
				q_file_target.moveToTrash();
			Path::copy(source, target);
			q_file_target.setPermissions(QFile::ReadUser | QFile::WriteUser);
		}
	}
}

// Sample.h, Fernanda
