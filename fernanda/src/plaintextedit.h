// plaintextedit.h, Fernanda

#pragma once

#include "icon.h"
#include "keyfilter.h"
#include "layout.h"
#include "path.h"
#include "text.h"

#include <QAbstractSlider>
#include <QColor>
#include <QContextMenuEvent>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QLatin1Char>
#include <QObject>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSize>
#include <QTextBlock>
#include <QTextFormat>
#include <QTextEdit>
#include <QWheelEvent>

class PlainTextEdit : public QPlainTextEdit
{
    using FsPath = std::filesystem::path;

    Q_OBJECT

public:
    PlainTextEdit(QWidget* editor);

    enum class Scroll {
        Next,
        Previous
    };
    enum class Zoom {
        In,
        Out
    };

    QString cursorColorHex;
    QString cursorUnderColorHex;

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();
    int selectedLineCount();
    void scrollNavClicked(Scroll direction);
    void handleFont(FsPath fontPath, int sliderValue);

public slots:
    void highlightCurrentLine();
    void toggleLineNumberArea(bool checked);
    void toggleScrolls(bool checked);
    void toggleExtraScrolls(bool checked);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QWidget* lineNumberArea;
    Keyfilter* keyfilter = new Keyfilter;
    QPushButton* scrollUp = new QPushButton(this);
    QPushButton* scrollPrevious = new QPushButton(this);
    QPushButton* scrollNext = new QPushButton(this);
    QPushButton* scrollDown = new QPushButton(this);

    void keyPresses(QVector<QKeyEvent*> events);
    const QChar currentChar();
    const Keyfilter::ProximalChars proximalChars();
    bool shortcutFilter(QKeyEvent* event);
    void quoteWrap(QKeyEvent* event);
    void connections();
    const QRect reshapeCursor(QChar currentChar);
    const QColor recolorCursor(bool under = false);
    const QColor highlight();

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);

signals:
    bool askHasProject();
    void askFontSliderZoom(Zoom direction);
    bool askHasLineHighlight();
    bool askHasKeyfilter();
    bool askHasCursorBlink();
    bool askHasBlockCursor();
    bool askCursorVisible();
    void askNavPrevious();
    void askNavNext();
    bool askOverlayVisible();
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(PlainTextEdit* parent) : QWidget(parent), parent(parent) {}

    QSize sizeHint() const override
    {
        return QSize(parent->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        parent->lineNumberAreaPaintEvent(event);
    }

private:
    PlainTextEdit* parent;
};

// plaintextedit.h, Fernanda
