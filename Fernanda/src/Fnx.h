/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <string>

#include <QByteArray>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "Coco/Path.h"
#include "bit7z/bitarchivereader.hpp"
#include "bit7z/bitarchivewriter.hpp"

#include "AppDirs.h"
#include "FileTypes.h"
#include "Io.h"

// .fnx file format specification and utilities.
//
// Fnx::Io: Archive and working directory operations
// Fnx::Xml: DOM element factories and queries (stateless helpers)
//
// TODO: Are all these element.isNull checks necessary? Not sure...
// TODO: For mutators, probably pass QDomElement by value (otherwise by const
// ref). There are issues with passing QDomElement& for mutators
namespace Fernanda::Fnx {

namespace Internal {

    constexpr auto WORKING_DIR_MISSING_FMT_ =
        "Working directory ({}) doesn't exist!";

    // Io

    constexpr auto IO_MANIFEST_FILE_NAME_ = "Manifest.xml";
    constexpr auto IO_CONTENT_DIR_NAME_ = "content";

    inline const Coco::Path& dll_()
    {
        // 7za.dll is lighter than 7z.dll (a = "alone"); Unix uses full 7z.so
#if defined(Q_OS_WIN)
        auto file_name = "7za.dll";
        auto qrc_path = ":/7zip/7za.dll";
#else
        auto file_name = "7z.so";
        auto qrc_path = ":/7zip/7z.so";
#endif

        static auto file = AppDirs::userData() / file_name;
        if (!file.exists()) Coco::copy(qrc_path, file);
        return file;
    }

    // Xml

    constexpr auto XML_INDENT_ = 2;

    constexpr auto XML_FNX_VERSION_ATTR_ = "version";
    constexpr auto XML_FNX_VERSION_ = "1.0";

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

    constexpr auto DOCUMENT_ELEMENT_TAG = "fnx";
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
            Fernanda::Io::read(workingDir / Internal::IO_MANIFEST_FILE_NAME_);
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

        if (!Fernanda::Io::write(xml, path))
            CRITICAL("Failed to write manifest to {}!", path);
    }

    inline QDomElement
    addNewTextFile(const Coco::Path& workingDir, QDomDocument& dom)
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
        auto ext = ".txt";
        auto file_name = uuid + ext;
        auto path = workingDir / Internal::IO_CONTENT_DIR_NAME_ / file_name;

        if (!Fernanda::Io::write({}, path)) {
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

    // TODO: Section off some code from this and addNewTextFile
    inline QDomElement importTextFile(
        const Coco::Path& workingDir,
        QDomDocument& dom,
        const Coco::Path& fsPath)
    {
        if (!fsPath.exists() || fsPath.isDir()) return {};

        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return {};
        }

        if (dom.isNull()) {
            CRITICAL(Internal::XML_NULL_DOM_);
            return {};
        }

        auto uuid = Internal::makeUuid_();
        auto ext = ".txt";
        auto file_name = uuid + ext;
        auto path = workingDir / Internal::IO_CONTENT_DIR_NAME_ / file_name;

        if (!Coco::copy(fsPath, path)) {
            WARN("Failed to copy text file at {}", fsPath);
            return {};
        }

        auto element = dom.createElement(Internal::XML_FILE_TAG_);
        element.setAttribute(Internal::XML_NAME_ATTR_, fsPath.stemQString());
        element.setAttribute(Internal::XML_UUID_ATTR_, uuid);
        element.setAttribute(Internal::XML_FILE_EXT_ATTR_, ext);

        return element;
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

    constexpr auto EXT = ".fnx";

    inline bool isFnxFile(const Coco::Path& path)
    {
        return path.ext() == EXT && FileTypes::is(FileTypes::SevenZip, path);
    }

    inline void makeNewWorkingDir(const Coco::Path& workingDir)
    {
        // Create content directory
        if (!Coco::mkdir(workingDir / Internal::IO_CONTENT_DIR_NAME_)) return;

        // Create base Manifest.xml
        QByteArray xml_content{};
        QXmlStreamWriter xml(&xml_content);
        xml.setAutoFormatting(true);
        xml.setAutoFormattingIndent(Internal::XML_INDENT_);

        xml.writeStartDocument();
        xml.writeStartElement(Xml::DOCUMENT_ELEMENT_TAG);
        xml.writeAttribute(
            Internal::XML_FNX_VERSION_ATTR_,
            Internal::XML_FNX_VERSION_);

        xml.writeStartElement(Xml::NOTEBOOK_TAG);
        xml.writeEndElement();
        xml.writeStartElement(Xml::TRASH_TAG);
        xml.writeEndElement();

        xml.writeEndElement();
        xml.writeEndDocument();

        Fernanda::Io::write(
            xml_content,
            workingDir / Internal::IO_MANIFEST_FILE_NAME_);
    }

    inline void
    extract(const Coco::Path& archivePath, const Coco::Path& workingDir)
    {
        using namespace bit7z;

        INFO("Extracting archive at {} to {}", archivePath, workingDir);

        if (!archivePath.exists()) {
            CRITICAL("Archive file ({}) doesn't exist!", archivePath);
            return;
        }

        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return;
        }

        try {
            Bit7zLibrary lib{ Internal::dll_().toString() };
            BitArchiveReader archive{ lib,
                                      archivePath.toString(),
                                      BitFormat::SevenZip };
            archive.test();
            archive.extractTo(workingDir.toString());

        } catch (const BitException& ex) {
            CRITICAL("FNX archive extraction failed! Error: {}", ex.what());
        }
    }

    inline bool
    compress(const Coco::Path& archivePath, const Coco::Path& workingDir)
    {
        using namespace bit7z;

        INFO("Compressing archive at {} to {}", archivePath, workingDir);

        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return false;
        }

        try {
            Bit7zLibrary lib{ Internal::dll_().toString() };
            BitArchiveWriter archive{ lib, BitFormat::SevenZip };
            archive.setOverwriteMode(OverwriteMode::Overwrite);

            for (auto& entry_path : Coco::paths(workingDir)) {
                entry_path.isDir() ? archive.addDirectory(entry_path.toString())
                                   : archive.addFile(entry_path.toString());
            }

            // TODO: Move original to backup + clean backup if over n files
            archive.compressTo(archivePath.toString());
            return true;

        } catch (const BitException& ex) {
            CRITICAL("FNX archive compression failed! Error: {}", ex.what());
            return false;
        }
    }

    inline QString uuid(const Coco::Path& path) { return path.stemQString(); }

} // namespace Io

} // namespace Fernanda::Fnx
