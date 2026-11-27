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
#include <QHash>
#include <QList>
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
// Footnotes and endnotes are marked [1], [2], and so on, numbered together in
// reading order, and listed after the text, each on one line. Headers, footers,
// and comments are left out.
//
// Elements are matched by namespace as well as name, so documents in Word's
// Strict Open XML format read the same as ordinary ones
namespace Fernanda::Docx {

using namespace Qt::StringLiterals;

enum class Failure
{
    None,
    NotAWordDocument, // Not a zip, no main document, or a part unreadable
    Damaged           // A part's XML is malformed or isn't what it should be
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

// A footnote or endnote reference, in the order the text makes them
struct NoteReference_
{
    bool isEndnote = false;
    QString id{};
};

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

inline void readParagraph_(
    QXmlStreamReader& reader,
    QStringList& lines,
    QList<NoteReference_>& references);

// The content of whatever element the reader is in, up to its end, added to
// the paragraph's line. A paragraph met inside it (a text box's) goes to
// after, to follow the paragraph being read. A note reference is added to
// references, and its marker to the line
inline void readInline_(
    QXmlStreamReader& reader,
    QString& line,
    QStringList& after,
    QList<NoteReference_>& references)
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

        } else if (
            isW_(reader, u"footnoteReference") ||
            isW_(reader, u"endnoteReference")) {
            references << NoteReference_{
                reader.name() == u"endnoteReference",
                reader.attributes()
                    .value(reader.namespaceUri(), u"id")
                    .toString()
            };
            line += u'[' + QString::number(references.size()) + u']';
            reader.skipCurrentElement();

        } else if (isW_(reader, u"p")) {
            readParagraph_(reader, after, references);

        } else if (isMc_(reader, u"AlternateContent")) {
            // The first choice is read and the rest, the fallback included,
            // skipped: each holds the same content in another form
            auto read_one = false;

            while (reader.readNextStartElement()) {
                if (!read_one && isMc_(reader, u"Choice")) {
                    readInline_(reader, line, after, references);
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
            readInline_(reader, line, after, references);
        }
    }
}

inline void readParagraph_(
    QXmlStreamReader& reader,
    QStringList& lines,
    QList<NoteReference_>& references)
{
    QString line{};
    QStringList after{};

    readInline_(reader, line, after, references);

    lines << line;
    lines << after;
}

// Everything between paragraphs (the body, tables, rows, cells, content
// controls) is read through to the paragraphs inside it
inline void readBlocks_(
    QXmlStreamReader& reader,
    QStringList& lines,
    QList<NoteReference_>& references)
{
    while (reader.readNextStartElement()) {
        if (isSkipped_(reader)) {
            reader.skipCurrentElement();

        } else if (isW_(reader, u"p")) {
            readParagraph_(reader, lines, references);

        } else if (isMc_(reader, u"AlternateContent")) {
            auto read_one = false;

            while (reader.readNextStartElement()) {
                if (!read_one && isMc_(reader, u"Choice")) {
                    readBlocks_(reader, lines, references);
                    read_one = true;
                } else {
                    reader.skipCurrentElement();
                }
            }

        } else {
            readBlocks_(reader, lines, references);
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

// The part a relationship of the given type names, from a part's
// relationships. Types end in the same word ("/officeDocument", "/footnotes")
// in both the ordinary and the Strict format. nullopt when there's no such
// relationship, or its target is outside the package
[[nodiscard]] inline std::optional<QString> relatedPart_(
    const QByteArray& relationships,
    const QString& baseFolder,
    QStringView type)
{
    auto suffix = u'/' + type.toString();
    QXmlStreamReader reader(relationships);

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement() ||
            reader.namespaceUri() != RELATIONSHIPS_ ||
            reader.name() != u"Relationship") {
            continue;
        }

        auto attributes = reader.attributes();

        if (!attributes.value(u"Type").endsWith(suffix) ||
            attributes.value(u"TargetMode") == u"External") {
            continue;
        }

        return resolveTarget_(
            baseFolder,
            attributes.value(u"Target").toString());
    }

    return std::nullopt;
}

// The folder a part is in ("word" for "word/document.xml"), empty at the root
[[nodiscard]] inline QString folderOf_(const QString& part)
{
    auto slash = part.lastIndexOf(u'/');
    return slash < 0 ? QString{} : part.left(slash);
}

// Where a part's own relationships are: "word/_rels/document.xml.rels" for
// "word/document.xml"
[[nodiscard]] inline QString relationshipsOf_(const QString& part)
{
    auto folder = folderOf_(part);
    auto name = part.mid(part.lastIndexOf(u'/') + 1);
    auto relationships = u"_rels/"_s + name + u".rels"_s;
    return folder.isEmpty() ? relationships : folder + u'/' + relationships;
}

// Each note's text by its id, from a footnotes or endnotes part, empty for a
// part the document doesn't have. A note's paragraphs join with spaces, and
// its tabs and breaks become spaces, so each note reads as one line. nullopt
// when the XML is malformed or its root isn't the part expected
[[nodiscard]] inline std::optional<QHash<QString, QString>>
notesOf_(const QByteArray& notesXml, QStringView root, QStringView note)
{
    QHash<QString, QString> notes{};

    if (notesXml.isEmpty()) {
        return notes;
    }

    QXmlStreamReader reader(notesXml);

    if (!reader.readNextStartElement() || !isW_(reader, root)) {
        return std::nullopt;
    }

    // A note holds no references of its own, so none are kept
    QList<NoteReference_> no_references{};

    while (reader.readNextStartElement()) {
        if (!isW_(reader, note)) {
            reader.skipCurrentElement();
            continue;
        }

        auto id =
            reader.attributes().value(reader.namespaceUri(), u"id").toString();

        QStringList lines{};
        readBlocks_(reader, lines, no_references);

        QStringList paragraphs{};

        for (auto line : lines) {
            line.replace(u'\n', u' ').replace(u'\t', u' ');
            line = line.trimmed();

            if (!line.isEmpty()) {
                paragraphs << line;
            }
        }

        notes.insert(id, paragraphs.join(u' '));
    }

    if (reader.hasError()) {
        WARN(
            "Malformed Word notes XML at line {} (Error: {})!",
            reader.lineNumber(),
            reader.errorString());
        return std::nullopt;
    }

    return notes;
}

// A part named by the main document's relationships: nullopt when it's named
// but can't be read, empty bytes when it isn't named
[[nodiscard]] inline std::optional<QByteArray> readRelatedPart_(
    const Coco::Path& path,
    const std::optional<QByteArray>& relationships,
    const QString& baseFolder,
    QStringView type)
{
    if (!relationships) {
        return QByteArray{};
    }

    auto part = relatedPart_(*relationships, baseFolder, type);

    if (!part) {
        return QByteArray{};
    }

    auto bytes = Zip::readEntry(path, *part, MAX_PART_SIZE_);

    if (!bytes) {
        WARN("Failed to read {} from {}!", *part, path);
    }

    return bytes;
}

} // namespace Internal

// The text of a main document part (document.xml), with its notes listed
// after it. The notes parts are empty for a document without them. nullopt
// when any part's XML is malformed or its root isn't the part expected
[[nodiscard]] inline std::optional<QString> textOf(
    const QByteArray& documentXml,
    const QByteArray& footnotesXml,
    const QByteArray& endnotesXml)
{
    QXmlStreamReader reader(documentXml);

    if (!reader.readNextStartElement() ||
        !Internal::isW_(reader, u"document")) {
        return std::nullopt;
    }

    QStringList lines{};
    QList<Internal::NoteReference_> references{};
    Internal::readBlocks_(reader, lines, references);

    if (reader.hasError()) {
        WARN(
            "Malformed Word document XML at line {} (Error: {})!",
            reader.lineNumber(),
            reader.errorString());
        return std::nullopt;
    }

    auto footnotes =
        Internal::notesOf_(footnotesXml, u"footnotes", u"footnote");
    auto endnotes = Internal::notesOf_(endnotesXml, u"endnotes", u"endnote");

    if (!footnotes || !endnotes) {
        return std::nullopt;
    }

    auto text = lines.join(u'\n');

    if (references.isEmpty()) {
        return text;
    }

    // A reference to a note the document doesn't have keeps its marker, with
    // nothing after it
    QStringList notes{};

    for (auto i = 0; i < references.size(); ++i) {
        const auto& reference = references.at(i);
        auto note =
            (reference.isEndnote ? *endnotes : *footnotes).value(reference.id);
        auto marker = u'[' + QString::number(i + 1) + u']';
        notes << (note.isEmpty() ? marker : marker + u' ' + note);
    }

    return text + u"\n\n"_s + notes.join(u'\n');
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

    auto part =
        Internal::relatedPart_(*relationships, QString{}, u"officeDocument");

    if (!part) {
        WARN("{} names no main document!", path);
        return { QString{}, Failure::NotAWordDocument };
    }

    auto xml = Zip::readEntry(path, *part, Internal::MAX_PART_SIZE_);

    if (!xml) {
        WARN("Failed to read {} from {}!", *part, path);
        return { QString{}, Failure::NotAWordDocument };
    }

    // The main document's own relationships are optional: a document
    // without them has no notes
    auto part_relationships = Zip::readEntry(
        path,
        Internal::relationshipsOf_(*part),
        Internal::MAX_PART_SIZE_);
    auto folder = Internal::folderOf_(*part);

    auto footnotes = Internal::readRelatedPart_(
        path,
        part_relationships,
        folder,
        u"footnotes");
    auto endnotes = Internal::readRelatedPart_(
        path,
        part_relationships,
        folder,
        u"endnotes");

    if (!footnotes || !endnotes) {
        return { QString{}, Failure::NotAWordDocument };
    }

    auto text = textOf(*xml, *footnotes, *endnotes);

    if (!text) {
        return { QString{}, Failure::Damaged };
    }

    return { *text, Failure::None };
}

} // namespace Fernanda::Docx
