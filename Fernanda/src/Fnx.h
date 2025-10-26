/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

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

namespace Fernanda::Fnx {

/// QDomDocument for living structure

namespace Internal {

    constexpr auto MODEL_FILE_NAME = "Model.xml";
    constexpr auto CONTENT_DIR_NAME = "content";

} // namespace Internal

inline void makeScaffold(const Coco::Path& root)
{
    // Create content directory
    if (!Coco::PathUtil::mkdir(root / Internal::CONTENT_DIR_NAME)) return;

    // Create empty Model.xml
    QString xml_content{};
    QXmlStreamWriter xml(&xml_content);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();
    xml.writeStartElement("notebook");
    xml.writeEndElement();
    xml.writeEndDocument();

    // Model.xml represents a virtual structuring of the contents of the
    // content folder

    TextIo::write(xml_content, root / Internal::MODEL_FILE_NAME);
}

// TODO: Replace QFile::copy with Coco version, maybe
inline const Coco::Path& dll()
{
    // Could be temp file?
    static auto file = AppDirs::userData() / "7z.dll";
    if (!file.exists()) QFile::copy(":/7zip/7z.dll", file.toQString());
    return file;
}

inline void extract(const Coco::Path& archivePath, const Coco::Path& root)
{
    using namespace bit7z;

    INFO("Extracting archive at {} to {}", archivePath, root);

    if (!archivePath.exists()) {
        CRITICAL("Archive file ({}) doesn't exist!", archivePath);
        return;
    }

    if (!root.exists()) {
        CRITICAL("Root/extraction directory ({}) doesn't exist!", root);
        return;
    }

    try {
        Bit7zLibrary lib{ dll().toString() };
        BitArchiveReader archive{ lib,
                                  archivePath.toString(),
                                  BitFormat::SevenZip };
        archive.test();
        archive.extractTo(root.toString());

    } catch (const BitException& ex) {
        CRITICAL("FNX archive extraction failed! Error: {}", ex.what());
    }
}

inline QDomDocument readModelXml(const Coco::Path& root)
{
    QDomDocument doc{};

    // auto content = TextIo::read(root / Internal::MODEL_FILE_NAME);
    /// Test:
    auto content =
        QString("<?xml version='1.0'?><notebook><folder name='Chapter 1'><file "
                "name='1' type='plaintext' uuid='xxx1'/></folder><file "
                "name='Notes' type='plaintext' uuid='xxx2'><file name='Other "
                "Notes' type='plaintext' uuid='xxx3'/></file></notebook>");
    auto result = doc.setContent(content);

    if (!result) {
        CRITICAL(
            "Failed to parse {}! Error: {} at line {}, column {}.",
            Internal::MODEL_FILE_NAME,
            result.errorMessage,
            result.errorLine,
            result.errorColumn);
        return {};
    }

    return doc;
}

} // namespace Fernanda::Fnx
