// editor.cpp, Fernanda

#include "editor.h"

Editor::Editor(QWidget* parent)
    : QWidget(parent)
{
    cursorBlink->setTimerType(Qt::VeryCoarseTimer);
    setLayout(Layout::stackLayout({ shadow, overlay, plainTextEdit, underlay }, this));
    shadow->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    shadow->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    overlay->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    underlay->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto effect = new QGraphicsBlurEffect(this);
    effect->setBlurHints(QGraphicsBlurEffect::QualityHint);
    effect->setBlurRadius(15);
    shadow->setGraphicsEffect(effect);
    shadow->setObjectName(QStringLiteral("shadow"));
    overlay->setObjectName(QStringLiteral("overlay"));
    underlay->setObjectName(QStringLiteral("underlay"));
    connections();
}

const QStringList Editor::devGetCursorPositions()
{
    QStringList result;
    result << QStringLiteral("Current document will not be present!");
    int i = 0;
    for (auto& set : cursorPositions)
    {
        ++i;
        result << QString::number(i) + "\nKey: " + set.key + "\nPosition: " + QString::number(set.position) + "\nAnchor: " + QString::number(set.anchor);
    }
    return result;
}

Editor::Action Editor::handleKeySwap(QString oldKey, QString newKey)
{
    if (oldKey == newKey)
    {
        setFocus();
        return Action::None;
    }
    if (oldKey == nullptr)
    {
        plainTextEdit->setReadOnly(false);
        overlay->setVisible(false);
        plainTextEdit->viewport()->setCursor(Qt::IBeamCursor);
    }
    else
        storeCursors(oldKey);
    return Action::AcceptNew;
}

void Editor::handleTextSwap(QString key, QString text)
{
    plainTextEdit->setPlainText(text);
    recallCursors(key);
    setFocus();
}

void Editor::setStyle(QAction* selection)
{
    if (selection == nullptr) return;
    auto theme_path = Path::toFs(selection->data());
    auto editor_style = Style::editorStyle(theme_path, hasTheme, hasShadow);
    shadow->setStyleSheet(editor_style.styleSheet);
    overlay->setStyleSheet(editor_style.styleSheet);
    underlay->setStyleSheet(editor_style.styleSheet);
    plainTextEdit->setStyleSheet(editor_style.styleSheet);
    plainTextEdit->cursorColorHex = editor_style.cursorColor;
    plainTextEdit->cursorUnderColorHex = editor_style.underCursorColor;
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::EditorTheme, Path::toQString(theme_path));
}

void Editor::handleFont(QAction* selection, int sliderValue)
{
    if (selection == nullptr) return;
    auto path = selection->data();
    plainTextEdit->handleFont(Path::toFs(path), sliderValue);
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::Font, path);
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::FontSlider, sliderValue);
}

void Editor::setTabStop(int distance)
{
    if (distance == -1)
        distance = 40;
    plainTextEdit->setTabStopDistance(distance);
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::TabStop, distance);
}

void Editor::setWrapMode(QString mode)
{
    if (mode == "NoWrap")
        plainTextEdit->setWordWrapMode(QTextOption::NoWrap);
    else if (mode == "WordWrap")
        plainTextEdit->setWordWrapMode(QTextOption::WordWrap);
    else if (mode == "WrapAnywhere")
        plainTextEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    else if (mode == "WrapAt")
        plainTextEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    else
    {
        mode = "WrapAt";
        plainTextEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    }
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::Wrap, mode);
}

void Editor::toggleLineHighlight(bool checked)
{
    hasLineHighlight = checked;
    plainTextEdit->highlightCurrentLine();
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::T_LineHighlight, checked);
}

void Editor::toggleKeyfilter(bool checked)
{
    hasKeyfilter = checked;
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Keyfilter, checked);
}

void Editor::toggleBlockCursor(bool checked)
{
    hasBlockCursor = checked;
    plainTextEdit->cursorPositionChanged();
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Cursor, checked);
}

void Editor::toggleCursorBlink(bool checked)
{
    hasCursorBlink = checked;
    startBlinker();
    Ud::saveConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::T_CursorBlink, checked);
}

void Editor::close(bool isFinal)
{
    plainTextEdit->clear();
    plainTextEdit->setReadOnly(true);
    overlay->setVisible(true);
    plainTextEdit->viewport()->setCursor(Qt::ArrowCursor);
    if (isFinal)
        cursorPositions.clear();
}

void Editor::connections()
{
    connect(this, &Editor::askToggleLineNumberArea, plainTextEdit, &PlainTextEdit::toggleLineNumberArea);
    connect(this, &Editor::askToggleScrolls, plainTextEdit, &PlainTextEdit::toggleScrolls);
    connect(this, &Editor::askToggleExtraScrolls, plainTextEdit, &PlainTextEdit::toggleExtraScrolls);
    connect(this, &Editor::startBlinker, this, [&]()
        {
            if (!hasCursorBlink) return;
            cursorBlink->start(200);
        });
    connect(cursorBlink, &QTimer::timeout, this, [&]()
        {
            cursorVisible = !cursorVisible;
            startBlinker();
        });
    connect(plainTextEdit, &PlainTextEdit::askOverlayVisible, this, [&]() { return overlay->isVisible(); });
    connect(plainTextEdit, &PlainTextEdit::askFontSliderZoom, this, [&](PlainTextEdit::Zoom direction) { askFontSliderZoom(direction); });
    connect(plainTextEdit, &PlainTextEdit::askHasProject, this, [&]() { return askHasProject(); });
    connect(plainTextEdit, &PlainTextEdit::textChanged, this, [&]() { textChanged(); });
    connect(plainTextEdit, &PlainTextEdit::askHasLineHighlight, this, [&]() { return hasLineHighlight; });
    connect(plainTextEdit, &PlainTextEdit::askHasKeyfilter, this, [&]() { return hasKeyfilter; });
    connect(plainTextEdit, &PlainTextEdit::askHasCursorBlink, this, [&]() { return hasCursorBlink; });
    connect(plainTextEdit, &PlainTextEdit::askHasBlockCursor, this, [&]() { return hasBlockCursor; });
    connect(plainTextEdit, &PlainTextEdit::askCursorVisible, this, [&]() { return cursorVisible; });
    connect(plainTextEdit, &PlainTextEdit::cursorPositionChanged, this, [&]() { cursorPositionChanged(); });
    connect(plainTextEdit, &PlainTextEdit::selectionChanged, this, [&]() { selectionChanged(); });
    connect(plainTextEdit, &PlainTextEdit::askNavNext, this, [&]() { askNavNext(); });
    connect(plainTextEdit, &PlainTextEdit::askNavPrevious, this, [&]() { askNavPrevious(); });
    connect(plainTextEdit, &PlainTextEdit::cursorPositionChanged, this, [&]()
        {
            if (plainTextEdit->textCursor().hasSelection() || !hasCursorBlink) return;
            cursorVisible = true;
            startBlinker();
        });
}

void Editor::storeCursors(QString key)
{
    for (auto& item : cursorPositions)
        if (key == item.key)
            cursorPositions.removeAll(item);
    cursorPositions << CursorPositions{
        key,
        QTextCursor(plainTextEdit->textCursor()).position(),
        QTextCursor(plainTextEdit->textCursor()).anchor()
    };
}

void Editor::recallCursors(QString key)
{
    for (auto& item : cursorPositions)
    {
        if (key != item.key) continue;
        auto cursor(plainTextEdit->textCursor());
        auto cursor_position = item.position;
        auto anchor_position = item.anchor;
        if (cursor_position == anchor_position)
            cursor.setPosition(cursor_position);
        else
        {
            cursor.setPosition(anchor_position, QTextCursor::MoveAnchor);
            cursor.setPosition(cursor_position, QTextCursor::KeepAnchor);
        }
        plainTextEdit->setTextCursor(cursor);
        cursorPositions.removeAll(item);
        break;
    }
}

// editor.cpp, Fernanda
