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
#include <QFile>
#include <QString>
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

    // TODO: Replace QFile::copy with Coco version, maybe
    inline const Coco::Path& dll_()
    {
        // Could be temp file?
        static auto file = AppDirs::userData() / "7z.dll";
        if (!file.exists()) QFile::copy(":/7zip/7z.dll", file.toQString());
        return file;
    }

} // namespace Internal

// TODO: Should we have isDir type checks in Fnx for use in Notebook and
// FnxModel? No constants there but pass DOM elements here for checks against
// the spec...
constexpr auto CONTENT_DIR_NAME = "content";
constexpr auto XML_DIR_TAG = "folder";
constexpr auto XML_FILE_TAG = "file";
constexpr auto XML_UUID_ATTR = "uuid";
constexpr auto XML_EXT_ATTR = "extension";

inline void addBlank(const Coco::Path& dir)
{
    // Create content directory
    if (!Coco::PathUtil::mkdir(dir / CONTENT_DIR_NAME)) return;

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

    TextIo::write(xml_content, dir / Internal::MODEL_FILE_NAME_);
}

inline void extract(const Coco::Path& archivePath, const Coco::Path& dir)
{
    using namespace bit7z;

    INFO("Extracting archive at {} to {}", archivePath, dir);

    if (!archivePath.exists()) {
        CRITICAL("Archive file ({}) doesn't exist!", archivePath);
        return;
    }

    if (!dir.exists()) {
        CRITICAL("Extraction directory ({}) doesn't exist!", dir);
        return;
    }

    try {
        Bit7zLibrary lib{ Internal::dll_().toString() };
        BitArchiveReader archive{ lib,
                                  archivePath.toString(),
                                  BitFormat::SevenZip };
        archive.test();
        archive.extractTo(dir.toString());

    } catch (const BitException& ex) {
        CRITICAL("FNX archive extraction failed! Error: {}", ex.what());
    }
}

inline QDomDocument makeDomDocument(const Coco::Path& extractDir)
{
    QDomDocument doc{};

    auto content = TextIo::read(extractDir / Internal::MODEL_FILE_NAME_);
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

} // namespace Fernanda::Fnx
