/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <string>

#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QString>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"
#include "bit7z/bitarchivereader.hpp"
#include "bit7z/bitarchivewriter.hpp"

#include "AppDirs.h"
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

    constexpr auto IO_MODEL_FILE_NAME_ = "Model.xml";
    constexpr auto IO_CONTENT_DIR_NAME_ = "content";

    // TODO: Replace QFile::copy with Coco version, maybe
    inline const Coco::Path& dll_()
    {
        // Could be temp file?
        static auto file = AppDirs::userData() / "7z.dll";
        if (!file.exists()) QFile::copy(":/7zip/7z.dll", file.toQString());
        return file;
    }

    // Xml

    constexpr auto XML_INDENT_ = 2;
    constexpr auto XML_ROOT_TAG_ = "notebook";
    constexpr auto XML_VFOLDER_TAG_ = "vfolder";
    constexpr auto XML_FILE_TAG_ = "file";
    constexpr auto XML_NAME_ATTR_ = "name";
    constexpr auto XML_NAME_ATTR_FILE_DEF_ = "Untitled";
    constexpr auto XML_NAME_ATTR_DIR_DEF_ = "New folder";
    constexpr auto XML_FILE_UUID_ATTR_ = "uuid";
    constexpr auto XML_FILE_EXT_ATTR_ = "extension";
    constexpr auto XML_FILE_EDITED_ATTR_ = "edited";
    constexpr auto XML_NULL_DOM_ = "DOM is null!";

    inline QString makeUuid_()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

} // namespace Internal

// Used by Notebook
namespace Io {

    inline void makeNewWorkingDir(const Coco::Path& workingDir)
    {
        // Create content directory
        if (!Coco::PathUtil::mkdir(workingDir / Internal::IO_CONTENT_DIR_NAME_))
            return;

        // Create base Model.xml
        QByteArray xml_content{};
        QXmlStreamWriter xml(&xml_content);
        xml.setAutoFormatting(true);
        xml.setAutoFormattingIndent(Internal::XML_INDENT_);
        xml.writeStartDocument();
        xml.writeStartElement(Internal::XML_ROOT_TAG_);
        xml.writeEndElement();
        xml.writeEndDocument();

        // Model.xml represents a virtual structuring of the contents of the
        // content folder
        Fernanda::Io::write(
            xml_content,
            workingDir / Internal::IO_MODEL_FILE_NAME_);
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
            archive.addDirectory(workingDir.toString());

            // Overwrites if exists
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

// Used by FnxModel
namespace Xml {

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
        return element.attribute(Internal::XML_FILE_UUID_ATTR_);
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

    inline void clearEditedRecursive(QDomElement parent)
    {
        auto child = parent.firstChildElement();
        while (!child.isNull()) {
            if (isFile(child)) setEdited(child, false);
            clearEditedRecursive(child);
            child = child.nextSiblingElement();
        }
    }

    inline void clearEditedRecursive(QDomDocument& dom)
    {
        clearEditedRecursive(dom.documentElement());
    }

    inline QDomDocument makeDom(const Coco::Path& workingDir)
    {
        if (!workingDir.exists()) {
            CRITICAL(Internal::WORKING_DIR_MISSING_FMT_, workingDir);
            return {};
        }

        QDomDocument doc{};
        auto content =
            Fernanda::Io::read(workingDir / Internal::IO_MODEL_FILE_NAME_);
        auto result = doc.setContent(content);

        if (!result) {
            CRITICAL(
                "Failed to parse {}! Error: {} at line {}, column {}.",
                Internal::IO_MODEL_FILE_NAME_,
                result.errorMessage,
                result.errorLine,
                result.errorColumn);
            return {};
        }

        return doc;
    }

    // TODO: Return bool?
    inline void
    writeModelFile(const Coco::Path& workingDir, const QDomDocument& dom)
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
        auto model_path = workingDir / Internal::IO_MODEL_FILE_NAME_;

        if (!Fernanda::Io::write(xml, model_path))
            CRITICAL("Failed to write model to {}!", model_path);
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
        element.setAttribute(Internal::XML_FILE_UUID_ATTR_, uuid);
        element.setAttribute(Internal::XML_FILE_EXT_ATTR_, ext);

        return element;
    }

    // TODO: Section off some code from this and addNewTextFile
    inline QDomElement importTextFile(
        const Coco::Path& workingDir,
        QDomDocument& dom,
        const Coco::Path& fsPath)
    {
        if (!fsPath.exists() || fsPath.isFolder()) return {};

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

        if (!QFile::copy(fsPath.toQString(), path.toQString())) {
            WARN("Failed to copy text file at {}", fsPath);
            return {};
        }

        auto element = dom.createElement(Internal::XML_FILE_TAG_);
        element.setAttribute(Internal::XML_NAME_ATTR_, fsPath.stemQString());
        element.setAttribute(Internal::XML_FILE_UUID_ATTR_, uuid);
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
        element.setAttribute(
            Internal::XML_FILE_UUID_ATTR_,
            Internal::makeUuid_());

        return element;
    }

} // namespace Xml

} // namespace Fernanda::Fnx
