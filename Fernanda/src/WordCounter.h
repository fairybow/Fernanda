/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolButton>
#include <QWidget>

#include "Coco/Bool.h"

#include "Debug.h"
#include "Timers.h"

namespace Fernanda {

// Status bar widget displaying document line, word, and char counts with cursor
// position. Base counts are cached and updated on text changes (debounced, with
// manual refresh fallback for large documents), while selection counts are
// computed on demand from the cache without retriggering a full recount
//
// TODO: Decide how to handle no document display (showing "null" is not ideal;
// ideally, we'd just hide the widget, maybe, or show zeroes)
// TODO: Replace "refresh" button with something more visually intuitive, like
// greater opacity and allowing user to click on the counts to update them
// TODO: Tr support
class WordCounter : public QWidget
{
    Q_OBJECT

public:
    explicit WordCounter(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~WordCounter() override { TRACER; }

    QPlainTextEdit* textEdit() const noexcept { return textEdit_; }

    void setTextEdit(QPlainTextEdit* textEdit)
    {
        if (textEdit_ == textEdit) return;

        if (textEdit_) textEdit_->disconnect(this);
        textEdit_ = textEdit;

        if (textEdit_) {
            connect(textEdit_, &QPlainTextEdit::textChanged, this, [&] {
                countDebouncer_->start();
            });

            connect(
                textEdit_,
                &QPlainTextEdit::cursorPositionChanged,
                this,
                [&] { updatePos_(); });

            connect(textEdit_, &QPlainTextEdit::selectionChanged, this, [&] {
                updateSelection_();
            });

            connect(textEdit_, &QObject::destroyed, this, [&] {
                textEdit_ = nullptr;
                cachedBaseCounts_.clear();
                updateCountsDisplay_();
                updatePos_();
            });
        }

        updateCounts_(Force_::Yes);
        updatePos_();
    }

    bool hasLineCount() const noexcept { return hasLineCount_; }

    void setHasLineCount(bool has)
    {
        if (hasLineCount_ == has) return;
        hasLineCount_ = has;
        updateCounts_(Force_::Yes);
    }

    bool hasWordCount() const noexcept { return hasWordCount_; }

    void setHasWordCount(bool has)
    {
        if (hasWordCount_ == has) return;
        hasWordCount_ = has;
        updateCounts_(Force_::Yes);
    }

    bool hasCharCount() const noexcept { return hasCharCount_; }

    void setHasCharCount(bool has)
    {
        if (hasCharCount_ == has) return;
        hasCharCount_ = has;
        updateCounts_(Force_::Yes);
    }

    bool hasSelectionCounts() const noexcept { return hasSelectionCounts_; }

    void setHasSelectionCounts(bool has)
    {
        if (hasSelectionCounts_ == has) return;
        hasSelectionCounts_ = has;
        updateSelection_();
    }

    bool hasSelectionReplacement() const noexcept
    {
        return hasSelectionReplacement_;
    }

    void setHasSelectionReplacement(bool has)
    {
        if (hasSelectionReplacement_ == has) return;
        hasSelectionReplacement_ = has;
        updateSelection_();
    }

    bool hasLinePosition() const noexcept { return hasLinePos_; }

    void setHasLinePosition(bool has)
    {
        if (hasLinePos_ == has) return;
        hasLinePos_ = has;
        updatePos_();
    }

    bool hasColumnPosition() const noexcept { return hasColPos_; }

    void setHasColumnPosition(bool has)
    {
        if (hasColPos_ == has) return;
        hasColPos_ = has;
        updatePos_();
    }

private:
    static constexpr auto MARGIN_ = 1;
    static constexpr auto LINES_LABEL_ = "line";
    static constexpr auto WORDS_LABEL_ = "word";
    static constexpr auto CHARS_LABEL_ = "char";
    static constexpr auto LINES_LABEL_P_ = "lines";
    static constexpr auto WORDS_LABEL_P_ = "words";
    static constexpr auto CHARS_LABEL_P_ = "chars";
    static constexpr auto LINE_POS_LABEL_ = "ln";
    static constexpr auto COL_POS_LABEL_ = "col";
    static constexpr auto DELIMITER_ = ", ";
    static constexpr auto SEPARATOR_ = " / ";
    static constexpr auto DISPLAY_NULL_LABEL_ = "null";

    // Threshold at which we switch from auto-count to manual refresh. With
    // in-place counting (no allocations), we can handle much larger documents
    // in real time. ~500k chars is roughly a full novel
    static constexpr qsizetype AUTO_COUNT_THRESHOLD_ = 500'000;
    static constexpr int DEBOUNCE_MS_ = 150;

    QPointer<QPlainTextEdit> textEdit_{};
    Timers::Debouncer* countDebouncer_ =
        new Timers::Debouncer(DEBOUNCE_MS_, this, [&] { updateCounts_(); });
    QLabel* countsDisplay_ = new QLabel(DISPLAY_NULL_LABEL_, this);
    QLabel* separatorDisplay_ = new QLabel(SEPARATOR_, this);
    QLabel* posDisplay_ = new QLabel(DISPLAY_NULL_LABEL_, this);
    QToolButton* refresh_ = new QToolButton(this);

    COCO_BOOL(Force_);

    // TODO: An "is active" mode (or guard for widget is hidden)?

    bool autoCount_ = true;

    // TODO: How to handle selections when all these are turned off? When only
    // some counts are off, it makes sense for selection to function (if true)
    // and show only the counts user has turned on here. However, if all counts
    // are turned off, should we also not show selection or force selection to
    // show all (making counts only appear when there's a selection)?
    // BUG: On startup with all these turned off, I'll see "null / null" for
    // empty document. So, we should maybe only show "null" when any count or
    // any pos is chosen (otherwise, if no count or no pos is chosen, then those
    // respective sides shouldn't be rendered at all)
    bool hasLineCount_ = true;
    bool hasWordCount_ = true;
    bool hasCharCount_ = false;
    bool hasSelectionCounts_ = true;
    bool hasSelectionReplacement_ = true;

    bool hasLinePos_ = true;
    bool hasColPos_ = true;

    // Cached base counts string, updated on text changes. Selection changes
    // read from this cache rather than recomputing the (potentially expensive)
    // full document counts
    QString cachedBaseCounts_{};

    void setup_()
    {
        refresh_->setVisible(false);
        refresh_->setText("Refresh");

        opacifyWidget_(countsDisplay_, 0.8);
        opacifyWidget_(separatorDisplay_, 0.3);
        opacifyWidget_(posDisplay_, 0.8);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(MARGIN_, MARGIN_, MARGIN_, MARGIN_);
        layout->setSpacing(3);

        layout->addWidget(countsDisplay_);
        layout->addWidget(separatorDisplay_);
        layout->addWidget(posDisplay_);
        layout->addWidget(refresh_);

        connect(refresh_, &QToolButton::clicked, this, [&] {
            updateCounts_(Force_::Yes);
        });
    }

    // --- Base count computation ---

    void updateCounts_(Force_ force = Force_::No)
    {
        auto any = hasAnyCount_();

        if (!textEdit_) {
            cachedBaseCounts_.clear();
            adjustCountStyle_(0);
            updateCountsDisplay_();

            return;
        }

        auto document = textEdit_->document();
        auto char_count = document ? document->characterCount() : 0;
        adjustCountStyle_(char_count);

        if ((autoCount_ || force) && any)
            cachedBaseCounts_ = buildCounts_(CountSource_::Document);

        updateCountsDisplay_();
    }

    void adjustCountStyle_(qsizetype charCount)
    {
        if (charCount < AUTO_COUNT_THRESHOLD_) {
            refresh_->setVisible(false);
            autoCount_ = true;
        } else {
            refresh_->setFixedHeight(height() - (MARGIN_ * 2));
            refresh_->setVisible(true);
            autoCount_ = false;
        }
    }

    // --- Selection handling ---

    void updateSelection_()
    {
        // Selection changes never recompute base counts; they just rebuild the
        // display using the cache plus fresh (cheap) selection counts
        updateCountsDisplay_();
    }

    bool hasActiveSelection_() const
    {
        if (!textEdit_) return false;
        return textEdit_->textCursor().hasSelection();
    }

    // --- Shared count building ---

    enum class CountSource_
    {
        Document,
        Selection
    };

    // Builds a counts string from either the full document or the current
    // selection. Both paths use the same in-place counter, just on different
    // text. The Document path uses blockCount() for lines (free), while the
    // Selection path counts paragraph separators in the selected text
    QString buildCounts_(CountSource_ source)
    {
        QStringList elements{};
        QString text{};
        auto need_text = hasWordCount_ || hasCharCount_;

        if (source == CountSource_::Document) {
            if (hasLineCount_) {
                auto document = textEdit_->document();
                auto lines = document ? document->blockCount() : 0;
                elements << countElement_(lines, LINES_LABEL_, LINES_LABEL_P_);
            }

            if (need_text) text = textEdit_->toPlainText();

        } else {
            auto selected = textEdit_->textCursor().selectedText();

            if (hasLineCount_) {
                auto lines = selectionLineCount_(selected);
                elements << countElement_(lines, LINES_LABEL_, LINES_LABEL_P_);
            }

            if (need_text) text = selected;
        }

        if (hasWordCount_) {
            auto words = wordCount_(text);
            elements << countElement_(words, WORDS_LABEL_, WORDS_LABEL_P_);
        }

        if (hasCharCount_) {
            auto chars = text.size();
            elements << countElement_(chars, CHARS_LABEL_, CHARS_LABEL_P_);
        }

        return elements.join(DELIMITER_);
    }

    QString
    countElement_(qsizetype count, const char* singular, const char* plural)
    {
        return QString::number(count) + " "
               + ((count != 1) ? plural : singular);
    }

    // --- Display composition ---

    void updateCountsDisplay_()
    {
        auto any = hasAnyCount_();

        if (!textEdit_ || !any) {
            countsDisplay_->setText(DISPLAY_NULL_LABEL_);
            setDisplayVisible_(countsDisplay_, any);

            return;
        }

        QString display = cachedBaseCounts_;

        if (hasSelectionCounts_ && hasActiveSelection_()) {
            auto selection_counts = buildCounts_(CountSource_::Selection);

            if (hasSelectionReplacement_)
                display = selection_counts;
            else
                display = cachedBaseCounts_ + " (" + selection_counts + ")";
        }

        countsDisplay_->setText(display);
        setDisplayVisible_(countsDisplay_, any);
    }

    // --- Position ---

    void updatePos_()
    {
        auto any = hasAnyPos_();

        if (!textEdit_) {
            posDisplay_->setText(DISPLAY_NULL_LABEL_);
            setDisplayVisible_(posDisplay_, any);

            return;
        }

        if (any) posDisplay_->setText(pos_());
        setDisplayVisible_(posDisplay_, any);
    }

    QString pos_()
    {
        QStringList elements{};
        auto cursor = textEdit_->textCursor();

        if (hasLinePos_)
            elements << QString(LINE_POS_LABEL_) + " "
                            + QString::number(cursor.blockNumber() + 1);

        if (hasColPos_)
            elements << QString(COL_POS_LABEL_) + " "
                            + QString::number(cursor.positionInBlock() + 1);

        return elements.join(DELIMITER_);
    }

    // --- Counting utilities ---

    bool hasAnyCount_() const noexcept
    {
        return hasLineCount_ || hasWordCount_ || hasCharCount_;
    }

    bool hasAnyPos_() const noexcept { return hasLinePos_ || hasColPos_; }

    // In-place word count: O(n) scan with zero allocations. Counts transitions
    // from whitespace to non-whitespace
    int wordCount_(const QString& text) const
    {
        auto count = 0;
        auto in_word = false;

        for (auto& ch : text) {
            if (ch.isSpace() || ch == QChar::ParagraphSeparator) {
                in_word = false;
            } else if (!in_word) {
                in_word = true;
                ++count;
            }
        }

        return count;
    }

    // QTextCursor::selectedText() uses U+2029 (ParagraphSeparator) instead of
    // newlines. Each separator represents a block boundary, so line count is
    // separators + 1
    int selectionLineCount_(const QString& selectedText) const
    {
        auto separators = 0;

        for (auto& ch : selectedText)
            if (ch == QChar::ParagraphSeparator) ++separators;

        return separators + 1;
    }

    // --- Display helpers ---

    void setDisplayVisible_(QLabel* display, bool show)
    {
        display->setVisible(show);
        separatorDisplay_->setVisible(
            countsDisplay_->isVisible() && posDisplay_->isVisible());
    }

    // TODO: Move to Coco?
    void opacifyWidget_(QWidget* widget, double opacity = 0.5)
    {
        auto effect = new QGraphicsOpacityEffect(widget);
        effect->setOpacity(opacity);
        widget->setGraphicsEffect(effect);
    }
};

} // namespace Fernanda

// Old file and why it was bad (for posterity):

/*
The problem was that split() did a tremendous amount of work we did't need. It
walked the entire string, found every boundary, then allocated a new QString for
every word between those boundaries, and put each one into a QStringList, so it
was managing a dynamically-growing array of heap-allocated strings. For a
50,000-word document, that's 50,000 individual QString objects being
constructed, each involving a heap allocation for its character data, all being
appended into a container that itself resizes multiple times as it grows. Then
we called .count() to get a single integer, and the entire QStringList was
immediately destroyed: 50,000 destructor calls, 50,000 heap frees.

Additionally, QRegularExpression added overhead even when static. PCRE2 (Qt's
regex backend) has to maintain match state, walk through the string with its
pattern-matching engine, and handle backtracking logic, all for what is
conceptually just "is this character whitespace?"

The above alternative just walks the string character by character and notices
when it transitions from whitespace to non-whitespace, which is a word boundary.
It counts those transitions. The entire operation is a single pass over the
string's existing memory with two local variables (an int and a bool). No
allocations, no regex engine, no container resizing, no destructor calls. The
CPU can prefetch the string data efficiently because the access pattern is
perfectly sequential. The difference is most visible during real-time typing
because the counting fires repeatedly. Each firing of the old version was
creating and destroying tens of thousands of objects, competing with Qt's text
layout engine for heap access. The new version is essentially invisible to the
allocator, it just reads memory that's already there.

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolButton>
#include <QWidget>

#include "Coco/Bool.h"

#include "Debug.h"
#include "Timers.h"

namespace Fernanda {

class WordCounter : public QWidget
{
    Q_OBJECT

public:
    explicit WordCounter(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~WordCounter() override { TRACER; }

    QPlainTextEdit* textEdit() const noexcept { return textEdit_; }

    void setTextEdit(QPlainTextEdit* textEdit)
    {
        if (textEdit_ == textEdit) return;

        if (textEdit_) textEdit_->disconnect(this);
        textEdit_ = textEdit;

        if (textEdit_) {
            connect(textEdit_, &QPlainTextEdit::textChanged, this, [&] {
                countDebouncer_->start();
            });
            connect(
                textEdit_,
                &QPlainTextEdit::cursorPositionChanged,
                this,
                [&] { updatePos_(); });
            // connect(textEdit_, &QPlainTextEdit::selectionChanged, this,
            // &Meter::onSelectionChanged);
        }

        // Maybe or maybe not force. I cannot decide. If we don't, new, huge
        // documents will display null and they may take ages to open
        // (relatively)
        updateCounts_(); // Force_::Yes maybe
        updatePos_();
    }

    bool hasLineCount() const noexcept { return hasLineCount_; }
    void setHasLineCount(bool has)
    {
        if (hasLineCount_ == has) return;
        hasLineCount_ = has;
        updateCounts_(Force_::Yes);
    }

    bool hasWordCount() const noexcept { return hasWordCount_; }
    void setHasWordCount(bool has)
    {
        if (hasWordCount_ == has) return;
        hasWordCount_ = has;
        updateCounts_(Force_::Yes);
    }

    bool hasCharCount() const noexcept { return hasCharCount_; }
    void setHasCharCount(bool has)
    {
        if (hasCharCount_ == has) return;
        hasCharCount_ = has;
        updateCounts_(Force_::Yes);
    }

    bool hasLinePosition() const noexcept { return hasLinePos_; }
    void setHasLinePosition(bool has)
    {
        if (hasLinePos_ == has) return;
        hasLinePos_ = has;
        updatePos_();
    }

    bool hasColumnPosition() const noexcept { return hasColPos_; }
    void setHasColumnPosition(bool has)
    {
        if (hasColPos_ == has) return;
        hasColPos_ = has;
        updatePos_();
    }

private:
    static constexpr auto MARGIN_ = 1;
    static constexpr auto LINES_LABEL_ = "line";
    static constexpr auto WORDS_LABEL_ = "word";
    static constexpr auto CHARS_LABEL_ = "char";
    static constexpr auto LINES_LABEL_P_ = "lines";
    static constexpr auto WORDS_LABEL_P_ = "words";
    static constexpr auto CHARS_LABEL_P_ = "chars";
    static constexpr auto LINE_POS_LABEL_ = "ln";
    static constexpr auto COL_POS_LABEL_ = "col";
    static constexpr auto DELIMITER_ = ", ";
    static constexpr auto SEPARATOR_ = " / ";
    static constexpr auto DISPLAY_NULL_LABEL_ = "null";
    static constexpr auto LEADING_WS_REGEX_ = "(\\s|\U00002029|^)+";

    QPlainTextEdit* textEdit_ = nullptr;
    Timers::Debouncer* countDebouncer_ =
        new Timers::Debouncer(0, this, [&] { updateCounts_(); });
    QLabel* countsDisplay_ = new QLabel(DISPLAY_NULL_LABEL_, this);
    // Selection display (hide when no selection)
    QLabel* separatorDisplay_ = new QLabel(SEPARATOR_, this);
    QLabel* posDisplay_ = new QLabel(DISPLAY_NULL_LABEL_, this);
    QToolButton* refresh_ = new QToolButton(this);

    COCO_BOOL(Force_);
    bool autoCount_ = true;
    bool hasLineCount_ = true;
    bool hasWordCount_ = true;
    bool hasCharCount_ = true;
    bool hasLinePos_ = true;
    bool hasColPos_ = true;

    void setup_()
    {
        refresh_->setVisible(false);
        refresh_->setText("Refresh");

        opacifyWidget(countsDisplay_, 0.8);
        opacifyWidget(separatorDisplay_, 0.3);
        opacifyWidget(posDisplay_, 0.8);

        // auto layout = Coco::Layout::make<QHBoxLayout*>(MARGIN_, 3, this);
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(MARGIN_, MARGIN_, MARGIN_, MARGIN_);
        layout->setSpacing(3);

        layout->addWidget(countsDisplay_);
        layout->addWidget(separatorDisplay_);
        layout->addWidget(posDisplay_);
        layout->addWidget(refresh_);

        connect(refresh_, &QToolButton::clicked, this, [&] {
            updateCounts_(Force_::Yes);
        });
    }

    void updateCounts_(Force_ force = Force_::No)
    {
        if (!textEdit_) {
            adjustCountStyle_(0);
            countsDisplay_->setText(DISPLAY_NULL_LABEL_);
            setDisplayVisible_(countsDisplay_, hasAnyCount_());
            return;
        }

        auto document = textEdit_->document();

        // Lightweight check to adjust strategy. Via Qt: Note: As a
        // QTextDocument always contains at least one QChar::ParagraphSeparator,
        // this method will return at least 1. (QPlainTextEdit should always
        // have a document, but let's be safe)
        auto char_count = document ? document->characterCount() : 0;
        adjustCountStyle_(char_count);

        // Perform expensive counting only if necessary
        if (autoCount_ || force) {
            auto any = hasAnyCount_();
            if (any) countsDisplay_->setText(counts_());
            setDisplayVisible_(countsDisplay_, any);
        }
    }

    QString counts_()
    {
        QStringList elements{};
        auto text = textEdit_->toPlainText();

        if (hasLineCount_) {
            // Note: There's always at least 1 line
            auto document = textEdit_->document();
            auto lines = document ? document->blockCount() : 0;
            elements << QString::number(lines) + " "
                            + ((lines != 1) ? LINES_LABEL_P_ : LINES_LABEL_);
        }

        if (hasWordCount_) {
            auto words = wordCount_(text);
            elements << QString::number(words) + " "
                            + ((words != 1) ? WORDS_LABEL_P_ : WORDS_LABEL_);
        }

        if (hasCharCount_) {
            auto chars = text.size();
            elements << QString::number(chars) + " "
                            + ((chars != 1) ? CHARS_LABEL_P_ : CHARS_LABEL_);
        }

        return elements.join(DELIMITER_);
    }

    void adjustCountStyle_(qsizetype charCount)
    {
        if (charCount < 5000) {
            // Instant updates for short content
            refresh_->setVisible(false);
            countDebouncer_->setInterval(0);
            autoCount_ = true;
        } else if (charCount < 10000) {
            refresh_->setVisible(false);
            countDebouncer_->setInterval(150);
            autoCount_ = true;
        } else {
            // Manual refresh for very large content
            refresh_->setFixedHeight(height() - (MARGIN_ * 2));
            refresh_->setVisible(true);
            countDebouncer_->setInterval(0);
            autoCount_ = false;
        }
    }

    void updatePos_()
    {
        if (!textEdit_) {
            posDisplay_->setText(DISPLAY_NULL_LABEL_);
            setDisplayVisible_(posDisplay_, hasAnyPos_());
            return;
        }

        auto any = hasAnyPos_();
        if (any) posDisplay_->setText(pos_());
        setDisplayVisible_(posDisplay_, any);
    }

    QString pos_()
    {
        QStringList elements{};
        auto cursor = textEdit_->textCursor();

        if (hasLinePos_)
            elements << QString(LINE_POS_LABEL_) + " "
                            + QString::number(cursor.blockNumber() + 1);

        if (hasColPos_)
            elements << QString(COL_POS_LABEL_) + " "
                            + QString::number(cursor.positionInBlock() + 1);

        return elements.join(DELIMITER_);
    }

    bool hasAnyCount_() const noexcept
    {
        return hasLineCount_ || hasWordCount_ || hasCharCount_;
    }

    bool hasAnyPos_() const noexcept { return hasLinePos_ || hasColPos_; }

    int wordCount_(const QString& text) const
    {
        static QRegularExpression regex(LEADING_WS_REGEX_);
        auto words = text.split(regex, Qt::SkipEmptyParts);
        return words.count();
    }

    void setDisplayVisible_(QLabel* display, bool show)
    {
        display->setVisible(show);
        separatorDisplay_->setVisible(
            countsDisplay_->isVisible() && posDisplay_->isVisible());
    }

    // TODO: Move to Coco?
    QGraphicsOpacityEffect* opacifyWidget(QWidget* widget, double opacity = 0.5)
    {
        auto effect = new QGraphicsOpacityEffect(widget);
        effect->setOpacity(opacity);
        widget->setGraphicsEffect(effect);
        return effect;
    }
};

} // namespace Fernanda*/
