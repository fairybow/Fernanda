/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Editor.h, Fernanda

#pragma once

#include "PlainTextEdit.h"
#include "Style.h"
#include "UserData.h"

#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QShortcut>
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

    enum class Has {
        BlockCursor,
        CursorBlink,
        ExtraScrolls,
        Keyfilter,
        LineHighlight,
        LineNumberArea,
        Scrolls,
        Shadow,
        Theme
    };

    const QStringList devGetCursorPositions();
    void devRemoveStyle();
    void toggle(bool checked, Has has);
    Action handleKeySwap(QString oldKey, QString newKey);
    void handleTextSwap(QString key, QString text);
    void setStyle(QAction* selection);
    void handleFont(QAction* selection, int sliderValue);

    int blockCount() const { return plainTextEdit->blockCount(); }
    int cursorBlockNumber() const { return plainTextEdit->textCursor().blockNumber(); }
    int cursorPositionInBlock() const { return plainTextEdit->textCursor().positionInBlock(); }
    bool hasSelection() const { return plainTextEdit->textCursor().hasSelection(); }
    void scrollNavClicked(PlainTextEdit::Scroll direction) { plainTextEdit->scrollNavClicked(direction); }
    int selectedLineCount() const { return plainTextEdit->selectedLineCount(); }
    QString selectedText() const { return plainTextEdit->textCursor().selectedText(); }
    void setFocus() { plainTextEdit->setFocus(); }
    QString toPlainText() const { return plainTextEdit->toPlainText(); }

public slots:
    void setTabStop(int distance);
    void setWrapMode(QString mode);
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
    bool hasTheme = true;
    bool hasShadow = true;
    bool hasLineHighlight = true;
    bool hasKeyfilter = true;
    bool hasCursorBlink = true;
    bool hasBlockCursor = true;
    bool cursorVisible = true;

    void connections();
    void shortcuts();
    void storeCursors(QString key);
    void recallCursors(QString key);
    void cycleCoreThemes();

signals:
    QActionGroup* askFonts();
    void askFontSliderZoom(PlainTextEdit::Zoom direction);
    void askGoNext();
    void askGoPrevious();
    bool askHasProject();
    QAction* askTheme();
    QActionGroup* askThemes();
    void askToggleExtraScrolls(bool checked);
    void askToggleLineNumberArea(bool checked);
    void askToggleScrolls(bool checked);
    void cursorPositionChanged();
    void selectionChanged();
    void startBlinker();
    void textChanged();
};

// Editor.h, Fernanda
