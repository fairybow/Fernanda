/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>
#include <string>

#include <QByteArray>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <miniz.h>

#include <Coco/Path.h>

#include "core/Files.h"
#include "core/Io.h"

// .fnx file format specification and utilities
// - Nbx::Io: Archive and working directory operations
// - Nbx::Xml: DOM element factories and queries (stateless helpers)
//
// TODO: Are all these element.isNull checks necessary? Not sure...
// TODO: For mutators, probably pass QDomElement by value (otherwise by const
// ref). There are issues with passing QDomElement& for mutators
namespace Hearth::Nbx {

namespace Internal {

    constexpr auto WORKING_DIR_MISSING_FMT_ =
        "Working directory ({}) doesn't exist!";

    // Io

    constexpr auto IO_MANIFEST_FILE_NAME_ = "Manifest.xml";
    constexpr auto IO_CONTENT_DIR_NAME_ = "content";

    // Xml

    constexpr auto XML_INDENT_ = 2;

    constexpr auto XML_NBX_VERSION_ATTR_ = "version";
    constexpr auto XML_NBX_VERSION_ = "1.0";

    constexpr auto XML_VFOLDER_TAG_ = "vfolder";
    constexpr auto XML_FILE_TAG_ = "file";
    constexpr auto XML_NAME_ATTR_ = "name";
    constexpr auto XML_NAME_ATTR_FILE_DEF_ = "Untitled";
    constexpr auto XML_NAME_ATTR_DIR_DEF_ = "New folder";
    constexpr auto XML_UUID_ATTR_ = "uuid";
    constexpr auto XML_TRASH_RESTORE_PARENT_UUID_ATTR_ =
        "parent_on_restore_uuid";
    constexpr auto XML_FILE_EXT_ATTR_ = "extension";
    constexpr auto XML_FILE_EDITED_ATTR_ = "edited";
    constexpr auto XML_NULL_DOM_ = "DOM is null!";

    inline QString makeUuid_()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

} // namespace Internal

// Used by FnxModel
namespace Xml {

    constexpr auto DOCUMENT_ELEMENT_TAG = "nbx";
    constexpr auto NOTEBOOK_TAG = "notebook";
    constexpr auto TRASH_TAG = "trash";

    inline bool isVirtualFolder(const QDomElement& element)
    {
        if (element.isNull()) return false;
        return element.tagName() == Internal::XML_VFOLDER_TAG_;
    }

    inline bool isFile(const QDomElement& element)
    {
        if (element.isNull()) return false;
        return element.tagName() == Internal::XML_FILE_TAG_;
    }

    inline bool isEdited(const QDomElement& element)
    {
        return element.hasAttribute(Internal::XML_FILE_EDITED_ATTR_);
    }

    inline bool hasEditedDescendant(const QDomElement& element)
    {
        auto child = element.firstChildElement();

        while (!child.isNull()) {
            if (isFile(child) && isEdited(child)) return true;
            if (hasEditedDescendant(child)) return true;
            child = child.nextSiblingElement();
        }

        return false;
    }

    inline QString name(const QDomElement& element)
    {
        return element.attribute(Internal::XML_NAME_ATTR_);
    }

    inline QString uuid(const QDomElement& element)
    {
        return element.attribute(Internal::XML_UUID_ATTR_);
    }

    inline QString ext(const QDomElement& element)
    {
        return element.attribute(Internal::XML_FILE_EXT_ATTR_);
    }

    inline Coco::Path relPath(const QDomElement& element)
    {
        if (!isFile(element)) return {};
        auto file_name = uuid(element) + ext(element);
        return Coco::Path(Internal::IO_CONTENT_DIR_NAME_) / file_name;
    }

    inline void rename(QDomElement element, const QString& name)
    {
        if (element.isNull() || !element.hasAttribute(Internal::XML_NAME_ATTR_)
            || name.isEmpty())
            return;
        element.setAttribute(Internal::XML_NAME_ATTR_, name);
    }

    // Setting false removes the attribute (the attribute is itself the boolean)
    inline void setEdited(QDomElement element, bool edited)
    {
        if (element.isNull() || !isFile(element)) return;
        edited ? element.setAttribute(Internal::XML_FILE_EDITED_ATTR_, "")
               : element.removeAttribute(Internal::XML_FILE_EDITED_ATTR_);
    }

    inline QString restoreParentUuid(const QDomElement& element)
    {
        return element.attribute(Internal::XML_TRASH_RESTORE_PARENT_UUID_ATTR_);
    }

    inline void setRestoreParentUuid(QDomElement& element, const QString& uuid)
    {
        uuid.isEmpty() ? element.removeAttribute(
                             Internal::XML_TRASH_RESTORE_PARENT_UUID_ATTR_)
                       : element.setAttribute(
                             Internal::XML_TRASH_RESTORE_PARENT_UUID_ATTR_,
                             uuid);
    }

    inline void clearRestoreParentUuid(QDomElement& element)
    {
        setRestoreParentUuid(element, {});
    }

    inline QDomElement notebookElement(const QDomDocument& dom)
    {
        return dom.documentElement().firstChildElement(Xml::NOTEBOOK_TAG);
    }

    inline QDomElement trashElement(const QDomDocument& dom)
    {
        return dom.documentElement().firstChildElement(Xml::TRASH_TAG);
    }

    inline bool isInTrash(const QDomDocument& dom, const QDomElement& element)
    {
        auto trash = trashElement(dom);
        auto current = element.parentNode().toElement();
        while (!current.isNull()) {
            if (current == trash) return true;
            current = current.parentNode().toElement();
        }

        return false;
    }

    inline QDomDocument makeDom(const Coco::Path& workingDir)
    {
        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return {};
        }

        QDomDocument doc{};
        auto content =
            Hearth::Io::read(workingDir / Internal::IO_MANIFEST_FILE_NAME_);
        auto result = doc.setContent(content);

        if (!result) {
            CRITICAL(
                "Failed to parse {}! Error: {} at line {}, column {}.",
                Internal::IO_MANIFEST_FILE_NAME_,
                result.errorMessage,
                result.errorLine,
                result.errorColumn);
            return {};
        }

        return doc;
    }

    // TODO: Return bool?
    inline void
    writeManifest(const Coco::Path& workingDir, const QDomDocument& dom)
    {
        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return;
        }

        if (dom.isNull()) {
            CRITICAL(Internal::XML_NULL_DOM_);
            return;
        }

        auto xml = dom.toByteArray(Internal::XML_INDENT_);
        auto path = workingDir / Internal::IO_MANIFEST_FILE_NAME_;

        if (!Hearth::Io::write(xml, path))
            CRITICAL("Failed to write manifest to {}!", path);
    }

    inline QDomElement addNewFile(
        Files::Type fileType,
        const QString& extension,
        const Coco::Path& workingDir,
        QDomDocument& dom)
    {
        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return {};
        }

        if (dom.isNull()) {
            CRITICAL(Internal::XML_NULL_DOM_);
            return {};
        }

        auto uuid = Internal::makeUuid_();
        auto ext =
            extension.isEmpty() ? Files::canonicalExt(fileType) : extension;
        auto file_name = uuid + ext;
        auto path = workingDir / Internal::IO_CONTENT_DIR_NAME_ / file_name;

        if (!Hearth::Io::write({}, path)) {
            WARN("Failed to create text file at {}", path);
            return {};
        }

        auto element = dom.createElement(Internal::XML_FILE_TAG_);
        element.setAttribute(
            Internal::XML_NAME_ATTR_,
            Internal::XML_NAME_ATTR_FILE_DEF_);
        element.setAttribute(Internal::XML_UUID_ATTR_, uuid);
        element.setAttribute(Internal::XML_FILE_EXT_ATTR_, ext);

        return element;
    }

    inline QDomElement addNewFile(
        Files::Type fileType,
        const Coco::Path& workingDir,
        QDomDocument& dom)
    {
        return addNewFile(fileType, {}, workingDir, dom);
    }

    inline QDomElement addVirtualFolder(QDomDocument& dom)
    {
        if (dom.isNull()) {
            CRITICAL(Internal::XML_NULL_DOM_);
            return {};
        }

        auto element = dom.createElement(Internal::XML_VFOLDER_TAG_);

        element.setAttribute(
            Internal::XML_NAME_ATTR_,
            Internal::XML_NAME_ATTR_DIR_DEF_);
        element.setAttribute(Internal::XML_UUID_ATTR_, Internal::makeUuid_());

        return element;
    }

} // namespace Xml

// Used by Notebook
namespace Io {

    /// TODO BA
    using BeforeOverwriteHook =
        std::function<void(const Coco::Path& originalFnx)>;

    inline void makeNewWorkingDir(const Coco::Path& workingDir)
    {
        // Create content directory
        if (!Coco::mkpath(workingDir / Internal::IO_CONTENT_DIR_NAME_)) return;

        // Create base Manifest.xml
        QByteArray xml_content{};
        QXmlStreamWriter xml(&xml_content);
        xml.setAutoFormatting(true);
        xml.setAutoFormattingIndent(Internal::XML_INDENT_);

        xml.writeStartDocument();
        xml.writeStartElement(Xml::DOCUMENT_ELEMENT_TAG);
        xml.writeAttribute(
            Internal::XML_NBX_VERSION_ATTR_,
            Internal::XML_NBX_VERSION_);

        xml.writeStartElement(Xml::NOTEBOOK_TAG);
        xml.writeEndElement();
        xml.writeStartElement(Xml::TRASH_TAG);
        xml.writeEndElement();

        xml.writeEndElement();
        xml.writeEndDocument();

        Hearth::Io::write(
            xml_content,
            workingDir / Internal::IO_MANIFEST_FILE_NAME_);
    }

    // TODO: Return bool? Fully fail if any file fails?
    inline void
    extract(const Coco::Path& archivePath, const Coco::Path& workingDir)
    {
        INFO("Extracting archive at {} to {}", archivePath, workingDir);

        if (!archivePath.exists()) {
            CRITICAL("Archive file ({}) doesn't exist!", archivePath);
            return;
        }

        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return;
        }

        mz_zip_archive zip{};

        // Read it
        if (!mz_zip_reader_init_file(&zip, archivePath.toString().c_str(), 0)) {
            CRITICAL(
                "NBX archive read failed! Error: {}",
                mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
            return;
        }

        auto cleanup = qScopeGuard([&] { mz_zip_reader_end(&zip); });

        auto file_count = mz_zip_reader_get_num_files(&zip);

        for (mz_uint i = 0; i < file_count; ++i) {

            // Get metadata
            mz_zip_archive_file_stat stat{};

            if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
                WARN("Failed to stat archive entry {}", i);
                continue;
            }

            // Write
            auto out_path = workingDir / stat.m_filename;

            if (mz_zip_reader_is_file_a_directory(&zip, i)) {
                Coco::mkpath(out_path);
                continue;
            }

            Coco::mkpath(out_path.parent());

            if (!mz_zip_reader_extract_to_file(
                    &zip,
                    i,
                    out_path.toString().c_str(),
                    0)) {
                WARN(
                    "Failed to extract {}: {}",
                    stat.m_filename,
                    mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
            }
        }
    }

    inline bool compress(
        const Coco::Path& archivePath,
        const Coco::Path& workingDir,
        const BeforeOverwriteHook& beforeOverwriteHook = {})
    {
        INFO("Compressing archive at {} to {}", workingDir, archivePath);

        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return false;
        }

        // Create temp archive beside original
        auto temp_path = archivePath.toString() + ".tmp";
        mz_zip_archive zip{};

        if (!mz_zip_writer_init_file(&zip, temp_path.c_str(), 0)) {
            CRITICAL(
                "NBX temp archive creation failed! Error: {}",
                mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
            return false;
        }

        auto cleanup = qScopeGuard([&] { mz_zip_writer_end(&zip); });

        auto ok = true; // Get warnings for all fails
        auto entries = Coco::allFilePaths(workingDir);

        for (const auto& entry_path : entries) {
            // Make relative path (zip expects generic path)
            auto rel = entry_path.lexicallyRelative(workingDir).genericString();

            if (!mz_zip_writer_add_file(
                    &zip,
                    rel.c_str(),
                    entry_path.toString().c_str(),
                    nullptr,
                    0,
                    MZ_DEFAULT_COMPRESSION)) {
                WARN(
                    "Failed to add {}: {}",
                    rel,
                    mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
                ok = false;
            }
        }

        if (ok) ok = mz_zip_writer_finalize_archive(&zip);

        if (!ok) {
            CRITICAL("NBX archive compression failed!");
            Coco::remove(temp_path);
            return false;
        }

        // Close the writer before file operations
        cleanup.dismiss();
        mz_zip_writer_end(&zip);

        /// TODO BA
        if (beforeOverwriteHook) beforeOverwriteHook(archivePath);
        Coco::remove(archivePath);

        return Coco::rename(temp_path, archivePath);
    }

    inline QString uuid(const Coco::Path& path) { return path.stemQString(); }

} // namespace Io

} // namespace Hearth::Nbx
