/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QFrame>
#include <QList>
#include <QMargins>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QString>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextLayout>
#include <QWidget>
#include <QtMath>
#include <QtMinMax>
#include <QtTypes>

#include <Coco/Debug.h>

#include "core/git/GitDiff.h"
#include "ui/UiConstants.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

class GitDiffView;

namespace Internal {

// The strip the line numbers are drawn in, a child of the diff view sitting
// in the left viewport margin the view reserves for it. It only forwards its
// paint event: the view does the drawing, since the block geometry it needs
// is protected on QPlainTextEdit. Defined below GitDiffView, which it has to
// see whole
class GitDiffGutter_ : public QWidget
{
public:
    explicit GitDiffGutter_(GitDiffView* parentDiffView);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    GitDiffView* diffView_;
};

} // namespace Internal

// One file's diff in the Git view: every hunk, each under its @@ line, with
// added lines tinted GIT_DIFF_ADDED_BACKGROUND and removed lines
// GIT_DIFF_REMOVED_BACKGROUND, the way GitHub Desktop shows a file's changes.
// A gutter on the left gives each line its number in the old file and in the
// new one; an added line has only the second, and a removed line only the
// first.
//
// Read-only, and selectable for copying. Lines wrap to the view's width,
// unlike GitHub Desktop's: in prose a paragraph is one line, and a diff of it
// would otherwise run far off the right edge. The font is the vault's text
// font (setFont, from the Git view), so prose reads as it does in the editor.
// Showing a diff either keeps where the view is scrolled to (Scroll::Kept),
// for the same file read again, or goes to its top; a rebuild for a palette
// change keeps it.
//
// The words that changed within a line (GitDiff::Line::changes) are tinted
// again, with GIT_DIFF_ADDED_WORD_BACKGROUND or
// GIT_DIFF_REMOVED_WORD_BACKGROUND. Git's note that a file ends without a line
// break is shown as its own line, in this view's words, under the line it's
// about, like the @@ lines.
//
// The tints are colors with an alpha, so they tint the page rather than cover
// it and serve a light page and a dark one alike. A line's is its block
// format's background, which QPlainTextEdit paints across the whole width
// itself; a word's is its character format's, painted over the line's, so
// the two alphas add up. The @@ lines and the notes are in
// GIT_DIFF_HEADER_ROLE, set as each line's text color when the document is
// built, so a palette change builds it again.
//
// The gutter's shape follows TextEditor's: numbers drawn on each block's
// first-line baseline, tabular digits, and block tops accumulated as qreal.
// Each tinted line's tint runs through the gutter too
class GitDiffView : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit GitDiffView(QWidget* parentGitView)
        : QPlainTextEdit(parentGitView)
    {
        setup_();
    }

    ~GitDiffView() override { TRACER; }

    // Where showDiff leaves the view scrolled
    enum class Scroll
    {
        ToTop,
        Kept
    };

    // Replace what's shown with this diff
    void showDiff(const GitDiff::Diff& diff, Scroll scroll)
    {
        diff_ = diff;
        build_();

        if (scroll == Scroll::ToTop) {
            verticalScrollBar()->setValue(0);
        }
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QPlainTextEdit::resizeEvent(event);
        layoutGutter_();
    }

    // The gutter's width depends on the font. The header lines' color is
    // the palette's, written into the document when it's built
    void changeEvent(QEvent* event) override
    {
        QPlainTextEdit::changeEvent(event);

        if (event->type() == QEvent::FontChange) {
            updateViewportMargins_();
        } else if (event->type() == QEvent::PaletteChange) {
            build_();
        }
    }

private:
    friend class Internal::GitDiffGutter_;

    enum class Kind_
    {
        Header,
        Note,
        Context,
        Added,
        Removed
    };

    // What a block of the document is, by its number: the document is built
    // here and never edited, so the numbers stay put
    struct BlockInfo_
    {
        Kind_ kind = Kind_::Context;
        int oldNumber = 0;
        int newNumber = 0;
    };

    Internal::GitDiffGutter_* gutter_ = nullptr;
    GitDiff::Diff diff_{};
    QList<BlockInfo_> blocks_{};

    // The widest line number in the diff, in digits
    int digits_ = 1;

    // The gutter comes first: setBackgroundRole delivers a PaletteChange at
    // once, and changeEvent's rebuild lays the gutter out
    void setup_()
    {
        gutter_ = new Internal::GitDiffGutter_(this);

        setFrameShape(QFrame::NoFrame);
        setReadOnly(true);
        setUndoRedoEnabled(false);
        setLineWrapMode(QPlainTextEdit::WidgetWidth);

        // The margins around the viewport are this widget's surface. Fill it
        // as the viewport fills itself, so the gutter's strip reads as part of
        // the page
        setBackgroundRole(viewport()->backgroundRole());
        setAutoFillBackground(true);

        connect(
            this,
            &QPlainTextEdit::updateRequest,
            this,
            &GitDiffView::onUpdateRequest_);

        updateViewportMargins_();
    }

    // Built again from diff_, scrolled where it was. The text cursor goes to
    // the start, where the cleared document left it
    void build_()
    {
        auto scrolled = verticalScrollBar()->value();
        clear();
        blocks_.clear();
        digits_ = 1;

        QTextCursor cursor(document());
        cursor.beginEditBlock();

        auto first = true;

        for (const auto& hunk : diff_.hunks) {
            appendBlock_(
                cursor,
                first,
                { Kind_::Header, 0, 0 },
                hunk.header,
                {});
            first = false;

            for (const auto& line : hunk.lines) {
                auto kind = kindOf_(line.kind);

                appendBlock_(
                    cursor,
                    false,
                    { kind, line.oldNumber, line.newNumber },
                    kind == Kind_::Note ? tr("No newline at end of file")
                                        : line.text,
                    line.changes);

                digits_ = qMax(
                    digits_,
                    digitsOf_(qMax(line.oldNumber, line.newNumber)));
            }
        }

        cursor.endEditBlock();

        moveCursor(QTextCursor::Start);
        verticalScrollBar()->setValue(scrolled);
        updateViewportMargins_();
        gutter_->update();
    }

    // The first block is the document's own, so it's formatted in place
    // rather than inserted after an empty one. The text goes in piece by
    // piece, each changed span with the word tint
    void appendBlock_(
        QTextCursor& cursor,
        bool first,
        const BlockInfo_& info,
        const QString& text,
        const QList<GitDiff::Span>& changes)
    {
        QTextBlockFormat block_format{};
        QTextCharFormat char_format{};
        QTextCharFormat word_format{};

        switch (info.kind) {
        case Kind_::Header:
        case Kind_::Note:
            char_format.setForeground(palette().color(GIT_DIFF_HEADER_ROLE));
            break;
        case Kind_::Added:
            block_format.setBackground(GIT_DIFF_ADDED_BACKGROUND);
            word_format.setBackground(GIT_DIFF_ADDED_WORD_BACKGROUND);
            break;
        case Kind_::Removed:
            block_format.setBackground(GIT_DIFF_REMOVED_BACKGROUND);
            word_format.setBackground(GIT_DIFF_REMOVED_WORD_BACKGROUND);
            break;
        case Kind_::Context:
            break;
        }

        if (first) {
            cursor.setBlockFormat(block_format);
            cursor.setBlockCharFormat(char_format);
        } else {
            cursor.insertBlock(block_format, char_format);
        }

        qsizetype at = 0;

        for (const auto& span : changes) {
            cursor.insertText(text.sliced(at, span.start - at), char_format);
            cursor.insertText(
                text.sliced(span.start, span.length),
                word_format);
            at = span.start + span.length;
        }

        cursor.insertText(text.sliced(at), char_format);
        blocks_ << info;
    }

    [[nodiscard]] static Kind_ kindOf_(GitDiff::Line::Kind kind)
    {
        switch (kind) {
        case GitDiff::Line::Kind::Added:
            return Kind_::Added;
        case GitDiff::Line::Kind::Removed:
            return Kind_::Removed;
        case GitDiff::Line::Kind::NoNewline:
            return Kind_::Note;
        case GitDiff::Line::Kind::Context:
            return Kind_::Context;
        }

        return Kind_::Context;
    }

    [[nodiscard]] static int digitsOf_(int number)
    {
        return int(QString::number(number).size());
    }

    // The view's font with tabular numerals. Built on demand rather than set
    // on the gutter, so the gutter always tracks the view's font
    [[nodiscard]] QFont numberFont_() const
    {
        auto number_font = font();
        number_font.setFeature(QFont::Tag("tnum"), 1);
        return number_font;
    }

    // The width one column of numbers takes
    [[nodiscard]] qreal columnWidth_() const
    {
        QFontMetricsF metrics(numberFont_());
        return metrics.horizontalAdvance(QString(digits_, QChar(u'9')));
    }

    // Padding, the old numbers, a gap, the new numbers, padding again, and
    // the text's padding, so a tint runs unbroken from the gutter into the
    // text
    [[nodiscard]] int gutterWidth_() const
    {
        return GIT_DIFF_GUTTER_PADDING + qCeil(2 * columnWidth_()) +
               GIT_DIFF_GUTTER_SPACING + GIT_DIFF_GUTTER_PADDING +
               GIT_DIFF_TEXT_PADDING;
    }

    // The gutter is the only margin. A block's tint fills the viewport's
    // whole width, so a margin on the right would leave an untinted strip
    void updateViewportMargins_()
    {
        QMargins margins(gutterWidth_(), 0, 0, 0);

        if (margins != viewportMargins()) {
            setViewportMargins(margins);
        }

        layoutGutter_();
    }

    void layoutGutter_()
    {
        auto viewport_rect = viewport()->geometry();

        gutter_->setGeometry(
            contentsRect().left(),
            viewport_rect.top(),
            gutterWidth_(),
            viewport_rect.height());
    }

    // The viewport scrolled by deltaY, or needs rect repainted: the gutter
    // does the same, so the numbers move and refresh with their lines
    void onUpdateRequest_(const QRect& rect, int deltaY)
    {
        if (deltaY != 0) {
            gutter_->scroll(0, deltaY);
        } else {
            gutter_->update(0, rect.y(), gutter_->width(), rect.height());
        }
    }

    void paintGutter_(QPaintEvent* event)
    {
        QPainter painter(gutter_);

        if (viewport()->autoFillBackground()) {
            painter.fillRect(
                event->rect(),
                viewport()->palette().brush(viewport()->backgroundRole()));
        }

        auto number_font = numberFont_();
        QFontMetricsF metrics(number_font);

        painter.setFont(number_font);
        painter.setPen(gutter_->palette().color(GIT_DIFF_NUMBER_ROLE));

        auto column = columnWidth_();
        qreal old_right = GIT_DIFF_GUTTER_PADDING + column;
        qreal new_right = old_right + GIT_DIFF_GUTTER_SPACING + column;

        // Walk the visible blocks top-down in viewport coordinates, as
        // QPlainTextEdit's own paint does
        auto block = firstVisibleBlock();
        auto top =
            blockBoundingGeometry(block).translated(contentOffset()).top();

        while (block.isValid() && top <= event->rect().bottom()) {
            auto height = blockBoundingRect(block).height();
            auto number = block.blockNumber();

            if (block.isVisible() && top + height >= event->rect().top() &&
                number < blocks_.size()) {
                const auto& info = blocks_[number];

                if (auto tint = block.blockFormat().background();
                    tint.style() != Qt::NoBrush) {
                    painter.fillRect(
                        QRectF(0, top, gutter_->width(), height),
                        tint);
                }

                auto first_line = block.layout()->lineAt(0);

                if (first_line.isValid()) {
                    auto baseline = top + first_line.y() + first_line.ascent();
                    drawNumber_(
                        painter,
                        metrics,
                        info.oldNumber,
                        old_right,
                        baseline);
                    drawNumber_(
                        painter,
                        metrics,
                        info.newNumber,
                        new_right,
                        baseline);
                }
            }

            top += height;
            block = block.next();
        }
    }

    // A number of 0 is a line not in that file, and draws nothing
    static void drawNumber_(
        QPainter& painter,
        const QFontMetricsF& metrics,
        int number,
        qreal right,
        qreal baseline)
    {
        if (number <= 0) {
            return;
        }

        auto text = QString::number(number);
        painter.drawText(
            QPointF(right - metrics.horizontalAdvance(text), baseline),
            text);
    }
};

namespace Internal {

inline GitDiffGutter_::GitDiffGutter_(GitDiffView* parentDiffView)
    : QWidget(parentDiffView)
    , diffView_(parentDiffView)
{
}

inline void GitDiffGutter_::paintEvent(QPaintEvent* event)
{
    diffView_->paintGutter_(event);
}

} // namespace Internal

} // namespace Fernanda::Ui
