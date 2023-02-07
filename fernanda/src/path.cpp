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

 // path.cpp, Fernanda

#include "path.h"

#if QT_VERSION > QT_VERSION_CHECK(6, 2, 4)

void Path::copy(StdFs::path fileName, StdFs::path newName) { QFile::copy(fileName, newName); }

#else

void Path::copy(StdFs::path fileName, StdFs::path newName) { QFile::copy(toQString(fileName), toQString(newName)); }

#endif

void Path::makeDirs(StdFs::path dirPath)
{
	if (QDir(dirPath).exists()) return;
	StdFs::create_directories(dirPath);
}

void Path::makeParent(StdFs::path path)
{
	auto parent = path.parent_path();
	if (QDir(parent).exists()) return;
	StdFs::create_directories(parent);
}

#ifdef Q_OS_LINUX

std::string Path::toBit7z(StdFs::path path) { return path.string(); }

#else

std::string Path::toBit7z(StdFs::path path)
{
	auto result = toQString(path);
	result.replace(R"(/)", R"(\)");
	return result.toStdString();
}

#endif

QString Path::toQString(StdFs::path path, bool sanitize)
{
	auto result = QString::fromStdString(path.make_preferred().string());
	if (sanitize)
		result.replace(R"(\)", R"(/)");
	return result;
}

// path.cpp, Fernanda
