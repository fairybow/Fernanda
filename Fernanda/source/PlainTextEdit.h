/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// PlainTextEdit.h, Fernanda

#pragma once

#include "Icon.h"
#include "KeyFilter.h"
#include "Layout.h"
#include "Path.h"
#include "Text.h"

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
    using StdFsPath = std::filesystem::path;

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
    void handleFont(StdFsPath fontPath, int sliderValue);

public slots:
    void highlightCurrentLine();
    void toggleLineNumberArea(bool checked);
    void toggleScrolls(bool checked);
    void toggleExtraScrolls(bool checked);

    void toggleCenterOnScroll(bool checked) { setCenterOnScroll(checked); }

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void contextMenuEvent(QContextMenuEvent* event) override { if (!askOverlayVisible()) QPlainTextEdit::contextMenuEvent(event); }

private:
    QWidget* lineNumberArea;
    KeyFilter* keyFilter = new KeyFilter;
    QPushButton* scrollUp = new QPushButton(this);
    QPushButton* scrollPrevious = new QPushButton(this);
    QPushButton* scrollNext = new QPushButton(this);
    QPushButton* scrollDown = new QPushButton(this);

    const QChar currentChar();
    const KeyFilter::ProximalChars proximalChars();
    bool shortcutFilter(QKeyEvent* event);
    void quoteWrap(QKeyEvent* event);
    void connections();
    const QRect reshapeCursor(QChar currentChar);
    const QColor recolorCursor(bool under = false);
    const QColor highlight();
    void keyPresses(QVector<QKeyEvent*> events);

    bool isMaximumScroll() { return (verticalScrollBar()->sliderPosition() == verticalScrollBar()->maximum()); }
    bool isMinimumScroll() { return (verticalScrollBar()->sliderPosition() == verticalScrollBar()->minimum()); }

private slots:
    void scrollButtonEnabledHandler();
    void updateLineNumberArea(const QRect& rect, int dy);

    void typewriter() { if (askHasCursorTypewriter()) centerCursor(); }
    void updateLineNumberAreaWidth(int newBlockCount) { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); }

signals:
    bool askCursorVisible();
    void askFontSliderZoom(Zoom direction);
    void askGoNext();
    void askGoPrevious();
    bool askHasCursorBlink();
    bool askHasCursorBlock();
    bool askHasCursorEnsureVisible();
    bool askHasCursorTypewriter();
    bool askHasKeyFilter();
    bool askHasLineHighlight();
    bool askHasProject();
    bool askOverlayVisible();
    void sendBlockNumber(int blockNumber);
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(PlainTextEdit* parent) : QWidget(parent), parent(parent) {}

    QSize sizeHint() const override { return QSize(parent->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent* event) override { parent->lineNumberAreaPaintEvent(event); }

private:
    PlainTextEdit* parent;
};

// PlainTextEdit.h, Fernanda
