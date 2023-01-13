// archiver.cpp, Fernanda

#include "archiver.h"

using namespace bit7z;

void Archiver::create(FsPath arcPath, QVector<Io::ArcWRPaths> wRPaths)
{
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toFs(temp_dir.path());
	for (auto& entry : wRPaths)
	{
		auto temp_w_path = temp_dir_path / entry.writeRelPath;
		if (!entry.readFullPath.has_value())
			Path::makeDirs(temp_w_path);
		else
		{
			Path::makeDirs(temp_w_path.parent_path());
			QFile::copy(entry.readFullPath.value(), temp_w_path);
			QFile(temp_w_path).setPermissions(QFile::ReadUser | QFile::WriteUser);
		}
	}
	Bit7zLibrary lib{ Ud::dll() };
	BitFileCompressor compressor{ lib, format };
	compressor.setCompressionLevel(level);
	compressor.compressDirectory(Path::toB7z(temp_dir_path / Io::storyRoot), arcPath.string());
}

const QString Archiver::read(FsPath arcPath, FsPath rPath)
{
	QString result = nullptr;
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toFs(temp_dir.path());
	auto was_found = extractMatch(arcPath, rPath, temp_dir_path);
	if (was_found)
		result = Io::readFile(temp_dir_path / rPath);
	return result;
}

bool Archiver::extractMatch(FsPath arcPath, FsPath relPath, FsPath extractPath)
{
	try {
		Bit7zLibrary lib{ Ud::dll() };
		BitFileExtractor extractor{ lib, format };
		extractor.extractMatching(Path::toB7z(arcPath), Path::toB7z(relPath), Path::toB7z(extractPath));
	}
	catch (const BitException&) {
		return false;
	}
	return true;
}

void Archiver::extract(FsPath arcPath, FsPath exPath)
{
	Bit7zLibrary lib{ Ud::dll() };
	BitFileExtractor extractor{ lib, format };
	extractor.extract(Path::toB7z(arcPath), Path::toB7z(exPath));
}

void Archiver::add(FsPath arcPath, FsPath rPath, FsPath wPath)
{
	Bit7zLibrary lib{ Ud::dll() };
	BitFileCompressor compressor{ lib, format };
	compressor.setCompressionLevel(level);
	compressor.setUpdateMode(UpdateMode::Overwrite);
	std::map<std::string, std::string> in_map;
	in_map[rPath.string()] = Path::toB7z(wPath);
	compressor.compress(in_map, Path::toB7z(arcPath));
}

void Archiver::add(FsPath arcPath, QVector<Io::ArcWRPaths> wRPaths)
{
	Bit7zLibrary lib{ Ud::dll() };
	BitFileCompressor compressor{ lib, format };
	compressor.setCompressionLevel(level);
	compressor.setUpdateMode(UpdateMode::Overwrite);
	std::map<std::string, std::string> in_map;
	for (auto& wr_path : wRPaths)
		in_map[wr_path.readFullPath.value().string()] = Path::toB7z(wr_path.writeRelPath);
	compressor.compress(in_map, Path::toB7z(arcPath));
}

void Archiver::add(FsPath arcPath, Io::ArcWrite textAndWPath)
{
	QTemporaryDir temp_dir;
	auto& w_path = textAndWPath.writeRelPath;
	auto temp_path = Path::toFs(temp_dir.path()) / Path::getName<FsPath>(w_path);
	Io::writeFile(temp_path, textAndWPath.text);
	Bit7zLibrary lib{ Ud::dll() };
	BitFileCompressor compressor{ lib, format };
	compressor.setCompressionLevel(level);
	compressor.setUpdateMode(UpdateMode::Overwrite);
	std::map<std::string, std::string> in_map;
	in_map[temp_path.string()] = Path::toB7z(w_path);
	compressor.compress(in_map, Path::toB7z(arcPath));
}

void Archiver::save(FsPath arcPath, QVector<Io::ArcRename> renamePaths)
{
	std::map<std::string, Path::Type> additions;
	std::map<std::string, std::string> renames;
	for (auto& entry : renamePaths)
	{
		if (entry.typeIfNewOrCut.has_value())
			additions[entry.relPath.string()] = entry.typeIfNewOrCut.value();
		else
			renames[entry.relPath.string()] = entry.origRelPath.value().string();
	}
	if (!renames.empty())
		rename(arcPath, renames);
	if (!additions.empty())
		blanks(arcPath, additions);
}

void Archiver::cut(FsPath arcPath, QVector<Io::ArcRename> cuts)
{
	std::map<std::string, std::string> in_map;
	std::vector<std::string> cut_folders;
	for (auto& cut : cuts)
	{
		if (!cut.origRelPath.has_value()) continue;
		auto& rel_path = cut.origRelPath.value();
		auto rel_path_str = rel_path.string();
		if (cut.typeIfNewOrCut != Path::Type::Dir)
		{
			auto cut_path = Path::toFs((Path::getName<QString>(rel_path) + Io::tempExt));
			in_map[FsPath(".cut" / cut_path).string() ] = rel_path_str;
		}
		else
			cut_folders << rel_path_str;
	}
	rename(arcPath, in_map);
	if (!cut_folders.empty())
		del(arcPath, cut_folders);
}

void Archiver::rename(FsPath arcPath, std::map<std::string, std::string> renames)
{
	Bit7zLibrary lib{ Ud::dll() };
	BitArchiveEditor editor{ lib, Path::toB7z(arcPath), format };
	for (const auto& [key, value] : renames)
		editor.renameItem(Path::toB7z(value), Path::toB7z(key));
	editor.applyChanges();
}

void Archiver::del(FsPath arcPath, std::vector<std::string> relPaths)
{
	Bit7zLibrary lib{ Ud::dll() };
	BitArchiveEditor editor{ lib, Path::toB7z(arcPath), format };
	for (const auto& rel_path : relPaths)
		editor.deleteItem(Path::toB7z(rel_path));
	editor.applyChanges();
}

void Archiver::blanks(FsPath arcPath, std::map<std::string, Path::Type> additions)
{
	Bit7zLibrary lib{ Ud::dll() };
	BitFileCompressor compressor{ lib, format };
	compressor.setCompressionLevel(level);
	compressor.setUpdateMode(UpdateMode::Overwrite);
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toFs(temp_dir.path());
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
	compressor.compressDirectory(Path::toB7z(temp_dir_path / Io::storyRoot), Path::toB7z(arcPath));
}

// archiver.cpp, Fernanda
