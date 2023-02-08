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

// Archiver.h, Fernanda

#pragma once

#include "Io.h"
#include "UserData.h"

#include "bit7z/include/bit7z.hpp"
#include "bit7z/include/bitarchiveeditor.hpp"
#include <QTemporaryDir>

#include <map>
#include <vector>

inline void operator<<(std::vector<std::string>& lhs, const std::string& rhs) { return lhs.push_back(rhs); }

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

// Archiver.h, Fernanda
