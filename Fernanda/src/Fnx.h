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
#include "TextIo.h"

// The .fnx file format specification and utilities. Defines everything about
// the .fnx format: archive I/O, Model.xml structure, DOM element creation, and
// format constants. Single source of truth for "what is a valid .fnx file."
//
// Notebook uses Fnx for all .fnx operations. FnxModel uses Fnx constants to
// interpret the DOM structure it presents.
namespace Fernanda::Fnx {

namespace Internal {

    constexpr auto MODEL_FILE_NAME_ = "Model.xml";
    constexpr auto XML_ROOT_TAG_ = "notebook";
    constexpr auto CONTENT_DIR_NAME = "content";
    constexpr auto XML_DIR_TAG = "folder";
    constexpr auto XML_FILE_TAG = "file";
    constexpr auto XML_NAME_ATTR = "name";
    constexpr auto XML_UUID_ATTR = "uuid";
    constexpr auto XML_EXT_ATTR = "extension";

    // TODO: Replace QFile::copy with Coco version, maybe
    inline const Coco::Path& dll_()
    {
        // Could be temp file?
        static auto file = AppDirs::userData() / "7z.dll";
        if (!file.exists()) QFile::copy(":/7zip/7z.dll", file.toQString());
        return file;
    }

    inline QString makeUuid_()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

} // namespace Internal

// TODO: Rename?
struct NewFileResult
{
    Coco::Path path{};
    QDomElement element{};
    bool isValid() const { return path.exists() && !element.isNull(); }
    // operator bool() const { return path.exists() && !element.isNull(); }
};

inline void addBlank(const Coco::Path& workingDir)
{
    // Create content directory
    if (!Coco::PathUtil::mkdir(workingDir / Internal::CONTENT_DIR_NAME)) return;

    // Create empty Model.xml
    QString xml_content{};
    QXmlStreamWriter xml(&xml_content);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();
    xml.writeStartElement(Internal::XML_ROOT_TAG_);
    xml.writeEndElement();
    xml.writeEndDocument();

    // Model.xml represents a virtual structuring of the contents of the
    // content folder

    TextIo::write(xml_content, workingDir / Internal::MODEL_FILE_NAME_);
}

inline void extract(const Coco::Path& archivePath, const Coco::Path& workingDir)
{
    using namespace bit7z;

    INFO("Extracting archive at {} to {}", archivePath, workingDir);

    if (!archivePath.exists()) {
        CRITICAL("Archive file ({}) doesn't exist!", archivePath);
        return;
    }

    if (!workingDir.exists()) {
        CRITICAL("Working directory ({}) doesn't exist!", workingDir);
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

// TODO: Compress method

inline QDomDocument makeDomDocument(const Coco::Path& workingDir)
{
    QDomDocument doc{};

    auto content = TextIo::read(workingDir / Internal::MODEL_FILE_NAME_);
    auto result = doc.setContent(content);

    if (!result) {
        CRITICAL(
            "Failed to parse {}! Error: {} at line {}, column {}.",
            Internal::MODEL_FILE_NAME_,
            result.errorMessage,
            result.errorLine,
            result.errorColumn);
        return {};
    }

    return doc;
}

inline NewFileResult
addTextFile(const Coco::Path& workingDir, QDomDocument& dom)
{
    auto uuid = Internal::makeUuid_();
    auto ext = ".txt";
    auto file_name = uuid + ext;
    auto path = workingDir / Internal::CONTENT_DIR_NAME / file_name;

    if (!TextIo::write({}, path)) {
        WARN("Failed to create text file at {}", path);
        return {};
    }

    auto element = dom.createElement(Internal::XML_FILE_TAG);
    element.setAttribute(Internal::XML_NAME_ATTR, "Untitled");
    element.setAttribute(Internal::XML_UUID_ATTR, uuid);
    element.setAttribute(Internal::XML_EXT_ATTR, ext);

    return { path, element };
}

// TODO: Section off some code from this and addTextFile
inline NewFileResult importTextFile(
    const Coco::Path& fsPath,
    const Coco::Path& workingDir,
    QDomDocument& dom)
{
    auto uuid = Internal::makeUuid_();
    auto ext = ".txt";
    auto file_name = uuid + ext;
    auto path = workingDir / Internal::CONTENT_DIR_NAME / file_name;

    if (!QFile::copy(fsPath.toQString(), path.toQString())) {
        WARN("Failed to copy text file at {}", fsPath);
        return {};
    }

    auto element = dom.createElement(Internal::XML_FILE_TAG);
    element.setAttribute(Internal::XML_NAME_ATTR, fsPath.stemQString());
    element.setAttribute(Internal::XML_UUID_ATTR, uuid);
    element.setAttribute(Internal::XML_EXT_ATTR, ext);

    return { path, element };
}

inline bool isDir(const QDomElement& element)
{
    if (element.isNull()) return false;
    return element.tagName() == Internal::XML_DIR_TAG;
}

inline bool isFile(const QDomElement& element)
{
    if (element.isNull()) return false;
    return element.tagName() == Internal::XML_FILE_TAG;
}

inline QString name(const QDomElement& element)
{
    return element.attribute(Internal::XML_NAME_ATTR, "<unnamed>");
}

inline QString uuid(const QDomElement& element)
{
    return element.attribute(Internal::XML_UUID_ATTR);
}

inline QString ext(const QDomElement& element)
{
    return element.attribute(Internal::XML_EXT_ATTR);
}

inline Coco::Path relativePath(const QDomElement& element)
{
    if (!isFile(element)) return {};
    auto file_name = uuid(element) + ext(element);
    return Coco::Path(Internal::CONTENT_DIR_NAME) / file_name;
}

} // namespace Fernanda::Fnx
