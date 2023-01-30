/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// plaintextedit.cpp, Fernanda

#include "plaintextedit.h"

PlainTextEdit::PlainTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    viewport()->setCursor(Qt::ArrowCursor);
    lineNumberArea = new LineNumberArea(this);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    addScrollBarWidget(scrollUp, Qt::AlignTop);
    addScrollBarWidget(scrollPrevious, Qt::AlignTop);
    addScrollBarWidget(scrollNext, Qt::AlignBottom);
    addScrollBarWidget(scrollDown, Qt::AlignBottom);
    for (const auto& button : { scrollUp, scrollDown })
    {
        button->setAutoRepeat(true);
        button->setAutoRepeatDelay(500);
    }
    scrollUp->setText(Icon::draw(Icon::Name::ArrowUp));
    scrollPrevious->setText(Icon::draw(Icon::Name::ArrowPrevious));
    scrollNext->setText(Icon::draw(Icon::Name::ArrowNext));
    scrollDown->setText(Icon::draw(Icon::Name::ArrowDown));
    for (const auto& button : { scrollUp, scrollPrevious, scrollNext, scrollDown })
        button->setMinimumHeight(30);
    setObjectName("editor");
    lineNumberArea->setObjectName("lineNumberArea");
    horizontalScrollBar()->setObjectName("hScrollBar");
    verticalScrollBar()->setObjectName("vScrollBar");
    scrollUp->setObjectName("scrollUp");
    scrollPrevious->setObjectName("scrollPrevious");
    scrollNext->setObjectName("scrollNext");
    scrollDown->setObjectName("scrollDown");
    connections();
    scrollButtonEnabledHandler();
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void PlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea);
    auto block = firstVisibleBlock();
    auto block_number = block.blockNumber();
    auto top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    auto bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            auto number = QString::number(block_number + 1);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++block_number;
    }
}

int PlainTextEdit::lineNumberAreaWidth()
{
    if (lineNumberArea->isVisible())
    {
        auto digits = 1;
        auto max = qMax(1, blockCount());
        while (max >= 10)
        {
            max /= 10;
            ++digits;
        }
        auto space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
        return space;
    }
    return 0;
}

int PlainTextEdit::selectedLineCount()
{
    auto cursor = textCursor();
    if (!cursor.hasSelection()) return 1;
    return cursor.selectedText().count(Text::regex(Text::Regex::ParagraphSeparator)) + 1;
}

void PlainTextEdit::scrollNavClicked(Scroll direction)
{
    if (!askHasProject()) return;
    auto early_return = false;
    switch (direction) {
    case Scroll::Next:
        if (!isMaximumScroll())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
            early_return = true;
        }
        break;
    case Scroll::Previous:
        if (!isMinimumScroll())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
            early_return = true;
        }
        break;
    }
    if (early_return) return;
    (direction == Scroll::Next) ? askGoNext() : askGoPrevious();
}

void PlainTextEdit::handleFont(StdFsPath fontPath, int sliderValue)
{
    QFont q_font;
    auto family = Path::toQString(fontPath.stem());
    if (!QFontDatabase::hasFamily(family))
    {
        auto id = QFontDatabase::addApplicationFont(Path::toQString(fontPath));
        q_font = QFontDatabase::applicationFontFamilies(id).at(0);
    }
    else
        q_font = QFontDatabase::font(family, nullptr, 0);
    q_font.setStyleStrategy(QFont::PreferAntialias);
    q_font.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
    q_font.setPointSize(sliderValue);
    setFont(q_font);
    lineNumberArea->setFont(q_font);
}

void PlainTextEdit::highlightCurrentLine()
{
    QVector<QTextEdit::ExtraSelection> extra_selections;
    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(highlight());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extra_selections.append(selection);
    }
    setExtraSelections(extra_selections);
}

void PlainTextEdit::toggleLineNumberArea(bool checked)
{
    lineNumberArea->setVisible(checked);
    updateLineNumberAreaWidth(0);
}

void PlainTextEdit::toggleScrolls(bool checked)
{
    scrollUp->setVisible(checked);
    scrollDown->setVisible(checked);
}

void PlainTextEdit::toggleExtraScrolls(bool checked)
{
    scrollPrevious->setVisible(checked);
    scrollNext->setVisible(checked);
}

void PlainTextEdit::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    auto contents = contentsRect();
    lineNumberArea->setGeometry(QRect(contents.left(), contents.top(), lineNumberAreaWidth(), contents.height()));
}

void PlainTextEdit::paintEvent(QPaintEvent* event)
{
    QPlainTextEdit::paintEvent(event);
    QPainter painter(viewport());
    auto current_char = currentChar();
    auto rect = reshapeCursor(current_char);
    painter.fillRect(rect, recolorCursor());
    if (!current_char.isNull() && askHasBlockCursor())
    {
        painter.setPen(recolorCursor(true));
        painter.drawText(rect, current_char);
    }
}

void PlainTextEdit::wheelEvent(QWheelEvent* event)
{
    (event->modifiers() == Qt::ControlModifier)
        ? (event->angleDelta().y() > 0)
            ? askFontSliderZoom(Zoom::In)
            : askFontSliderZoom(Zoom::Out)
        : QPlainTextEdit::wheelEvent(event);
    event->accept();
}

void PlainTextEdit::keyPressEvent(QKeyEvent* event)
{
    QTextCursor cursor = textCursor();
    if (shortcutFilter(event))
    {
        event->ignore();
        return;
    }
    if (!askHasKeyfilter())
    {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }
    auto chars = proximalChars();
    cursor.beginEditBlock();
    keyPresses(keyfilter->filter(event, chars));
    cursor.endEditBlock();
    if (cursor.atEnd() && !isMaximumScroll())
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
}

void PlainTextEdit::contextMenuEvent(QContextMenuEvent* event)
{
    if (askOverlayVisible()) return;
    QPlainTextEdit::contextMenuEvent(event);
}

void PlainTextEdit::keyPresses(QVector<QKeyEvent*> events)
{
    for (auto& event : events)
        QPlainTextEdit::keyPressEvent(event);
}

const QChar PlainTextEdit::currentChar()
{
    auto text = textCursor().block().text();
    auto current_position = textCursor().positionInBlock();
    if (current_position < text.size())
        return text.at(current_position);
    return QChar();
}

const Keyfilter::ProximalChars PlainTextEdit::proximalChars()
{
    auto text = textCursor().block().text();
    auto current_position = textCursor().positionInBlock();
    auto result = Keyfilter::ProximalChars{};
    if (current_position < text.size())
        result.current = text.at(current_position);
    if (current_position > 0)
        result.previous = text.at(static_cast<qsizetype>(current_position) - 1);
    if (current_position > 1)
        result.beforeLast = text.at(static_cast<qsizetype>(current_position) - 2);
    return result;
}

bool PlainTextEdit::shortcutFilter(QKeyEvent* event)
{
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
    {
        if (event->key() == Qt::Key_C)
        {
            quoteWrap(event);
            return true;
        }
    }
    return false;
}

void PlainTextEdit::quoteWrap(QKeyEvent* event)
{
    QKeyEvent backspace{ QKeyEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier };
    QKeyEvent quote{ QKeyEvent::KeyPress, Qt::Key_QuoteDbl, Qt::NoModifier, QString('"') };
    QKeyEvent right{ QKeyEvent::KeyPress, Qt::Key_Right, Qt::NoModifier };
    auto cursor = textCursor();
    auto text = cursor.block().text();
    cursor.beginEditBlock();
    if (cursor.hasSelection())
    {
        auto selection = cursor.selectedText();
        auto start_position = cursor.selectionStart();
        auto end_position = cursor.selectionEnd();
        cursor.setPosition(start_position);
        setTextCursor(cursor);
        QPlainTextEdit::keyPressEvent(&quote);
        cursor.setPosition(end_position);
        setTextCursor(cursor);
        if (selection.endsWith(" "))
            keyPresses({ &right, &backspace, &quote });
        else if (selection.end()->isNull())
            keyPresses({ &right, &quote });
        else
            QPlainTextEdit::keyPressEvent(&quote);
    }
    else
    {
        cursor.movePosition(QTextCursor::StartOfBlock);
        setTextCursor(cursor);
        QPlainTextEdit::keyPressEvent(&quote);
        cursor.movePosition(QTextCursor::EndOfBlock);
        setTextCursor(cursor);
        if (text.endsWith(" "))
            keyPresses({ &backspace, &quote });
        else
            QPlainTextEdit::keyPressEvent(&quote);
    }
    cursor.endEditBlock();
}

void PlainTextEdit::connections()
{
    connect(this, &PlainTextEdit::blockCountChanged, this, &PlainTextEdit::updateLineNumberAreaWidth);
    connect(this, &PlainTextEdit::updateRequest, this, &PlainTextEdit::updateLineNumberArea);
    connect(this, &PlainTextEdit::cursorPositionChanged, this, &PlainTextEdit::highlightCurrentLine);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &PlainTextEdit::scrollButtonEnabledHandler);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &PlainTextEdit::scrollButtonEnabledHandler);
    connect(scrollNext, &QPushButton::clicked, this, [&]() { scrollNavClicked(Scroll::Next); });
    connect(scrollPrevious, &QPushButton::clicked, this, [&]() { scrollNavClicked(Scroll::Previous); });
    connect(scrollUp, &QPushButton::clicked, this, [&]()
        {
            for (auto i = 4; i > 0; --i)
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        });
    connect(scrollDown, &QPushButton::clicked, this, [&]()
        {
            for (auto i = 4; i > 0; --i)
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        });
}

const QRect PlainTextEdit::reshapeCursor(QChar currentChar)
{
    if (askHasBlockCursor())
    {
        QFontMetrics metrics(font());
        (currentChar.isNull())
            ? setCursorWidth(metrics.averageCharWidth())
            : setCursorWidth(metrics.horizontalAdvance(currentChar));
    }
    else
        setCursorWidth(2);
    auto result = cursorRect(textCursor());
    setCursorWidth(0);
    return result;
}

const QColor PlainTextEdit::recolorCursor(bool under)
{
    QColor result;
    if (!askCursorVisible() && askHasCursorBlink())
        result = QColor(0, 0, 0, 0);
    else
    {
        (under)
            ? result = QColor(cursorUnderColorHex)
            : result = QColor(cursorColorHex);
    }
    return result;
}

const QColor PlainTextEdit::highlight()
{
    QColor result;
    (askHasLineHighlight())
        ? result = QColor(255, 255, 255, 30)
        : result = QColor(0, 0, 0, 0);
    return result;
}

void PlainTextEdit::scrollButtonEnabledHandler()
{
    (isMinimumScroll()) ? scrollUp->setEnabled(false) : scrollUp->setEnabled(true);
    (isMaximumScroll()) ? scrollDown->setEnabled(false) : scrollDown->setEnabled(true);
}

void PlainTextEdit::updateLineNumberArea(const QRect& rect, int dy)
{
    (dy)
        ? lineNumberArea->scroll(0, dy)
        : lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

// plaintextedit.cpp, Fernanda
