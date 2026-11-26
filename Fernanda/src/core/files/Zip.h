/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstddef>
#include <optional>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QSaveFile>
#include <QString>
#include <QtTypes>

#include <miniz.h>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/files/Io.h"

// Zip archives, as stateless free functions over miniz. miniz is built without
// its own file functions, so every archive is read through a QFile and written
// through a QSaveFile, via miniz's read and write callbacks
namespace Fernanda::Zip {

// A file on disk and the name it takes inside the archive: relative, with
// "/" between folders
struct Entry
{
    Coco::Path source{};
    QString name{};
};

namespace Internal {

// The largest entry readEntry will decompress. Archives read this way come
// from outside Fernanda, and the size is checked against what the archive
// declares, before anything is allocated
constexpr qint64 MAX_ENTRY_SIZE_ = 256LL * 1024 * 1024;

// miniz asks for reads and writes at file offsets. It writes an entry's
// header, then its data, then goes back to fill the header in, so the write
// side seeks too. Either returns the bytes moved, and 0 on failure, which
// miniz treats as an error
inline std::size_t
readFromDevice_(void* device, mz_uint64 offset, void* buffer, std::size_t size)
{
    auto file = static_cast<QIODevice*>(device);
    auto position = static_cast<qint64>(offset);

    if (file->pos() != position && !file->seek(position)) {
        return 0;
    }

    auto read = file->read(static_cast<char*>(buffer), qint64(size));
    return read < 0 ? 0 : std::size_t(read);
}

inline std::size_t writeToDevice_(
    void* device,
    mz_uint64 offset,
    const void* buffer,
    std::size_t size)
{
    auto file = static_cast<QIODevice*>(device);
    auto position = static_cast<qint64>(offset);

    if (file->pos() != position && !file->seek(position)) {
        return 0;
    }

    auto written = file->write(static_cast<const char*>(buffer), qint64(size));
    return written == qint64(size) ? size : 0;
}

[[nodiscard]] inline QString lastError_(mz_zip_archive& zip)
{
    return QString::fromUtf8(
        mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
}

// Relative, "/" between folders, and no empty, "." or ".." part, so no name
// can point outside the folder an archive is unpacked into
[[nodiscard]] inline bool isEntryName_(const QString& name)
{
    if (name.isEmpty() || name.contains(u'\\')) {
        return false;
    }

    for (const auto& part : name.split(u'/')) {
        if (part.isEmpty() || part == u"." || part == u"..") {
            return false;
        }
    }

    return true;
}

// An entry keeps its file's modification time, to the second
[[nodiscard]] inline bool addEntry_(mz_zip_archive& zip, const Entry& entry)
{
    if (!isEntryName_(entry.name)) {
        ASSERT(false, "Invalid zip entry name: {}!", entry.name);
        WARN("Invalid zip entry name: {}!", entry.name);
        return false;
    }

    auto data = Io::tryRead(entry.source);

    if (!data) {
        WARN("Failed to read {} for zipping!", entry.source);
        return false;
    }

    auto name = entry.name.toUtf8();
    auto modified =
        static_cast<MZ_TIME_T>(entry.source.lastModifiedEpochMs() / 1000);

    if (!mz_zip_writer_add_mem_ex_v2(
            &zip,
            name.constData(),
            data->constData(),
            std::size_t(data->size()),
            nullptr,
            0,
            MZ_DEFAULT_LEVEL,
            0,
            0,
            &modified,
            nullptr,
            0,
            nullptr,
            0)) {
        WARN(
            "Failed to add {} to a zip as {} (Error: {})!",
            entry.source,
            entry.name,
            lastError_(zip));
        return false;
    }

    return true;
}

[[nodiscard]] inline bool
addEntries_(mz_zip_archive& zip, const QList<Entry>& entries)
{
    for (const auto& entry : entries) {
        if (!addEntry_(zip, entry)) {
            return false;
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        WARN("Failed to finish a zip (Error: {})!", lastError_(zip));
        return false;
    }

    return true;
}

[[nodiscard]] inline std::optional<QByteArray>
extract_(mz_zip_archive& zip, const QString& name)
{
    auto utf8_name = name.toUtf8();
    auto index = mz_zip_reader_locate_file(
        &zip,
        utf8_name.constData(),
        nullptr,
        MZ_ZIP_FLAG_CASE_SENSITIVE);

    if (index < 0) {
        DEBUG("No entry named {} in the zip", name);
        return std::nullopt;
    }

    auto file_index = mz_uint(index);
    mz_zip_archive_file_stat stat{};

    if (!mz_zip_reader_file_stat(&zip, file_index, &stat)) {
        WARN("Failed to read zip entry {} (Error: {})!", name, lastError_(zip));
        return std::nullopt;
    }

    if (stat.m_is_directory) {
        DEBUG("Zip entry {} is a folder", name);
        return std::nullopt;
    }

    if (stat.m_uncomp_size > mz_uint64(MAX_ENTRY_SIZE_)) {
        WARN(
            "Zip entry {} declares {} bytes, over the {} allowed!",
            name,
            stat.m_uncomp_size,
            MAX_ENTRY_SIZE_);
        return std::nullopt;
    }

    if (stat.m_uncomp_size == 0) {
        return QByteArray{};
    }

    QByteArray data(qsizetype(stat.m_uncomp_size), Qt::Uninitialized);

    if (!mz_zip_reader_extract_to_mem(
            &zip,
            file_index,
            data.data(),
            std::size_t(data.size()),
            0)) {
        WARN(
            "Failed to extract zip entry {} (Error: {})!",
            name,
            lastError_(zip));
        return std::nullopt;
    }

    return data;
}

} // namespace Internal

// Writes the entries, in order, to a new archive at the path, replacing any
// archive there. All or nothing: if any entry fails, nothing is written and an
// existing archive is left as it was. The archive's folder must exist
inline bool write(const Coco::Path& archive, const QList<Entry>& entries)
{
    if (archive.isEmpty()) {
        DEBUG("Path empty!");
        return false;
    }

    // Uncommitted, a QSaveFile removes its temporary file when it's destroyed
    QSaveFile file(archive.toQString());

    if (!file.open(QIODevice::WriteOnly)) {
        WARN(
            "Failed to open {} for writing (Error: {})!",
            archive,
            file.errorString());
        return false;
    }

    mz_zip_archive zip{};
    zip.m_pWrite = Internal::writeToDevice_;
    zip.m_pIO_opaque = &file;

    if (!mz_zip_writer_init(&zip, 0)) {
        WARN(
            "Failed to start a zip at {} (Error: {})!",
            archive,
            Internal::lastError_(zip));
        return false;
    }

    auto added = Internal::addEntries_(zip, entries);
    mz_zip_writer_end(&zip);

    if (!added) {
        WARN("Failed to write the zip at {}!", archive);
        return false;
    }

    if (!file.commit()) {
        WARN(
            "Failed to commit the zip at {} (Error: {})!",
            archive,
            file.errorString());
        return false;
    }

    return true;
}

// The named entry's bytes, matched with letter case. nullopt when the archive
// can't be read, has no such entry, or declares the entry larger than
// Internal::MAX_ENTRY_SIZE_
inline std::optional<QByteArray>
readEntry(const Coco::Path& archive, const QString& name)
{
    if (archive.isEmpty()) {
        DEBUG("Path empty!");
        return std::nullopt;
    }

    QFile file(archive.toQString());

    if (!file.open(QIODevice::ReadOnly)) {
        WARN(
            "Failed to open {} for reading (Error: {})!",
            archive,
            file.errorString());
        return std::nullopt;
    }

    mz_zip_archive zip{};
    zip.m_pRead = Internal::readFromDevice_;
    zip.m_pIO_opaque = &file;

    if (!mz_zip_reader_init(&zip, mz_uint64(file.size()), 0)) {
        WARN(
            "Failed to read {} as a zip (Error: {})!",
            archive,
            Internal::lastError_(zip));
        return std::nullopt;
    }

    auto data = Internal::extract_(zip, name);
    mz_zip_reader_end(&zip);

    return data;
}

} // namespace Fernanda::Zip
