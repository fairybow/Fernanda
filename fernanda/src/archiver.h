// archiver.h, Fernanda

#pragma once

#include "bit7z/include/bit7z.hpp"
#include "bit7z/include/bitarchiveeditor.hpp"

#include "io.h"
#include "userdata.h"

#include <map>
#include <vector>

#include <QTemporaryDir>

inline void operator<<(std::vector<std::string>& lhs, const std::string& rhs)
{
    return lhs.push_back(rhs);
}

class Archiver
{
    using StdFsPath = std::filesystem::path;

public:
    void create(StdFsPath archivePath, QVector<Io::ArchiveWriteReadPaths> writeReadPaths);
    const QString read(StdFsPath archivePath, StdFsPath readPath);
    bool extractMatch(StdFsPath archivePath, StdFsPath relativePath, StdFsPath extractPath);
    void extract(StdFsPath archivePath, StdFsPath extractPath);
    void add(StdFsPath archivePath, StdFsPath readPath, StdFsPath writePath);
    void add(StdFsPath archivePath, QVector<Io::ArchiveWriteReadPaths> writeReadPaths);
    void add(StdFsPath archivePath, Io::ArchiveWrite textAndWritePath);
    void save(StdFsPath archivePath, QVector<Io::ArchiveRename> renamePaths);
    void cut(StdFsPath archivePath, QVector<Io::ArchiveRename> cuts);

private:
    const bit7z::BitInOutFormat& archiveFormat = bit7z::BitFormat::SevenZip;
    bit7z::BitCompressionLevel compressionLevel = bit7z::BitCompressionLevel::None;

    void rename(StdFsPath archivePath, std::map<std::string, std::string> renames);
    void del(StdFsPath archivePath, std::vector<std::string> relativePaths);
    void blanks(StdFsPath archivePath, std::map<std::string, Path::Type> additions);
};

// archiver.h, Fernanda
