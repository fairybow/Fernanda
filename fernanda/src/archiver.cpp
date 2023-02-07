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

// archiver.cpp, Fernanda

#include "archiver.h"

using namespace bit7z;

void Archiver::create(StdFsPath archivePath, QVector<Io::ArchiveWriteReadPaths> writeReadPaths)
{
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toStdFs(temp_dir.path());
	for (auto& entry : writeReadPaths)
	{
		auto temp_write_path = temp_dir_path / entry.writeRelPath;
		if (!entry.readFullPath.has_value())
			Path::makeDirs(temp_write_path);
		else
		{
			Path::makeDirs(temp_write_path.parent_path());
			Path::copy(entry.readFullPath.value(), temp_write_path);
			QFile(temp_write_path).setPermissions(QFile::ReadUser | QFile::WriteUser);
		}
	}
	Bit7zLibrary library{ UserData::dll() };
	BitFileCompressor compressor{ library, archiveFormat };
	compressor.setCompressionLevel(compressionLevel);
	compressor.compressDirectory(Path::toBit7z(temp_dir_path / Io::storyRoot), archivePath.string());
}

const QString Archiver::read(StdFsPath archivePath, StdFsPath readPath)
{
	QString result = nullptr;
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toStdFs(temp_dir.path());
	auto was_found = extractMatch(archivePath, readPath, temp_dir_path);
	if (was_found)
		result = Io::readFile(temp_dir_path / readPath);
	return result;
}

bool Archiver::extractMatch(StdFsPath archivePath, StdFsPath relativePath, StdFsPath extractPath)
{
	try {
		Bit7zLibrary library{ UserData::dll() };
		BitFileExtractor extractor{ library, archiveFormat };
		extractor.extractMatching(Path::toBit7z(archivePath), Path::toBit7z(relativePath), Path::toBit7z(extractPath));
	}
	catch (const BitException&) {
		return false;
	}
	return true;
}

void Archiver::extract(StdFsPath archivePath, StdFsPath extractPath)
{
	Bit7zLibrary library{ UserData::dll() };
	BitFileExtractor extractor{ library, archiveFormat };
	extractor.extract(Path::toBit7z(archivePath), Path::toBit7z(extractPath));
}

void Archiver::add(StdFsPath archivePath, StdFsPath readPath, StdFsPath writePath)
{
	Bit7zLibrary library{ UserData::dll() };
	BitFileCompressor compressor{ library, archiveFormat };
	compressor.setCompressionLevel(compressionLevel);
	compressor.setUpdateMode(UpdateMode::Update);
	std::map<std::string, std::string> in_map;
	in_map[readPath.string()] = Path::toBit7z(writePath);
	compressor.compress(in_map, Path::toBit7z(archivePath));
}

void Archiver::add(StdFsPath archivePath, QVector<Io::ArchiveWriteReadPaths> writeReadPaths)
{
	Bit7zLibrary library{ UserData::dll() };
	BitFileCompressor compressor{ library, archiveFormat };
	compressor.setCompressionLevel(compressionLevel);
	compressor.setUpdateMode(UpdateMode::Update);
	std::map<std::string, std::string> in_map;
	for (auto& wr_path : writeReadPaths)
		in_map[wr_path.readFullPath.value().string()] = Path::toBit7z(wr_path.writeRelPath);
	compressor.compress(in_map, Path::toBit7z(archivePath));
}

void Archiver::add(StdFsPath archivePath, Io::ArchiveWrite textAndWritePath)
{
	QTemporaryDir temp_dir;
	auto& w_path = textAndWritePath.writeRelPath;
	auto temp_path = Path::toStdFs(temp_dir.path()) / Path::getName<StdFsPath>(w_path);
	Io::writeFile(temp_path, textAndWritePath.text);
	Bit7zLibrary library{ UserData::dll() };
	BitFileCompressor compressor{ library, archiveFormat };
	compressor.setCompressionLevel(compressionLevel);
	compressor.setUpdateMode(UpdateMode::Update);
	std::map<std::string, std::string> in_map;
	in_map[temp_path.string()] = Path::toBit7z(w_path);
	compressor.compress(in_map, Path::toBit7z(archivePath));
}

void Archiver::save(StdFsPath archivePath, QVector<Io::ArchiveRename> renamePaths)
{
	std::map<std::string, Path::Type> additions;
	std::map<std::string, std::string> renames;
	for (auto& entry : renamePaths)
	{
		if (entry.typeIfNewOrCut.has_value())
			additions[entry.relativePath.string()] = entry.typeIfNewOrCut.value();
		else
			renames[entry.relativePath.string()] = entry.originalRelativePath.value().string();
	}
	if (!renames.empty())
		rename(archivePath, renames);
	if (!additions.empty())
		blanks(archivePath, additions);
}

void Archiver::cut(StdFsPath archivePath, QVector<Io::ArchiveRename> cuts)
{
	std::map<std::string, std::string> in_map;
	std::vector<std::string> cut_folders;
	for (auto& cut : cuts)
	{
		if (!cut.originalRelativePath.has_value()) continue;
		auto& relative_path = cut.originalRelativePath.value();
		auto relative_path_string = relative_path.string();
		if (cut.typeIfNewOrCut != Path::Type::Dir)
		{
			auto cut_path = Path::toStdFs((Path::getName<QString>(relative_path) + Io::tempExtension));
			in_map[StdFsPath(".cut" / cut_path).string() ] = relative_path_string;
		}
		else
			cut_folders << relative_path_string;
	}
	rename(archivePath, in_map);
	if (!cut_folders.empty())
		del(archivePath, cut_folders);
}

void Archiver::rename(StdFsPath archivePath, std::map<std::string, std::string> renames)
{
	Bit7zLibrary library{ UserData::dll() };
	BitArchiveEditor editor{ library, Path::toBit7z(archivePath), archiveFormat };
	for (const auto& [key, value] : renames)
		editor.renameItem(Path::toBit7z(value), Path::toBit7z(key));
	editor.applyChanges();
}

void Archiver::del(StdFsPath archivePath, std::vector<std::string> relativePaths)
{
	Bit7zLibrary library{ UserData::dll() };
	BitArchiveEditor editor{ library, Path::toBit7z(archivePath), archiveFormat };
	for (const auto& relative_path : relativePaths)
		editor.deleteItem(Path::toBit7z(relative_path));
	editor.applyChanges();
}

void Archiver::blanks(StdFsPath archivePath, std::map<std::string, Path::Type> additions)
{
	Bit7zLibrary library{ UserData::dll() };
	BitFileCompressor compressor{ library, archiveFormat };
	compressor.setCompressionLevel(compressionLevel);
	compressor.setUpdateMode(UpdateMode::Update);
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toStdFs(temp_dir.path());
	for (const auto& [key, value] : additions)
	{
		auto temp_r_path = temp_dir_path / key;
		if (value == Path::Type::Dir)
			Path::makeDirs(temp_r_path);
		else
		{
			Path::makeDirs(temp_r_path.parent_path());
			Io::writeFile(temp_r_path, nullptr);
		}
	}
	compressor.compressDirectory(Path::toBit7z(temp_dir_path / Io::storyRoot), Path::toBit7z(archivePath));
}

// archiver.cpp, Fernanda
