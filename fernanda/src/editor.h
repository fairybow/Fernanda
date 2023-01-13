// editor.h, Fernanda

#pragma once

#include "plaintextedit.h"
#include "style.h"

#include <QAction>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QSizePolicy>
#include <Qt>
#include <QTextOption>
#include <QTimer>

class Editor : public QWidget
{
    Q_OBJECT

public:
    Editor(QWidget* parent);

    enum class Action {
        None = 0,
        AcceptNew
    };

    bool hasTheme = true;
    bool hasShadow = true;

    const QStringList devGetCursorPositions();
    Action handleKeySwap(QString oldKey, QString newKey);
    void handleTextSwap(QString key, QString text);
    void setStyle(QAction* selection);
    void handleFont(QAction* selection, int sliderValue);

    QString toPlainText() const { return plainTextEdit->toPlainText(); }
    int cursorBlockNumber() const { return plainTextEdit->textCursor().blockNumber(); }
    int cursorPositionInBlock() const { return plainTextEdit->textCursor().positionInBlock(); }
    QString selectedText() const { return plainTextEdit->textCursor().selectedText(); }
    int selectedLineCount() const { return plainTextEdit->selectedLineCount(); }
    bool hasSelection() const { return plainTextEdit->textCursor().hasSelection(); }
    int blockCount() const { return plainTextEdit->blockCount(); }
    void scrollNavClicked(PlainTextEdit::Scroll direction) { plainTextEdit->scrollNavClicked(direction); }
    void setFocus() { plainTextEdit->setFocus(); }

public slots:
    void setTabStop(int distance);
    void setWrapMode(QString mode);
    void toggleLineHighlight(bool checked);
    void toggleKeyfilter(bool checked);
    void toggleBlockCursor(bool checked);
    void toggleCursorBlink(bool checked);
    void close(bool isFinal);

private:
    PlainTextEdit* plainTextEdit = new PlainTextEdit(this);
    QLabel* shadow = new QLabel(this);
    QLabel* overlay = new QLabel(this);
    QLabel* underlay = new QLabel(this);
    QTimer* cursorBlink = new QTimer(this);

    struct CursorPositions {
        QString key;
        int position;
        int anchor;
        bool operator==(const CursorPositions&) const = default;
        bool operator!=(const CursorPositions&) const = default;
    };

    QVector<CursorPositions> cursorPositions;
    bool hasLineHighlight = true;
    bool hasKeyfilter = true;
    bool hasCursorBlink = true;
    bool hasBlockCursor = true;
    bool cursorVisible = true;

    void connections();
    void storeCursors(QString key);
    void recallCursors(QString key);

signals:
    bool askHasProject();
    void askFontSliderZoom(PlainTextEdit::Zoom direction);
    void textChanged();
    void cursorPositionChanged();
    void selectionChanged();
    void startBlinker();
    void askToggleLineNumberArea(bool checked);
    void askToggleScrolls(bool checked);
    void askToggleExtraScrolls(bool checked);
    void askNavPrevious();
    void askNavNext();
};

// editor.h, Fernanda
