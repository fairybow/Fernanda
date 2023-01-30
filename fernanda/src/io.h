/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// io.h, Fernanda

#pragma once

#include "path.h"

#include <optional>
#include <utility>

#include <QFile>
#include <QIODevice>
#include <QTextStream>

namespace Io
{
	namespace StdFs = std::filesystem;

	enum class Move {
		Above,
		Below,
		On,
		Viewport
	};

	struct ArchiveWriteReadPaths {
		StdFs::path writeRelPath;
		std::optional<StdFs::path> readFullPath;
	};
	struct ArchiveWrite {
		QString text;
		StdFs::path writeRelPath;
	};
	struct ArchiveRename {
		QString key;
		StdFs::path relativePath;
		std::optional<StdFs::path> originalRelativePath;
		std::optional<Path::Type> typeIfNewOrCut;
	};

	const StdFs::path storyRoot = QStringLiteral("story").toStdString();
	const QString extension = QStringLiteral(".txt");
	const QString tempExtension = QStringLiteral(".txt~");

	inline const QString readFile(StdFs::path filePath)
	{
		QString text;
		QFile file(filePath);
		if (file.open(QFile::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);
			text = in.readAll();
			file.close();
		}
		return text;
	}

	inline void writeFile(StdFs::path filePath, QString text)
	{
		Path::makeParent(filePath);
		QFile file(filePath);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream out(&file);
			out << text;
			file.close();
		}
	}
}

// io.h, Fernanda
