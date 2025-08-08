#pragma once

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <Qt>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolButton>
#include <QtTypes>
#include <QWidget>

#include "Coco/Bool.h"
#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "Debouncer.h"

class WordCounter : public QWidget
{
    Q_OBJECT

public:
    WordCounter(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        initialize_();
    }

    ~WordCounter() { COCO_TRACER; }

    QPlainTextEdit* textEdit() const noexcept { return textEdit_; }

    void setTextEdit(QPlainTextEdit* textEdit)
    {
        if (textEdit_ == textEdit) return;

        if (textEdit_) textEdit_->disconnect(this);
        textEdit_ = textEdit;

        if (textEdit_)
        {
            connect(textEdit_, &QPlainTextEdit::textChanged, this, [&] { countDebouncer_->start(); });
            connect(textEdit_, &QPlainTextEdit::cursorPositionChanged, this, [&] { updatePos_(); });
            //connect(textEdit_, &QPlainTextEdit::selectionChanged, this, &Meter::onSelectionChanged);
        }

        // Maybe or maybe not force. I cannot decide. If we don't, new, huge
        // documents will display null and they may take ages to open
        // (relatively)
        updateCounts_(/*Force_::Yes*/);
        updatePos_();
    }

    bool hasLineCount() const noexcept { return hasLineCount_; }
    void setHasLineCount(bool has) { if (hasLineCount_ == has) return; hasLineCount_ = has; updateCounts_(Force_::Yes); }

    bool hasWordCount() const noexcept { return hasWordCount_; }
    void setHasWordCount(bool has) { if (hasWordCount_ == has) return; hasWordCount_ = has; updateCounts_(Force_::Yes); }

    bool hasCharCount() const noexcept { return hasCharCount_; }
    void setHasCharCount(bool has) { if (hasCharCount_ == has) return; hasCharCount_ = has; updateCounts_(Force_::Yes); }

    bool hasLinePosition() const noexcept { return hasLinePos_; }
    void setHasLinePosition(bool has) { if (hasLinePos_ == has) return; hasLinePos_ = has; updatePos_(); }

    bool hasColumnPosition() const noexcept { return hasColPos_; }
    void setHasColumnPosition(bool has) { if (hasColPos_ == has) return; hasColPos_ = has; updatePos_(); }

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
    Debouncer* countDebouncer_ = new Debouncer(0, this, [&] { updateCounts_(); });
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

    void initialize_()
    {
        refresh_->setVisible(false);
        refresh_->setText("Refresh");

        opacifyWidget(countsDisplay_, 0.8);
        opacifyWidget(separatorDisplay_, 0.3);
        opacifyWidget(posDisplay_, 0.8);

        auto layout = Coco::Layout::make<QHBoxLayout*>(MARGIN_, 3, this);
        layout->addWidget(countsDisplay_);
        layout->addWidget(separatorDisplay_);
        layout->addWidget(posDisplay_);
        layout->addWidget(refresh_);

        connect(refresh_, &QToolButton::clicked, this, [&] { updateCounts_(Force_::Yes); });
    }

    void updateCounts_(Force_ force = Force_::No)
    {
        if (!textEdit_)
        {
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
        if (autoCount_ || force)
        {
            auto any = hasAnyCount_();
            if (any) countsDisplay_->setText(counts_());
            setDisplayVisible_(countsDisplay_, any);
        }
    }

    QString counts_()
    {
        QStringList elements{};
        auto text = textEdit_->toPlainText();

        if (hasLineCount_)
        {
            // Note: There's always at least 1 line
            auto document = textEdit_->document();
            auto lines = document ? document->blockCount() : 0;
            elements << QString::number(lines) + " " + ((lines != 1) ? LINES_LABEL_P_ : LINES_LABEL_);
        }

        if (hasWordCount_)
        {
            auto words = wordCount_(text);
            elements << QString::number(words) + " " + ((words != 1) ? WORDS_LABEL_P_ : WORDS_LABEL_);
        }

        if (hasCharCount_)
        {
            auto chars = text.size();
            elements << QString::number(chars) + " " + ((chars != 1) ? CHARS_LABEL_P_ : CHARS_LABEL_);
        }

        return elements.join(DELIMITER_);
    }

    void adjustCountStyle_(qsizetype charCount)
    {
        if (charCount < 5000)
        {
            // Instant updates for short content
            refresh_->setVisible(false);
            countDebouncer_->setInterval(0);
            autoCount_ = true;
        }
        else if (charCount < 10000)
        {
            refresh_->setVisible(false);
            countDebouncer_->setInterval(150);
            autoCount_ = true;
        }
        else
        {
            // Manual refresh for very large content
            refresh_->setFixedHeight(height() - (MARGIN_ * 2));
            refresh_->setVisible(true);
            countDebouncer_->setInterval(0);
            autoCount_ = false;
        }
    }

    void updatePos_()
    {
        if (!textEdit_)
        {
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
            elements << QString(LINE_POS_LABEL_) + " " + QString::number(cursor.blockNumber() + 1);

        if (hasColPos_)
            elements << QString(COL_POS_LABEL_) + " " + QString::number(cursor.positionInBlock() + 1);

        return elements.join(DELIMITER_);
    }

    bool hasAnyCount_() const noexcept { return hasLineCount_ || hasWordCount_ || hasCharCount_; }
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
        separatorDisplay_->setVisible(countsDisplay_->isVisible() && posDisplay_->isVisible());
    }

    // Move to Coco?
    inline QGraphicsOpacityEffect* opacifyWidget(QWidget* widget, double opacity = 0.5)
    {
        auto effect = new QGraphicsOpacityEffect(widget);
        effect->setOpacity(opacity);
        widget->setGraphicsEffect(effect);
        return effect;
    }
};
