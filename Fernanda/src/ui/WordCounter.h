/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextCursor>
#include <QTextDocument>
#include <QWidget>

#include <Coco/Bool.h>
#include <Coco/Fx.h>

#include "core/Debug.h"
#include "core/Time.h"
#include "core/Tr.h"

namespace Fernanda {

// Status bar widget displaying document line, word, and char counts with cursor
// position. Base counts are cached and updated on text changes (debounced, with
// manual refresh fallback for large documents), while selection counts are
// computed on demand from the cache without retriggering a full recount
//
// TODO: Padding of some kind to prevent bouncing around between labels /
// separator / right edge of status bar (bouncing around inside labels
// counts/pos is probably fine)
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
            connect(textEdit_, &QPlainTextEdit::textChanged, this, [this] {
                countDebouncer_->start();
            });

            connect(
                textEdit_,
                &QPlainTextEdit::cursorPositionChanged,
                this,
                [this] { updatePos_(); });

            connect(textEdit_, &QPlainTextEdit::selectionChanged, this, [this] {
                updateSelection_();
            });

            connect(textEdit_, &QObject::destroyed, this, [this] {
                textEdit_ = nullptr;
                cachedBaseCounts_.clear();
                updateCountsDisplay_();
                updatePos_();
            });
        }

        updateCounts_(Force_::Yes);
        updatePos_();
    }

    bool active() const noexcept { return active_; }

    void setActive(bool active)
    {
        if (active_ == active) return;
        active_ = active;

        if (active_) {
            updateCounts_(Force_::Yes);
            updatePos_();
        } else {
            countDebouncer_->stop();
            cachedBaseCounts_.clear();
            updateCountsDisplay_();
            updatePos_();
        }
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

protected:
    virtual void mousePressEvent(QMouseEvent* event) override
    {
        // Maybe: && event->button() == Qt::LeftButton
        if (!autoCount_) {
            updateCounts_(Force_::Yes);
            dimCountsDisplay_(false);
            staleDimTimer_->start(); // Delay re-dimming

            return;
        }

        QWidget::mousePressEvent(event);
    }

private:
    static constexpr auto MARGIN_ = 0.5;
    static constexpr auto DELIMITER_ = ", ";
    static constexpr auto SEPARATOR_ = " / ";

    // Threshold at which we switch from auto-count to manual refresh. With
    // in-place counting (no allocations), we can handle much larger documents
    // in real time. ~500k chars is roughly a full novel
    static constexpr qsizetype AUTO_COUNT_THRESHOLD_ = 500'000;
    static constexpr int DEBOUNCE_MS_ = 150;
    static constexpr int STALE_DIM_MS_ = 2000;
    static constexpr double FRESH_OPACITY_ = 0.8;
    static constexpr double STALE_OPACITY_ = 0.35;

    QPointer<QPlainTextEdit> textEdit_{};
    Time::Debouncer* countDebouncer_ =
        Time::newDebouncer(this, [this] { updateCounts_(); }, DEBOUNCE_MS_);
    QLabel* countsDisplay_ = new QLabel(this);
    QLabel* separatorDisplay_ = new QLabel(SEPARATOR_, this);
    QLabel* posDisplay_ = new QLabel(this);
    Time::Delayer* staleDimTimer_ = Time::newDelayer(
        this,
        [this] { dimCountsDisplay_(true); },
        STALE_DIM_MS_);

    COCO_BOOL(Force_)

    bool active_ = true;

    bool autoCount_ = true;
    bool hasLineCount_ = true;
    bool hasWordCount_ = true;
    bool hasCharCount_ = false;
    bool hasLinePos_ = true;
    bool hasColPos_ = true;

    // All counts show when this is on even when all counts are false
    bool hasSelectionCounts_ = true;
    bool hasSelectionReplacement_ = true;

    // Cached base counts string, updated on text changes. Selection changes
    // read from this cache rather than recomputing the (potentially expensive)
    // full document counts
    QString cachedBaseCounts_{};

    void setup_()
    {
        Coco::Fx::opacify(countsDisplay_, 0.8);
        Coco::Fx::opacify(separatorDisplay_, 0.3);
        Coco::Fx::opacify(posDisplay_, 0.8);

        // Ensure correct initial visibility. If we don't do this, on startup,
        // if the settings values match the defaults, then every setter call
        // from the Module (responding to settings) will be guarded and return
        // early (because each already matches the default), and so the
        // separator/labels will never be told to hide!
        setDisplayVisible_(countsDisplay_, false);
        setDisplayVisible_(posDisplay_, false);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(MARGIN_, MARGIN_, MARGIN_, MARGIN_);
        layout->setSpacing(3);

        layout->addWidget(countsDisplay_);
        layout->addWidget(separatorDisplay_);
        layout->addWidget(posDisplay_);
    }

    // --- Base count computation ---

    void updateCounts_(Force_ force = Force_::No)
    {
        if (!active_ || !textEdit_) {
            cachedBaseCounts_.clear();
            adjustCountStyle_(0);
            updateCountsDisplay_();

            return;
        }

        auto document = textEdit_->document();
        auto char_count = document ? document->characterCount() : 0;
        adjustCountStyle_(char_count);

        if ((autoCount_ || force) && hasAnyCount_())
            cachedBaseCounts_ = buildCounts_(CountSource_::Document);

        updateCountsDisplay_();
    }

    void adjustCountStyle_(qsizetype charCount)
    {
        if (charCount < AUTO_COUNT_THRESHOLD_) {
            staleDimTimer_->stop();
            dimCountsDisplay_(false);
            setCursor(Qt::ArrowCursor);
            autoCount_ = true;
        } else if (autoCount_) {
            setCursor(Qt::PointingHandCursor);
            autoCount_ = false;
            dimCountsDisplay_(true); // Dim immediately
        }
    }

    // --- Selection handling ---

    void updateSelection_()
    {
        // Selection changes never recompute base counts; they just rebuild the
        // display using the cache plus fresh (cheap) selection counts
        updateCountsDisplay_();

        if (!autoCount_) {
            if (hasSelectionCounts_ && hasActiveSelection_()) {
                staleDimTimer_->stop();
                dimCountsDisplay_(false);
            } else {
                dimCountsDisplay_(true);
            }
        }
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
    QString buildCounts_(CountSource_ source, Force_ force = Force_::No)
    {
        if (!textEdit_) return {};

        QStringList elements{};
        QString text{};
        auto line = hasLineCount_ || force;
        auto word = hasWordCount_ || force;
        auto char_ = hasCharCount_ || force;
        auto need_text = word || char_;

        if (source == CountSource_::Document) {
            if (line) {
                auto document = textEdit_->document();
                auto lines = document ? document->blockCount() : 0;
                elements << Tr::wordCounterLines(lines);
            }

            if (need_text) text = textEdit_->toPlainText();

        } else {
            auto selected = textEdit_->textCursor().selectedText();

            if (line) {
                auto lines = selectionLineCount_(selected);
                elements << Tr::wordCounterLines(lines);
            }

            if (need_text) text = selected;
        }

        if (word) {
            auto words = wordCount_(text);
            elements << Tr::wordCounterWords(words);
        }

        if (char_) {
            auto chars = text.size();
            elements << Tr::wordCounterChars(chars);
        }

        return elements.join(DELIMITER_);
    }

    // --- Display composition ---

    void updateCountsDisplay_()
    {
        if (!active_ || !textEdit_) {
            setDisplayVisible_(countsDisplay_, false);
            return;
        }

        auto any = hasAnyCount_();
        auto selection_active = hasSelectionCounts_ && hasActiveSelection_();

        if (!any && !selection_active) {
            setDisplayVisible_(countsDisplay_, false);
            return;
        }

        QString display{};

        if (any) {
            display = cachedBaseCounts_;

            if (selection_active) {
                auto sel = buildCounts_(CountSource_::Selection);

                if (hasSelectionReplacement_)
                    display = sel;
                else
                    display = cachedBaseCounts_ + QStringLiteral(" (") + sel
                              + QStringLiteral(")");
            }
        } else {
            // All counts off (!any) but selection on (since we didn't hit (!any
            // && !selection_active) above): show all three for selection
            display = buildCounts_(CountSource_::Selection, Force_::Yes);
        }

        countsDisplay_->setText(display);
        setDisplayVisible_(countsDisplay_, true);
    }

    // --- Position ---

    void updatePos_()
    {
        if (!active_ || !textEdit_) {
            setDisplayVisible_(posDisplay_, false);
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
            elements << Tr::wordCounterLinePos() + QStringLiteral(" ")
                            + QString::number(cursor.blockNumber() + 1);

        if (hasColPos_)
            elements << Tr::wordCounterColPos() + QStringLiteral(" ")
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

        if (!show) display->clear();
    }

    void dimCountsDisplay_(bool dim)
    {
        auto opacity = dim ? STALE_OPACITY_ : FRESH_OPACITY_;
        Coco::Fx::opacify(countsDisplay_, opacity);
        Coco::Fx::opacify(separatorDisplay_, dim ? STALE_OPACITY_ : 0.3);
        Coco::Fx::opacify(posDisplay_, opacity);
    }
};

} // namespace Fernanda
