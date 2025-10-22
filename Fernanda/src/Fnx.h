#pragma once

#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "Coco/Path.h"

#include "TextIo.h"

namespace Fernanda::Fnx {

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
