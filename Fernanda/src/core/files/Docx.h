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

#include <optional>

#include <QByteArray>
#include <QChar>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QUrl>
#include <QXmlStreamReader>
#include <QtTypes>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/files/Zip.h"

// Word documents (.docx) read as plain text: one line per paragraph, in
// reading order, with formatting dropped. Only the text a reader of the
// document sees is kept: deleted tracked changes, field codes, hidden text, and
// Word's fallback copies of content are skipped. A text box's paragraphs follow
// the paragraph it's anchored to, and a table's paragraphs come row by row.
// Headers, footers, and comments are left out.
//
// Elements are matched by namespace as well as name, so documents in Word's
// Strict Open XML format read the same as ordinary ones
namespace Fernanda::Docx {

using namespace Qt::StringLiterals;

enum class Failure
{
    None,
    NotAWordDocument, // Not a zip, no main document, or a part too large
    Damaged // The main document's XML is malformed or isn't a document
};

struct Conversion
{
    QString text{};
    Failure failure = Failure::None;
};

namespace Internal {

// The largest part read from a Word file. A long novel's document part is
// tens of megabytes
constexpr qint64 MAX_PART_SIZE_ = 256LL * 1024 * 1024;

constexpr QStringView W_ =
    u"http://schemas.openxmlformats.org/wordprocessingml/2006/main";
constexpr QStringView W_STRICT_ =
    u"http://purl.oclc.org/ooxml/wordprocessingml/main";
constexpr QStringView MC_ =
    u"http://schemas.openxmlformats.org/markup-compatibility/2006";
constexpr QStringView RELATIONSHIPS_ =
    u"http://schemas.openxmlformats.org/package/2006/relationships";

[[nodiscard]] inline bool isW_(const QXmlStreamReader& reader, QStringView name)
{
    auto uri = reader.namespaceUri();
    return (uri == W_ || uri == W_STRICT_) && reader.name() == name;
}

[[nodiscard]] inline bool
isMc_(const QXmlStreamReader& reader, QStringView name)
{
    return reader.namespaceUri() == MC_ && reader.name() == name;
}

// Elements whose content is never part of the text: deleted and moved-away
// tracked changes, field instructions, paragraph properties (which hold the
// paragraph mark's formatting, not text), and the section's page setup
[[nodiscard]] inline bool isSkipped_(const QXmlStreamReader& reader)
{
    return isW_(reader, u"del") || isW_(reader, u"moveFrom") ||
           isW_(reader, u"delText") || isW_(reader, u"instrText") ||
           isW_(reader, u"delInstrText") || isW_(reader, u"pPr") ||
           isW_(reader, u"sectPr");
}

// An on/off property (<w:vanish/>, <w:vanish w:val="0"/>) is on unless its
// val says otherwise
[[nodiscard]] inline bool isOn_(const QXmlStreamReader& reader)
{
    auto value = reader.attributes().value(reader.namespaceUri(), u"val");
    return value.isEmpty() ||
           (value != u"0" && value != u"false" && value != u"off");
}

// Reads a run's properties to their end and says whether they hide the run
[[nodiscard]] inline bool readRunIsHidden_(QXmlStreamReader& reader)
{
    auto hidden = false;

    while (reader.readNextStartElement()) {
        if (isW_(reader, u"vanish")) {
            hidden = isOn_(reader);
        }

        reader.skipCurrentElement();
    }

    return hidden;
}

inline void readParagraph_(QXmlStreamReader& reader, QStringList& lines);

// The content of whatever element the reader is in, up to its end, added to
// the paragraph's line. A paragraph met inside it (a text box's) goes to
// after, to follow the paragraph being read
inline void
readInline_(QXmlStreamReader& reader, QString& line, QStringList& after)
{
    while (reader.readNextStartElement()) {
        if (isSkipped_(reader)) {
            reader.skipCurrentElement();

        } else if (isW_(reader, u"t")) {
            line += reader.readElementText();

        } else if (isW_(reader, u"rPr")) {
            // A run's properties come first in the run, so a hidden run is
            // skipped from here to the run's end
            if (readRunIsHidden_(reader)) {
                reader.skipCurrentElement();
                return;
            }

        } else if (isW_(reader, u"tab") || isW_(reader, u"ptab")) {
            line += u'\t';
            reader.skipCurrentElement();

        } else if (isW_(reader, u"br") || isW_(reader, u"cr")) {
            line += u'\n';
            reader.skipCurrentElement();

        } else if (isW_(reader, u"noBreakHyphen")) {
            line += QChar(0x2011);
            reader.skipCurrentElement();

        } else if (isW_(reader, u"p")) {
            readParagraph_(reader, after);

        } else if (isMc_(reader, u"AlternateContent")) {
            // The first choice is read and the rest, the fallback included,
            // skipped: each holds the same content in another form
            auto read_one = false;

            while (reader.readNextStartElement()) {
                if (!read_one && isMc_(reader, u"Choice")) {
                    readInline_(reader, line, after);
                    read_one = true;
                } else {
                    reader.skipCurrentElement();
                }
            }

        } else {
            // Runs, hyperlinks, inserted and moved-to changes, fields' results,
            // content controls, drawings on the way to a text box: what's
            // inside them is read as if they weren't there. Anything else
            // with no text (an optional hyphen, a symbol-font character)
            // contributes nothing
            readInline_(reader, line, after);
        }
    }
}

inline void readParagraph_(QXmlStreamReader& reader, QStringList& lines)
{
    QString line{};
    QStringList after{};

    readInline_(reader, line, after);

    lines << line;
    lines << after;
}

// Everything between paragraphs (the body, tables, rows, cells, content
// controls) is read through to the paragraphs inside it
inline void readBlocks_(QXmlStreamReader& reader, QStringList& lines)
{
    while (reader.readNextStartElement()) {
        if (isSkipped_(reader)) {
            reader.skipCurrentElement();

        } else if (isW_(reader, u"p")) {
            readParagraph_(reader, lines);

        } else if (isMc_(reader, u"AlternateContent")) {
            auto read_one = false;

            while (reader.readNextStartElement()) {
                if (!read_one && isMc_(reader, u"Choice")) {
                    readBlocks_(reader, lines);
                    read_one = true;
                } else {
                    reader.skipCurrentElement();
                }
            }

        } else {
            readBlocks_(reader, lines);
        }
    }
}

// A relationship's target, as a part's name inside the zip. Targets are
// relative to the folder of the part that names them, or start with "/" from
// the package's root, and are percent-encoded. nullopt for one that climbs
// out of the package
[[nodiscard]] inline std::optional<QString>
resolveTarget_(const QString& baseFolder, const QString& target)
{
    auto decoded = QUrl::fromPercentEncoding(target.toUtf8());
    auto full =
        decoded.startsWith(u'/') ? decoded.mid(1) : baseFolder + u'/' + decoded;

    QStringList parts{};

    for (const auto& part : full.split(u'/', Qt::SkipEmptyParts)) {
        if (part == u".") {
            continue;
        }

        if (part == u"..") {
            if (parts.isEmpty()) {
                return std::nullopt;
            }

            parts.removeLast();
            continue;
        }

        parts << part;
    }

    if (parts.isEmpty()) {
        return std::nullopt;
    }

    return parts.join(u'/');
}

// The main document's part name, from the package's relationships. The
// relationship's type ends in "/officeDocument" in both the ordinary and the
// Strict format
[[nodiscard]] inline std::optional<QString>
mainDocumentPart_(const QByteArray& packageRelationships)
{
    QXmlStreamReader reader(packageRelationships);

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement() ||
            reader.namespaceUri() != RELATIONSHIPS_ ||
            reader.name() != u"Relationship") {
            continue;
        }

        auto attributes = reader.attributes();

        if (!attributes.value(u"Type").endsWith(u"/officeDocument") ||
            attributes.value(u"TargetMode") == u"External") {
            continue;
        }

        return resolveTarget_(
            QString{},
            attributes.value(u"Target").toString());
    }

    return std::nullopt;
}

} // namespace Internal

// The text of a main document part (document.xml). nullopt when the XML is
// malformed or its root isn't a document
[[nodiscard]] inline std::optional<QString>
textOf(const QByteArray& documentXml)
{
    QXmlStreamReader reader(documentXml);

    if (!reader.readNextStartElement() ||
        !Internal::isW_(reader, u"document")) {
        return std::nullopt;
    }

    QStringList lines{};
    Internal::readBlocks_(reader, lines);

    if (reader.hasError()) {
        WARN(
            "Malformed Word document XML at line {} (Error: {})!",
            reader.lineNumber(),
            reader.errorString());
        return std::nullopt;
    }

    return lines.join(u'\n');
}

// The Word file's text, or why it couldn't be read. A file that fails gives no
// text at all, never part of it
[[nodiscard]] inline Conversion toPlainText(const Coco::Path& path)
{
    auto relationships =
        Zip::readEntry(path, u"_rels/.rels"_s, Internal::MAX_PART_SIZE_);

    if (!relationships) {
        WARN("{} has no package relationships to read!", path);
        return { QString{}, Failure::NotAWordDocument };
    }

    auto part = Internal::mainDocumentPart_(*relationships);

    if (!part) {
        WARN("{} names no main document!", path);
        return { QString{}, Failure::NotAWordDocument };
    }

    auto xml = Zip::readEntry(path, *part, Internal::MAX_PART_SIZE_);

    if (!xml) {
        WARN("Failed to read {} from {}!", *part, path);
        return { QString{}, Failure::NotAWordDocument };
    }

    auto text = textOf(*xml);

    if (!text) {
        return { QString{}, Failure::Damaged };
    }

    return { *text, Failure::None };
}

} // namespace Fernanda::Docx
