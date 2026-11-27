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
// textOf a document part's XML directly; the file tests build .docx files with
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
        return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
               R"(<w:document xmlns:w=")" +
               wordNamespace +
               R"(" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006")"
               R"( xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing")"
               R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")"
               R"( xmlns:wps="http://schemas.microsoft.com/office/word/2010/wordprocessingShape")"
               R"( xmlns:v="urn:schemas-microsoft-com:vml">)"
               R"(<w:body>)" +
               body + R"(</w:body></w:document>)";
    }

    [[nodiscard]] static QByteArray document_(const QByteArray& body)
    {
        return document_(
            body,
            "http://schemas.openxmlformats.org/wordprocessingml/2006/main");
    }

    [[nodiscard]] static QString textOf_(const QByteArray& body)
    {
        return Fernanda::Docx::textOf(document_(body)).value_or(u"(none)"_s);
    }

    // Package relationships naming target as the main document
    [[nodiscard]] static QByteArray relationships_(const QByteArray& target)
    {
        return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
               R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
               R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target=")" +
               target + R"("/></Relationships>)";
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
                R"(<w:p><w:r><w:t xml:space="preserve">Dear </w:t></w:r>)"
                R"(<w:r><w:t>Cun</w:t></w:r><w:r><w:t>egonde</w:t></w:r></w:p>)"),
            u"Dear Cunegonde"_s);
    }

    void tabsAndBreaksAreKept()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>a</w:t><w:tab/><w:t>b</w:t><w:br/>"
                "<w:t>c</w:t><w:cr/><w:t>d</w:t>"
                R"(<w:br w:type="page"/><w:t>e</w:t></w:r></w:p>)"),
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

    // A field's instruction is skipped and its result kept, for both kinds of
    // field
    void fieldsReadAsTheirResults()
    {
        QCOMPARE(
            textOf_(
                R"(<w:p><w:r><w:t xml:space="preserve">Page </w:t></w:r>)"
                R"(<w:r><w:fldChar w:fldCharType="begin"/></w:r>)"
                R"(<w:r><w:instrText xml:space="preserve"> PAGE </w:instrText></w:r>)"
                R"(<w:r><w:fldChar w:fldCharType="separate"/></w:r>)"
                R"(<w:r><w:t>3</w:t></w:r>)"
                R"(<w:r><w:fldChar w:fldCharType="end"/></w:r></w:p>)"
                R"(<w:p><w:fldSimple w:instr=" NUMPAGES ">)"
                R"(<w:r><w:t>9</w:t></w:r></w:fldSimple></w:p>)"),
            u"Page 3\n9"_s);
    }

    void hiddenTextIsSkipped()
    {
        QCOMPARE(
            textOf_(
                "<w:p><w:r><w:t>Seen</w:t></w:r>"
                "<w:r><w:rPr><w:i/><w:vanish/></w:rPr><w:t> secret</w:t></w:r>"
                R"(<w:r><w:rPr><w:vanish w:val="0"/></w:rPr><w:t> shown</w:t></w:r></w:p>)"),
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
                R"(<w:p><w:r><w:t xml:space="preserve">Before </w:t></w:r>)"
                R"(<w:r><mc:AlternateContent><mc:Choice Requires="wps">)"
                R"(<w:drawing><wp:anchor><a:graphic><a:graphicData><wps:wsp>)"
                R"(<wps:txbx><w:txbxContent><w:p><w:r><w:t>Box</w:t></w:r></w:p>)"
                R"(</w:txbxContent></wps:txbx></wps:wsp></a:graphicData>)"
                R"(</a:graphic></wp:anchor></w:drawing></mc:Choice>)"
                R"(<mc:Fallback><w:pict><v:shape><v:textbox><w:txbxContent>)"
                R"(<w:p><w:r><w:t>Box</w:t></w:r></w:p></w:txbxContent>)"
                R"(</v:textbox></v:shape></w:pict></mc:Fallback>)"
                R"(</mc:AlternateContent></w:r>)"
                R"(<w:r><w:t>after</w:t></w:r></w:p>)"
                R"(<w:p><w:r><w:t>Next</w:t></w:r></w:p>)"),
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
            Fernanda::Docx::textOf(xml).value_or(u"(none)"_s),
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
        auto text = Fernanda::Docx::textOf(document_(QByteArray{}));

        QVERIFY(text.has_value());
        QVERIFY(text->isEmpty());
    }

    void malformedXmlIsNothing()
    {
        QVERIFY(!Fernanda::Docx::textOf(document_("<w:p><w:r><w:t>Cut"))
                     .has_value());
    }

    void aPartThatIsntADocumentIsNothing()
    {
        QVERIFY(
            !Fernanda::Docx::textOf(
                 R"(<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">)"
                 R"(<w:p><w:r><w:t>Header</w:t></w:r></w:p></w:hdr>)")
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
