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

#include <utility>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/files/Docx.h"
#include "core/files/Io.h"
#include "core/files/Zip.h"

using namespace Qt::StringLiterals;

// Word documents as plain text (core/files/Docx.h). The text tests give
// textOf the parts' XML directly; the file tests build .docx files with
// Zip::write, so no binary test files are kept in the repo. Each file test
// works in its own temporary folder.
//
// Qt Test runs every private slot as a test
class DocxTest : public QObject
{
    Q_OBJECT

private:
    // The namespaces a document part declares, so each test's body can use
    // any of their prefixes
    [[nodiscard]] static QByteArray
    document_(const QByteArray& body, const QByteArray& wordNamespace)
    {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               "<w:document xmlns:w=\"" +
               wordNamespace +
               "\" "
               "xmlns:mc=\"http://schemas.openxmlformats.org/"
               "markup-compatibility/2006\""
               " xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/"
               "wordprocessingDrawing\""
               " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/"
               "main\""
               " xmlns:wps=\"http://schemas.microsoft.com/office/word/2010/"
               "wordprocessingShape\""
               " xmlns:v=\"urn:schemas-microsoft-com:vml\">"
               "<w:body>" +
               body + "</w:body></w:document>";
    }

    [[nodiscard]] static QByteArray document_(const QByteArray& body)
    {
        return document_(
            body,
            "http://schemas.openxmlformats.org/wordprocessingml/2006/main");
    }

    [[nodiscard]] static QString textOf_(const QByteArray& body)
    {
        return Fernanda::Docx::textOf(document_(body), {}, {})
            .value_or(u"(none)"_s);
    }

    // A footnotes or endnotes part (root "footnotes" or "endnotes"), with the
    // separator notes Word writes first
    [[nodiscard]] static QByteArray
    notesPart_(const QByteArray& root, const QByteArray& notes)
    {
        auto note = root.chopped(1);

        return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               "<w:" +
               root +
               " xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/"
               "2006/main\">"
               "<w:" +
               note +
               " w:type=\"separator\" w:id=\"-1\"><w:p><w:r><w:separator/>"
               "</w:r></w:p></w:" +
               note + ">" + notes + "</w:" + root + ">";
    }

    [[nodiscard]] static QString textWithNotes_(
        const QByteArray& body,
        const QByteArray& footnotes,
        const QByteArray& endnotes)
    {
        return Fernanda::Docx::textOf(document_(body), footnotes, endnotes)
            .value_or(u"(none)"_s);
    }

    // A part's relationships, each given as its type's last word and its
    // target
    [[nodiscard]] static QByteArray
    relationshipsOf_(const QList<std::pair<QByteArray, QByteArray>>& targets)
    {
        QByteArray xml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships "
            "xmlns=\"http://schemas.openxmlformats.org/package/2006/"
            "relationships\">";

        for (auto i = 0; i < targets.size(); ++i) {
            xml += "<Relationship Id=\"rId" + QByteArray::number(i + 1) +
                   "\" "
                   "Type=\"http://schemas.openxmlformats.org/officeDocument/"
                   "2006/relationships/" +
                   targets.at(i).first + "\" Target=\"" + targets.at(i).second +
                   "\"/>";
        }

        return xml + "</Relationships>";
    }

    // Package relationships naming target as the main document
    [[nodiscard]] static QByteArray relationships_(const QByteArray& target)
    {
        return relationshipsOf_(
            {
                { "officeDocument", target }
        });
    }

    struct Fixture_
    {
        QTemporaryDir folder{};

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return Coco::Path(folder.filePath(relative));
        }

        void write(const QString& relative, const QByteArray& bytes) const
        {
            QVERIFY(
                Fernanda::Io::write(
                    bytes,
                    absolute(relative),
                    Fernanda::Io::CreateDirs::Yes));
        }

        // A zip of the parts, each given as its name in the archive and its
        // bytes
        [[nodiscard]] Coco::Path
        zip(const QList<std::pair<QString, QByteArray>>& parts) const
        {
            QList<Fernanda::Zip::Entry> entries{};

            for (auto i = 0; i < parts.size(); ++i) {
                auto source = u"parts/"_s + QString::number(i);
                write(source, parts.at(i).second);
                entries << Fernanda::Zip::Entry{ absolute(source),
                                                 parts.at(i).first };
            }

            auto archive = absolute(u"test.docx"_s);
            Fernanda::Zip::write(archive, entries);
            return archive;
        }
    };

private slots:
    void paragraphsBecomeLines()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>One</w:t></w:r></w:p>"
                "<w:p/>"
                "<w:p><w:r><w:t>Two</w:t></w:r></w:p>"),
            u"One\n\nTwo"_s);
    }

    // A paragraph's runs join with nothing between them, keeping their own
    // spaces
    void runsJoinWithTheirSpaces()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t xml:space=\"preserve\">Dear </w:t></w:r>"
                "<w:r><w:t>Cun</w:t></w:r><w:r><w:t>egonde</w:t></w:r></w:p>"),
            u"Dear Cunegonde"_s);
    }

    void tabsAndBreaksAreKept()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>a</w:t><w:tab/><w:t>b</w:t><w:br/>"
                "<w:t>c</w:t><w:cr/><w:t>d</w:t>"
                "<w:br w:type=\"page\"/><w:t>e</w:t></w:r></w:p>"),
            u"a\tb\nc\nd\ne"_s);
    }

    // A non-breaking hyphen is kept as one; an optional hyphen shows only
    // where Word wraps a line, so it's dropped
    void hyphensAreKeptOrDropped()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>well</w:t><w:noBreakHyphen/><w:t>known "
                "every</w:t><w:softHyphen/><w:t>where</w:t></w:r></w:p>"),
            u"well\u2011known everywhere"_s);
    }

    void trackedChangesReadAsAccepted()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>The </w:t></w:r>"
                "<w:del><w:r><w:delText>old </w:delText></w:r></w:del>"
                "<w:ins><w:r><w:t>new </w:t></w:r></w:ins>"
                "<w:moveFrom><w:r><w:t>moved </w:t></w:r></w:moveFrom>"
                "<w:r><w:t>text</w:t></w:r>"
                "<w:moveTo><w:r><w:t> moved</w:t></w:r></w:moveTo></w:p>"),
            u"The new text moved"_s);
    }

    // A paragraph deleted whole, with its mark, leaves no line behind
    void aDeletedParagraphLeavesNoLine()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:r><w:t>First</w:t></w:r></w:p>)"
                R"(<w:p><w:pPr><w:rPr><w:del w:id="1" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:del w:id="2" w:author="A"><w:r>)"
                R"(<w:delText>Cut.</w:delText></w:r></w:del></w:p>)"
                R"(<w:p><w:r><w:t>Kept</w:t></w:r></w:p>)"),
            u"First\nKept"_s);
    }

    // A deleted mark joins its paragraph to the next with nothing between
    // them, and a run of deleted marks joins every paragraph up to the first
    // whose mark stays
    void aDeletedMarkJoinsTheNextParagraph()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:pPr><w:rPr><w:del w:id="1" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:r><w:t>The end</w:t></w:r></w:p>)"
                R"(<w:p><w:pPr><w:rPr><w:del w:id="2" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:r><w:t>ing</w:t></w:r></w:p>)"
                R"(<w:p><w:r><w:t>.</w:t></w:r></w:p>)"
                R"(<w:p><w:r><w:t>Next</w:t></w:r></w:p>)"),
            u"The ending.\nNext"_s);
    }

    // A hidden mark joins its paragraph to the next, as Word shows it: here
    // Word's style separator (specVanish) running a heading into its body,
    // then a mark hidden directly. A mark whose hiding is switched off stays
    void aHiddenMarkJoinsTheNextParagraph()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:pPr><w:rPr><w:specVanish/></w:rPr></w:pPr>)"
                R"(<w:r><w:t>Heading.</w:t></w:r></w:p>)"
                R"(<w:p><w:r><w:t xml:space="preserve"> Body</w:t></w:r></w:p>)"
                R"(<w:p><w:pPr><w:rPr><w:vanish/></w:rPr></w:pPr>)"
                R"(<w:r><w:t>Run</w:t></w:r></w:p>)"
                R"(<w:p><w:r><w:t>-in</w:t></w:r></w:p>)"
                R"(<w:p><w:pPr><w:rPr><w:vanish w:val="0"/></w:rPr></w:pPr>)"
                R"(<w:r><w:t>A</w:t></w:r></w:p>)"
                R"(<w:p><w:r><w:t>B</w:t></w:r></w:p>)"),
            u"Heading. Body\nRun-in\nA\nB"_s);
    }

    // Joined onto an empty paragraph that stays, a deleted one leaves that
    // empty line, as any empty paragraph does
    void aJoinOntoAnEmptyParagraphKeepsItsLine()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:r><w:t>A</w:t></w:r></w:p>)"
                R"(<w:p><w:pPr><w:rPr><w:del w:id="1" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:del w:id="2" w:author="A"><w:r>)"
                R"(<w:delText>Cut.</w:delText></w:r></w:del></w:p>)"
                R"(<w:p/>)"
                R"(<w:p><w:r><w:t>B</w:t></w:r></w:p>)"),
            u"A\n\nB"_s);
    }

    // A join doesn't reach into a table: the paragraph before it keeps its
    // own line, and one left empty is dropped
    void aJoinStopsAtATable()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:pPr><w:rPr><w:del w:id="1" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:r><w:t>Before</w:t></w:r></w:p>)"
                R"(<w:tbl><w:tr><w:tc><w:p><w:r><w:t>cell</w:t></w:r>)"
                R"(</w:p></w:tc></w:tr></w:tbl>)"
                R"(<w:p><w:pPr><w:rPr><w:del w:id="2" w:author="A"/>)"
                R"(</w:rPr></w:pPr></w:p>)"
                R"(<w:tbl><w:tr><w:tc><w:p><w:r><w:t>next</w:t></w:r>)"
                R"(</w:p></w:tc></w:tr></w:tbl>)"),
            u"Before\ncell\nnext"_s);
    }

    // A moved paragraph's mark at its old place is moved away with it
    void aMovedParagraphLeavesNoLine()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:r><w:t>One</w:t></w:r></w:p>)"
                R"(<w:p><w:pPr><w:rPr><w:moveFrom w:id="1" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:moveFrom w:id="2" w:author="A">)"
                R"(<w:r><w:t>Moved</w:t></w:r></w:moveFrom></w:p>)"
                R"(<w:p><w:r><w:t>Two</w:t></w:r></w:p>)"
                R"(<w:p><w:moveTo w:id="3" w:author="A"><w:r><w:t>Moved</w:t>)"
                R"(</w:r></w:moveTo></w:p>)"),
            u"One\nTwo\nMoved"_s);
    }

    // A text box in a joined paragraph follows the joined line
    void aJoinedParagraphsTextBoxFollowsTheJoinedLine()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:pPr><w:rPr><w:del w:id="1" w:author="A"/>)"
                R"(</w:rPr></w:pPr><w:r><w:t xml:space="preserve">Start </w:t>)"
                R"(</w:r><w:r><w:pict><v:shape><v:textbox><w:txbxContent>)"
                R"(<w:p><w:r><w:t>Box</w:t></w:r></w:p></w:txbxContent>)"
                R"(</v:textbox></v:shape></w:pict></w:r></w:p>)"
                R"(<w:p><w:r><w:t>end</w:t></w:r></w:p>)"),
            u"Start end\nBox"_s);
    }

    // Ruby is written in Aozora Bunko's notation, base before reading: here
    // two kanji with a reading of three hiragana
    void rubyIsWrittenInAozoraNotation()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>A </w:t></w:r><w:r><w:ruby>"
                "<w:rubyPr><w:hps w:val=\"10\"/></w:rubyPr>"
                "<w:rt><w:r><w:t>\xE3\x81\x8B\xE3\x82\x93\xE3\x81\x98</w:t>"
                "</w:r></w:rt>"
                "<w:rubyBase><w:r><w:t>\xE6\xBC\xA2\xE5\xAD\x97</w:t></w:r>"
                "</w:rubyBase></w:ruby></w:r><w:r><w:t> B</w:t></w:r></w:p>"),
            u"A \uFF5C\u6F22\u5B57\u300A\u304B\u3093\u3058\u300B B"_s);
    }

    void rubyWithNoReadingIsItsBase()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:ruby><w:rt/>"
                "<w:rubyBase><w:r><w:t>base</w:t></w:r></w:rubyBase>"
                "</w:ruby></w:r></w:p>"),
            u"base"_s);
    }

    // A field's instruction is skipped and its result kept, for both kinds of
    // field
    void fieldsReadAsTheirResults()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t xml:space=\"preserve\">Page </w:t></w:r>"
                "<w:r><w:fldChar w:fldCharType=\"begin\"/></w:r>"
                "<w:r><w:instrText xml:space=\"preserve\"> PAGE "
                "</w:instrText></w:r>"
                "<w:r><w:fldChar w:fldCharType=\"separate\"/></w:r>"
                "<w:r><w:t>3</w:t></w:r>"
                "<w:r><w:fldChar w:fldCharType=\"end\"/></w:r></w:p>"
                "<w:p><w:fldSimple w:instr=\" NUMPAGES \">"
                "<w:r><w:t>9</w:t></w:r></w:fldSimple></w:p>"),
            u"Page 3\n9"_s);
    }

    void hiddenTextIsSkipped()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>Seen</w:t></w:r>"
                "<w:r><w:rPr><w:i/><w:vanish/></w:rPr><w:t> secret</w:t></w:r>"
                "<w:r><w:rPr><w:vanish w:val=\"0\"/></w:rPr><w:t> "
                "shown</w:t></w:r></w:p>"),
            u"Seen shown"_s);
    }

    void hyperlinksAndContentControlsKeepTheirText()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:hyperlink><w:r><w:t>link</w:t></w:r></w:hyperlink>"
                "<w:sdt><w:sdtContent><w:r><w:t> control</w:t></w:r>"
                "</w:sdtContent></w:sdt></w:p>"),
            u"link control"_s);
    }

    // Word writes a text box twice, as its choice and its fallback. It's read
    // once, after the paragraph it's anchored in
    void aTextBoxFollowsItsParagraphOnce()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t xml:space=\"preserve\">Before </w:t></w:r>"
                "<w:r><mc:AlternateContent><mc:Choice Requires=\"wps\">"
                "<w:drawing><wp:anchor><a:graphic><a:graphicData><wps:wsp>"
                "<wps:txbx><w:txbxContent><w:p><w:r><w:t>Box</w:t></w:r></w:p>"
                "</w:txbxContent></wps:txbx></wps:wsp></a:graphicData>"
                "</a:graphic></wp:anchor></w:drawing></mc:Choice>"
                "<mc:Fallback><w:pict><v:shape><v:textbox><w:txbxContent>"
                "<w:p><w:r><w:t>Box</w:t></w:r></w:p></w:txbxContent>"
                "</v:textbox></v:shape></w:pict></mc:Fallback>"
                "</mc:AlternateContent></w:r>"
                "<w:r><w:t>after</w:t></w:r></w:p>"
                "<w:p><w:r><w:t>Next</w:t></w:r></w:p>"),
            u"Before after\nBox\nNext"_s);
    }

    void aTableReadsRowByRow()
    {
        QCOMPARE(
            textOf_(
                "<w:tbl><w:tr>"
                "<w:tc><w:p><w:r><w:t>a</w:t></w:r></w:p></w:tc>"
                "<w:tc><w:p><w:r><w:t>b</w:t></w:r></w:p>"
                "<w:p><w:r><w:t>b2</w:t></w:r></w:p></w:tc>"
                "</w:tr><w:tr>"
                "<w:tc><w:p><w:r><w:t>c</w:t></w:r></w:p></w:tc>"
                "<w:tc><w:p><w:r><w:t>d</w:t></w:r></w:p></w:tc>"
                "</w:tr></w:tbl>"),
            u"a\nb\nb2\nc\nd"_s);
    }

    // A row deleted as a tracked change is gone, as accepting it removes it.
    // Other row properties leave a row as it is
    void aDeletedTableRowIsSkipped()
    {
        QCOMPARE(
            textOf_(
                R"(<w:tbl><w:tr><w:trPr><w:trHeight w:val="300"/></w:trPr>)"
                R"(<w:tc><w:p><w:r><w:t>a</w:t></w:r></w:p></w:tc></w:tr>)"
                R"(<w:tr><w:trPr><w:del w:id="1" w:author="A"/></w:trPr>)"
                R"(<w:tc><w:p><w:r><w:t>gone</w:t></w:r></w:p></w:tc></w:tr>)"
                R"(<w:tr><w:tc><w:p><w:r><w:t>c</w:t></w:r></w:p></w:tc>)"
                R"(</w:tr></w:tbl>)"),
            u"a\nc"_s);
    }

    // The paragraph mark's properties and the page setup hold no text, even
    // where their elements share names with ones that do
    void propertiesHoldNoText()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:pPr><w:rPr><w:vanish/></w:rPr></w:pPr>"
                "<w:r><w:t>Body</w:t></w:r></w:p>"
                "<w:sectPr><w:pgSz/></w:sectPr>"),
            u"Body"_s);
    }

    void strictDocumentsReadTheSame()
    {
        auto xml = document_(
            "<w:p><w:r><w:t>Strict</w:t></w:r></w:p>",
            "http://purl.oclc.org/ooxml/wordprocessingml/main");

        QCOMPARE(
            Fernanda::Docx::textOf(xml, {}, {}).value_or(u"(none)"_s),
            u"Strict"_s);
    }

    void textOutsideAsciiIsKept()
    {
        QCOMPARE(
            textOf_("<w:p><w:r><w:t>caf\xC3\xA9</w:t></w:r></w:p>"),
            u"caf\u00E9"_s);
    }

    // A document with no text is still a document: empty text, not nothing
    void anEmptyDocumentIsEmpty()
    {
        auto text = Fernanda::Docx::textOf(document_(QByteArray{}), {}, {});

        QVERIFY(text.has_value());
        QVERIFY(text->isEmpty());
    }

    void malformedXmlIsNothing()
    {
        QVERIFY(!Fernanda::Docx::textOf(document_("<w:p><w:r><w:t>Cut"), {}, {})
                     .has_value());
    }

    // Nesting as deep as a real document's reads
    void deepNestingReads()
    {
        QCOMPARE(
            textOf_(
                "<w:p>" + QByteArray("<x>").repeated(100) +
                "<w:r><w:t>Deep</w:t></w:r>" +
                QByteArray("</x>").repeated(100) + "</w:p>"),
            u"Deep"_s);
    }

    // Nesting far past any real document's is refused as damaged, where
    // reading it level by level would overflow the stack
    void nestingPastTheLimitIsNothing()
    {
        QVERIFY(!Fernanda::Docx::textOf(
                     document_(
                         "<w:p>" + QByteArray("<x>").repeated(100000) +
                         QByteArray("</x>").repeated(100000) + "</w:p>"),
                     {},
                     {})
                     .has_value());
    }

    void aPartThatIsntADocumentIsNothing()
    {
        QVERIFY(!Fernanda::Docx::textOf(
                     "<w:hdr "
                     "xmlns:w=\"http://schemas.openxmlformats.org/"
                     "wordprocessingml/2006/main\">"
                     "<w:p><w:r><w:t>Header</w:t></w:r></w:p></w:hdr>",
                     {},
                     {})
                     .has_value());
    }

    // Footnotes and endnotes are numbered together, in the order the text
    // references them, and listed after it. A note's own number mark is
    // dropped, and its paragraphs and breaks become spaces
    void notesAreMarkedAndListed()
    {
        auto body = QByteArray(
            "<w:p><w:r><w:t>One</w:t></w:r>"
            "<w:r><w:footnoteReference w:id=\"2\"/></w:r>"
            "<w:r><w:t xml:space=\"preserve\"> two</w:t></w:r>"
            "<w:r><w:endnoteReference w:id=\"1\"/></w:r></w:p>"
            "<w:p><w:r><w:t>Three</w:t></w:r>"
            "<w:r><w:footnoteReference w:id=\"3\"/></w:r></w:p>");

        auto footnotes = notesPart_(
            "footnotes",
            "<w:footnote w:id=\"2\"><w:p><w:r><w:footnoteRef/></w:r>"
            "<w:r><w:t xml:space=\"preserve\"> First note.</w:t></w:r></w:p>"
            "</w:footnote>"
            "<w:footnote w:id=\"3\"><w:p><w:r><w:footnoteRef/></w:r>"
            "<w:r><w:t>Second,</w:t></w:r></w:p>"
            "<w:p><w:r><w:t>in</w:t><w:br/><w:t>two parts.</w:t></w:r></w:p>"
            "</w:footnote>");

        auto endnotes = notesPart_(
            "endnotes",
            "<w:endnote w:id=\"1\"><w:p><w:r><w:endnoteRef/></w:r>"
            "<w:r><w:t xml:space=\"preserve\"> An endnote.</w:t></w:r></w:p>"
            "</w:endnote>");

        QCOMPARE(
            textWithNotes_(body, footnotes, endnotes),
            u"One[1] two[2]\nThree[3]\n\n"
            u"[1] First note.\n[2] An endnote.\n[3] Second, in two parts."_s);
    }

    void aDeletedReferenceIsNotNumbered()
    {
        auto body = QByteArray(
            "<w:p><w:del><w:r><w:footnoteReference w:id=\"1\"/></w:r></w:del>"
            "<w:r><w:t>Kept</w:t></w:r>"
            "<w:r><w:footnoteReference w:id=\"2\"/></w:r></w:p>");

        auto footnotes = notesPart_(
            "footnotes",
            "<w:footnote w:id=\"1\"><w:p><w:r><w:t>Gone</w:t></w:r></w:p>"
            "</w:footnote>"
            "<w:footnote w:id=\"2\"><w:p><w:r><w:t>Stays</w:t></w:r></w:p>"
            "</w:footnote>");

        QCOMPARE(
            textWithNotes_(body, footnotes, {}),
            u"Kept[1]\n\n[1] Stays"_s);
    }

    // A reference to a note the document doesn't have keeps its marker
    void aMissingNoteKeepsItsMarker()
    {
        QCOMPARE(
            textWithNotes_(
                "<w:p><w:r><w:t>A</w:t></w:r>"
                "<w:r><w:footnoteReference w:id=\"7\"/></w:r></w:p>",
                {},
                {}),
            u"A[1]\n\n[1]"_s);
    }

    // Damaged notes give no text, as a damaged document does
    void malformedOrMisplacedNotesAreNothing()
    {
        auto body = QByteArray("<w:p><w:r><w:t>A</w:t></w:r></w:p>");

        QVERIFY(!Fernanda::Docx::textOf(
                     document_(body),
                     notesPart_("footnotes", "<w:footnote>"),
                     {})
                     .has_value());
        QVERIFY(!Fernanda::Docx::textOf(
                     document_(body),
                     notesPart_("endnotes", QByteArray{}),
                     {})
                     .has_value());
    }

    void aWordFileReads()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s,       relationships_("word/document.xml") },
                { u"word/document.xml"_s,
                 document_("<w:p><w:r><w:t>Hello</w:t></w:r></w:p>")          }
        });

        auto conversion = Fernanda::Docx::toPlainText(archive);

        QCOMPARE(conversion.failure, Fernanda::Docx::Failure::None);
        QCOMPARE(conversion.text, u"Hello"_s);
    }

    // Part names are found whatever their letter case, as the packaging rules
    // compare them
    void partNamesIgnoreLetterCase()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s,       relationships_("word/document.xml") },
                { u"Word/Document.xml"_s,
                 document_("<w:p><w:r><w:t>Hello</w:t></w:r></w:p>")          }
        });

        auto conversion = Fernanda::Docx::toPlainText(archive);

        QCOMPARE(conversion.failure, Fernanda::Docx::Failure::None);
        QCOMPARE(conversion.text, u"Hello"_s);
    }

    // The main document is wherever the relationships say: here a path from
    // the package's root, percent-encoded
    void theRelationshipsNameTheMainDocument()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s,          relationships_("/custom/main%20part.xml") },
                { u"word/document.xml"_s,
                 document_("<w:p><w:r><w:t>Wrong</w:t></w:r></w:p>")                   },
                { u"custom/main part.xml"_s,
                 document_("<w:p><w:r><w:t>Right</w:t></w:r></w:p>")                   }
        });

        QCOMPARE(Fernanda::Docx::toPlainText(archive).text, u"Right"_s);
    }

    void aFileThatIsntAZipIsNotAWordDocument()
    {
        Fixture_ f{};
        f.write(u"fake.docx"_s, "not a zip");

        auto conversion =
            Fernanda::Docx::toPlainText(f.absolute(u"fake.docx"_s));

        QCOMPARE(conversion.failure, Fernanda::Docx::Failure::NotAWordDocument);
        QVERIFY(conversion.text.isEmpty());
    }

    void aZipWithoutRelationshipsIsNotAWordDocument()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"word/document.xml"_s,
                 document_("<w:p><w:r><w:t>Hello</w:t></w:r></w:p>") }
        });

        QCOMPARE(
            Fernanda::Docx::toPlainText(archive).failure,
            Fernanda::Docx::Failure::NotAWordDocument);
    }

    void aMissingMainDocumentIsNotAWordDocument()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s, relationships_("word/document.xml") }
        });

        QCOMPARE(
            Fernanda::Docx::toPlainText(archive).failure,
            Fernanda::Docx::Failure::NotAWordDocument);
    }

    // A target climbing out of the package names no part in it
    void aTargetOutsideThePackageIsNotAWordDocument()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s,  relationships_("../document.xml") },
                { u"document.xml"_s,
                 document_("<w:p><w:r><w:t>Hello</w:t></w:r></w:p>")   }
        });

        QCOMPARE(
            Fernanda::Docx::toPlainText(archive).failure,
            Fernanda::Docx::Failure::NotAWordDocument);
    }

    // The main document's relationships name its notes, relative to its own
    // folder or from the package's root
    void aWordFileReadsItsNotes()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s, relationships_("word/document.xml") },
                { u"word/_rels/document.xml.rels"_s,
                 relationshipsOf_(
                      { { "footnotes", "footnotes.xml" },
                        { "endnotes", "/word/endnotes.xml" } }) },
                { u"word/document.xml"_s,
                 document_(
                      "<w:p><w:r><w:t>Text</w:t></w:r>"
                      "<w:r><w:footnoteReference w:id=\"1\"/></w:r>"
                      "<w:r><w:endnoteReference w:id=\"1\"/></w:r></w:p>") },
                { u"word/footnotes.xml"_s,
                 notesPart_(
                      "footnotes", "<w:footnote w:id=\"1\"><w:p><w:r><w:t>Foot</w:t></w:r>"
                      "</w:p></w:footnote>") },
                { u"word/endnotes.xml"_s,
                 notesPart_(
                      "endnotes", "<w:endnote w:id=\"1\"><w:p><w:r><w:t>End</w:t></w:r>"
                      "</w:p></w:endnote>") }
        });

        auto conversion = Fernanda::Docx::toPlainText(archive);

        QCOMPARE(conversion.failure, Fernanda::Docx::Failure::None);
        QCOMPARE(conversion.text, u"Text[1][2]\n\n[1] Foot\n[2] End"_s);
    }

    // A notes part the relationships name has to be there, as the main
    // document does
    void aNamedNotesPartThatIsMissingIsNotAWordDocument()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s,                  relationships_("word/document.xml") },
                { u"word/_rels/document.xml.rels"_s,
                 relationshipsOf_({ { "footnotes", "footnotes.xml" } })                  },
                { u"word/document.xml"_s,
                 document_("<w:p><w:r><w:t>Text</w:t></w:r></w:p>")                      }
        });

        QCOMPARE(
            Fernanda::Docx::toPlainText(archive).failure,
            Fernanda::Docx::Failure::NotAWordDocument);
    }

    // Malformed XML gives no text at all, not the text before the damage
    void aDamagedDocumentGivesNoText()
    {
        Fixture_ f{};
        auto archive = f.zip(
            {
                { u"_rels/.rels"_s,       relationships_("word/document.xml") },
                { u"word/document.xml"_s,
                 document_("<w:p><w:r><w:t>Hello</w:t></w:r></w:p><w:p>")     }
        });

        auto conversion = Fernanda::Docx::toPlainText(archive);

        QCOMPARE(conversion.failure, Fernanda::Docx::Failure::Damaged);
        QVERIFY(conversion.text.isEmpty());
    }
};

QTEST_GUILESS_MAIN(DocxTest)

#include "DocxTest.moc"
