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

inline bool makeScaffold(const Coco::Path& root)
{
    // Create content directory
    if (!Coco::PathUtil::mkdir(root / "content")) return false;

    // Create empty Model.xml
    QString xml_content{};
    QXmlStreamWriter xml(&xml_content);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();
    xml.writeStartElement("notebook");
    xml.writeEndElement();
    xml.writeEndDocument();

    // Model.xml represents a virtual structuring of the contents of the content
    // folder

    return TextIo::write(xml_content, root / "Model.xml");
}

// TODO: Replace QFile::copy with Coco version, maybe
inline const Coco::Path& dll()
{
    // Could be temp file?
    static auto file = AppDirs::userData() / "7z.dll";
    if (!file.exists()) QFile::copy(":/7zip/7z.dll", file.toQString());
    return file;
}

inline void
extract(const Coco::Path& archivePath, const Coco::Path& extractPath)
{
    using namespace bit7z;

    INFO("Extracting archive at {} to {}", archivePath, extractPath);

    if (!archivePath.exists()) {
        CRITICAL("Archive file ({}) doesn't exist!", archivePath);
        return;
    }

    if (!extractPath.exists()) {
        CRITICAL("Extraction directory ({}) doesn't exist!", archivePath);
        return;
    }

    try {
        Bit7zLibrary lib{ dll().toString() };
        BitArchiveReader archive{ lib,
                                  archivePath.toString(),
                                  BitFormat::SevenZip };
        archive.test();
        archive.extractTo(extractPath.toString());

    } catch (const BitException& ex) {
        CRITICAL("FNX archive extraction failed! Error: {}", ex.what());
    }
}

inline void tempTestWrite()
{
    QString xml_content{};
    QXmlStreamWriter xml(&xml_content);

    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    xml.writeStartDocument();

    // Root
    xml.writeStartElement("root");

    // Folder node at top level
    xml.writeStartElement("folder");
    xml.writeAttribute("name", "Chapter 1");

    // File parented by folder
    xml.writeStartElement("file");
    xml.writeAttribute("name", "1");
    xml.writeAttribute("type", "plaintext");
    xml.writeAttribute("uuid", "xxx1");
    xml.writeEndElement(); // file

    xml.writeEndElement(); // folder

    // File node at top level
    xml.writeStartElement("file");
    xml.writeAttribute("name", "Notes");
    xml.writeAttribute("type", "plaintext");
    xml.writeAttribute("uuid", "xxx2");

    // File parented by the top-level file
    xml.writeStartElement("file");
    xml.writeAttribute("name", "Other Notes");
    xml.writeAttribute("type", "plaintext");
    xml.writeAttribute("uuid", "xxx3");
    xml.writeEndElement(); // file (nested)

    xml.writeEndElement(); // file (top-level)

    xml.writeEndElement(); // root
    xml.writeEndDocument();

    auto path = Coco::Path::Desktop("sample_output.xml");
    bool success = TextIo::write(xml_content, path);

    if (success) {
        INFO("Sample XML saved to: {}", path);
    } else {
        WARN("Failed to save sample XML to: {}", path);
    }
}

} // namespace Fernanda::Fnx
