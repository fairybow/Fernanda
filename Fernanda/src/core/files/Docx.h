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
#include <Qt>
#include <QtTypes>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/files/Zip.h"

// Word documents (.docx) read as plain text: one line per paragraph, in reading
// order, with formatting dropped. Only the text a reader of the document sees
// is kept: deleted tracked changes (deleted table rows among them), field codes
// (nested fields inside them included), hidden text, and Word's fallback copies
// of content are skipped, and a paragraph whose mark is a deleted tracked
// change, or hidden, joins the next, as Word shows it. A text box's paragraphs
// follow the paragraph it's anchored to, and a table's paragraphs come row by
// row. Ruby is written in Aozora Bunko's notation, base and reading together.
// Footnotes and endnotes are marked [1], [2], and so on, numbered together in
// the order the text shows them, and listed after the text, each on one line.
// Headers, footers, and comments are left out.
//
// Elements are matched by namespace as well as name, so documents in Word's
// Strict Open XML format read the same as ordinary ones. A document nested
// deeper than any real one is refused as damaged, since each level of nesting
// is a level of recursion here
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

// The deepest nesting of elements read. Real documents nest a few dozen
// levels; each level is two calls on the stack, so a crafted document with
// tens of thousands could overflow it
constexpr int MAX_NESTING_ = 256;

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

// What reading one part carries from element to element: the note
// references made so far, how deeply the reader is nested, and the fields
// open where it is. A complex field runs from its begin mark to its end mark,
// across runs and paragraphs, with its instruction before its separate mark
// and its result after. Fields nest, and each open field is true here while
// its instruction is being read
struct ReadState_
{
    QList<NoteReference_> references{};
    int nesting = 0;
    QList<bool> openFields{};

    // Is the reader inside a field's instruction, at any level? A nested
    // field there, result and all, is part of the outer field's instruction
    [[nodiscard]] bool isInFieldInstruction() const
    {
        return openFields.contains(true);
    }
};

// Where a note reference sits in a line while the part is read: a mark, the
// reference's index in ReadState_::references, and a closing mark. Its number
// is given once the text is whole (numberNotes_), so notes are numbered in the
// order the text shows them, where a text box's lines come after its
// paragraph's. The marks are Unicode noncharacters, set aside for a program's
// own use, and any in a document's own text are dropped as it's read, so a
// mark is always one of these
constexpr QChar NOTE_MARK_OPEN_ = u'\uFDD0';
constexpr QChar NOTE_MARK_CLOSE_ = u'\uFDD1';

// Aozora Bunko's ruby marks: ｜ where the base begins, and 《 》 around the
// reading
constexpr QChar RUBY_BASE_MARK_ = u'\uFF5C';
constexpr QChar RUBY_OPEN_ = u'\u300A';
constexpr QChar RUBY_CLOSE_ = u'\u300B';

// One paragraph as read: its line, the lines of the text boxes anchored in it,
// and whether its mark joins it to the next paragraph (readMarkJoins_)
struct Paragraph_
{
    QString line{};
    QStringList after{};
    bool markJoins = false;
};

// The lines a container's paragraphs make, in order. A paragraph whose mark
// joins is held, and the paragraphs after it join its line until one whose mark
// doesn't, as Word shows them once a deleted mark is accepted, or with a hidden
// mark. The joined line is kept even when empty, as any empty paragraph is.
// endJoin ends a held paragraph where a join can't reach past (a table, a cell,
// a text box, the container's end), and drops its line if it's still empty, as
// it is when the whole paragraph was deleted. A paragraph's text boxes follow
// its line, and a joined paragraph's follow the joined line
struct Lines_
{
    QStringList lines{};

    void add(const Paragraph_& paragraph)
    {
        heldLine_ += paragraph.line;
        heldAfter_ << paragraph.after;

        if (!paragraph.markJoins) {
            lines << heldLine_;
            release_();
        }
    }

    void endJoin()
    {
        if (!heldLine_.isEmpty()) {
            lines << heldLine_;
        }

        release_();
    }

private:
    QString heldLine_{};
    QStringList heldAfter_{};

    void release_()
    {
        lines << heldAfter_;
        heldLine_.clear();
        heldAfter_.clear();
    }
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
// paragraph mark's formatting, not text, and which readParagraph_ reads only
// for whether the mark is deleted), and the section's page setup
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

// Reads a paragraph's properties to their end and says whether its mark joins
// the paragraph to the next: a deleted tracked change (deleted, or moved away
// to another place), or hidden. A hidden mark is how Word's style separator
// runs a heading into the paragraph after it (specVanish), and any other
// hidden mark shows the same way
[[nodiscard]] inline bool readMarkJoins_(QXmlStreamReader& reader)
{
    auto joins = false;

    while (reader.readNextStartElement()) {
        if (!isW_(reader, u"rPr")) {
            reader.skipCurrentElement();
            continue;
        }

        while (reader.readNextStartElement()) {
            if (isW_(reader, u"del") || isW_(reader, u"moveFrom") ||
                ((isW_(reader, u"vanish") || isW_(reader, u"specVanish")) &&
                 isOn_(reader))) {
                joins = true;
            }

            reader.skipCurrentElement();
        }
    }

    return joins;
}

// Reads a table row's properties to their end and says whether the row is a
// deleted tracked change, which accepting removes
[[nodiscard]] inline bool readRowIsDeleted_(QXmlStreamReader& reader)
{
    auto deleted = false;

    while (reader.readNextStartElement()) {
        if (isW_(reader, u"del")) {
            deleted = true;
        }

        reader.skipCurrentElement();
    }

    return deleted;
}

// A complex field's mark (<w:fldChar w:fldCharType="begin"/>), read to its
// end and applied to the open fields: begin opens a field in its instruction,
// separate moves the innermost open field to its result, and end closes it.
// A separate or end with no field open, as only a damaged document has,
// changes nothing
inline void readFieldMark_(QXmlStreamReader& reader, QList<bool>& openFields)
{
    auto type =
        reader.attributes().value(reader.namespaceUri(), u"fldCharType");

    if (type == u"begin") {
        openFields << true;
    } else if (type == u"separate" && !openFields.isEmpty()) {
        openFields.last() = false;
    } else if (type == u"end" && !openFields.isEmpty()) {
        openFields.removeLast();
    }

    reader.skipCurrentElement();
}

// The rest of a hidden run, read to the run's end. Its text isn't shown, but
// its field marks still begin, separate, and end fields, as Word keeps them:
// skipped, a field hidden only in part would stay open past its end, or close
// the field around it
inline void
readHiddenRunRest_(QXmlStreamReader& reader, QList<bool>& openFields)
{
    while (reader.readNextStartElement()) {
        if (isW_(reader, u"fldChar")) {
            readFieldMark_(reader, openFields);
        } else {
            reader.skipCurrentElement();
        }
    }
}

// Does the element add to a line directly? What a field's instruction holds
// is skipped by this test, so containers (runs, paragraphs) are still read
// through for the field marks inside them
[[nodiscard]] inline bool addsText_(const QXmlStreamReader& reader)
{
    return isW_(reader, u"t") || isW_(reader, u"tab") ||
           isW_(reader, u"ptab") || isW_(reader, u"br") ||
           isW_(reader, u"cr") || isW_(reader, u"noBreakHyphen") ||
           isW_(reader, u"footnoteReference") ||
           isW_(reader, u"endnoteReference");
}

// Text without the noncharacters that mark note references (NOTE_MARK_OPEN_),
// which mean nothing in a document's own text
[[nodiscard]] inline QString withoutNoteMarks_(QString text)
{
    text.remove(NOTE_MARK_OPEN_);
    text.remove(NOTE_MARK_CLOSE_);
    return text;
}

inline void readInline_(
    QXmlStreamReader& reader,
    QString& line,
    Lines_& after,
    ReadState_& state);

[[nodiscard]] inline Paragraph_
readParagraph_(QXmlStreamReader& reader, ReadState_& state);

// A ruby's base and reading, as ｜base《reading》. A ruby with no reading is
// just its base
[[nodiscard]] inline QString
readRuby_(QXmlStreamReader& reader, Lines_& after, ReadState_& state)
{
    QString base{};
    QString reading{};

    while (reader.readNextStartElement()) {
        if (isW_(reader, u"rt")) {
            readInline_(reader, reading, after, state);
        } else if (isW_(reader, u"rubyBase")) {
            readInline_(reader, base, after, state);
        } else {
            reader.skipCurrentElement();
        }
    }

    if (reading.isEmpty()) {
        return base;
    }

    return RUBY_BASE_MARK_ + base + RUBY_OPEN_ + reading + RUBY_CLOSE_;
}

// One element the reader has just entered, read to its end and added to the
// paragraph's line. A paragraph met inside it (a text box's) goes to after, to
// follow the paragraph being read. A note reference is added to the state's
// references, and its mark to the line (NOTE_MARK_OPEN_). False when the
// element is properties that take away what holds them (a hidden run's, a
// deleted table row's): the rest of the run or row has been read past, and
// its reading ends. Every level of nesting passes through here, so here is
// where nesting past MAX_NESTING_ is refused, as an XML error that ends the
// whole read
[[nodiscard]] inline bool readInlineElement_(
    QXmlStreamReader& reader,
    QString& line,
    Lines_& after,
    ReadState_& state)
{
    auto keep_reading = true;
    ++state.nesting;

    if (state.nesting > MAX_NESTING_) {
        reader.raiseError(u"Elements are nested too deeply"_s);

    } else if (isSkipped_(reader)) {
        reader.skipCurrentElement();

    } else if (isW_(reader, u"fldChar")) {
        readFieldMark_(reader, state.openFields);

    } else if (state.isInFieldInstruction() && addsText_(reader)) {
        // A field's instruction, a nested field's result inside it included,
        // isn't shown: only the outer field's result is
        reader.skipCurrentElement();

    } else if (isW_(reader, u"t")) {
        line += withoutNoteMarks_(reader.readElementText());

    } else if (isW_(reader, u"rPr")) {
        // A run's properties come first in the run, so a hidden run is read
        // from here to the run's end for its field marks alone
        if (readRunIsHidden_(reader)) {
            readHiddenRunRest_(reader, state.openFields);
            keep_reading = false;
        }

    } else if (isW_(reader, u"trPr")) {
        // A row's properties come first in the row, so a deleted row is
        // skipped from here to the row's end
        if (readRowIsDeleted_(reader)) {
            reader.skipCurrentElement();
            keep_reading = false;
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
        state.references << NoteReference_{
            reader.name() == u"endnoteReference",
            reader.attributes().value(reader.namespaceUri(), u"id").toString()
        };
        line += NOTE_MARK_OPEN_ + QString::number(state.references.size() - 1) +
                NOTE_MARK_CLOSE_;
        reader.skipCurrentElement();

    } else if (isW_(reader, u"ruby")) {
        line += readRuby_(reader, after, state);

    } else if (isW_(reader, u"p")) {
        after.add(readParagraph_(reader, state));

    } else if (
        isW_(reader, u"tbl") || isW_(reader, u"tc") ||
        isW_(reader, u"txbxContent")) {
        // A join doesn't reach into or out of a table, a cell, or a text box
        after.endJoin();
        readInline_(reader, line, after, state);
        after.endJoin();

    } else if (isMc_(reader, u"AlternateContent")) {
        // The first choice is read and the rest, the fallback included,
        // skipped: each holds the same content in another form
        auto read_one = false;

        while (reader.readNextStartElement()) {
            if (!read_one && isMc_(reader, u"Choice")) {
                readInline_(reader, line, after, state);
                read_one = true;
            } else {
                reader.skipCurrentElement();
            }
        }

    } else {
        // Runs, hyperlinks, inserted and moved-to changes, fields' results,
        // content controls, table rows, drawings on the way to a text box:
        // what's inside them is read as if they weren't there. Anything else
        // with no text (an optional hyphen, a symbol-font character)
        // contributes nothing
        readInline_(reader, line, after, state);
    }

    --state.nesting;
    return keep_reading;
}

// The content of whatever element the reader is in, up to its end
inline void readInline_(
    QXmlStreamReader& reader,
    QString& line,
    Lines_& after,
    ReadState_& state)
{
    while (reader.readNextStartElement()) {
        if (!readInlineElement_(reader, line, after, state)) {
            return;
        }
    }
}

// A paragraph's properties are read only for whether its mark joins it to the
// next. Its text boxes' paragraphs can't join past the paragraph
[[nodiscard]] inline Paragraph_
readParagraph_(QXmlStreamReader& reader, ReadState_& state)
{
    Paragraph_ paragraph{};
    Lines_ after{};

    while (reader.readNextStartElement()) {
        if (isW_(reader, u"pPr")) {
            paragraph.markJoins = readMarkJoins_(reader);
            continue;
        }

        if (!readInlineElement_(reader, paragraph.line, after, state)) {
            break;
        }
    }

    after.endJoin();
    paragraph.after = after.lines;
    return paragraph;
}

// The lines of whatever element the reader is in (the body, a note), up to
// its end. Everything between paragraphs (tables, rows, cells, content
// controls) is read through to the paragraphs inside it, as inline content
// is. Text is never outside a paragraph in a valid part, so what would go to
// a line here is dropped
[[nodiscard]] inline QStringList
readBlocks_(QXmlStreamReader& reader, ReadState_& state)
{
    Lines_ lines{};
    QString outside_paragraphs{};

    readInline_(reader, outside_paragraphs, lines, state);
    lines.endJoin();

    return lines.lines;
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

    // A note holds no references of its own, so the state's go unused
    ReadState_ state{};

    while (reader.readNextStartElement()) {
        if (!isW_(reader, note)) {
            reader.skipCurrentElement();
            continue;
        }

        auto id =
            reader.attributes().value(reader.namespaceUri(), u"id").toString();

        auto lines = readBlocks_(reader, state);

        QStringList paragraphs{};

        for (auto line : lines) {
            line.replace(u'\n', u' ').replace(u'\t', u' ');
            line = withoutNoteMarks_(line);
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

// The whole text with each note reference's mark (NOTE_MARK_OPEN_) numbered
// [1], [2], and so on in the order the text shows them, and the notes listed
// after it in that order, one per line. A reference to a note the document
// doesn't have keeps its number, with nothing after it in the list
[[nodiscard]] inline QString numberNotes_(
    const QString& text,
    const QList<NoteReference_>& references,
    const QHash<QString, QString>& footnotes,
    const QHash<QString, QString>& endnotes)
{
    QString numbered{};
    QStringList notes{};
    qsizetype from = 0;

    while (from < text.size()) {
        auto open = text.indexOf(NOTE_MARK_OPEN_, from);
        auto close =
            open < 0 ? qsizetype(-1) : text.indexOf(NOTE_MARK_CLOSE_, open);

        if (close < 0) {
            numbered += QStringView(text).mid(from);
            break;
        }

        numbered += QStringView(text).mid(from, open - from);

        auto index = QStringView(text).mid(open + 1, close - open - 1).toInt();
        const auto& reference = references.at(index);
        auto note =
            (reference.isEndnote ? endnotes : footnotes).value(reference.id);
        auto marker = u'[' + QString::number(notes.size() + 1) + u']';

        numbered += marker;
        notes << (note.isEmpty() ? marker : marker + u' ' + note);
        from = close + 1;
    }

    if (notes.isEmpty()) {
        return numbered;
    }

    return numbered + u"\n\n"_s + notes.join(u'\n');
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

    auto bytes =
        Zip::readEntry(path, *part, Qt::CaseInsensitive, MAX_PART_SIZE_);

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

    Internal::ReadState_ state{};
    auto lines = Internal::readBlocks_(reader, state);

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

    return Internal::numberNotes_(
        lines.join(u'\n'),
        state.references,
        *footnotes,
        *endnotes);
}

// The Word file's text, or why it couldn't be read. A file that fails gives no
// text at all, never part of it. Parts are found whatever the letter case of
// their names, as the Open Packaging Conventions compare them, since not every
// program that writes Word files matches the case its relationships give
[[nodiscard]] inline Conversion toPlainText(const Coco::Path& path)
{
    auto relationships = Zip::readEntry(
        path,
        u"_rels/.rels"_s,
        Qt::CaseInsensitive,
        Internal::MAX_PART_SIZE_);

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

    auto xml = Zip::readEntry(
        path,
        *part,
        Qt::CaseInsensitive,
        Internal::MAX_PART_SIZE_);

    if (!xml) {
        WARN("Failed to read {} from {}!", *part, path);
        return { QString{}, Failure::NotAWordDocument };
    }

    // The main document's own relationships are optional: a document
    // without them has no notes
    auto part_relationships = Zip::readEntry(
        path,
        Internal::relationshipsOf_(*part),
        Qt::CaseInsensitive,
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
