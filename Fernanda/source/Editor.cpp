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

// Editor.cpp, Fernanda

#include "Editor.h"

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
    shortcuts();
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

void Editor::devRemoveStyle()
{
    for (auto& widget : QVector<QWidget*>{ shadow, overlay, underlay, plainTextEdit })
        widget->setStyleSheet(nullptr);
    plainTextEdit->cursorColorHex = nullptr;
    plainTextEdit->cursorUnderColorHex = nullptr;
}

void Editor::toggle(bool checked, Has has)
{
    switch (has) {
    case Has::CursorBlink:
        hasCursorBlink = checked;
        startBlinker();
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorBlink, checked);
        break;
    case Has::CursorBlock:
        hasCursorBlock = checked;
        plainTextEdit->cursorPositionChanged();
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorBlock, checked);
        break;
    case Has::CursorCenterOnScroll:
        askToggleCenterOnScroll(checked);
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorCenterOnScroll, checked);
        break;
    case Has::CursorEnsureVisible:
        hasCursorEnsureVisible = checked;
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorEnsureVisible, checked);
        break;
    case Has::CursorTypewriter:
        hasCursorTypewriter = checked;
        plainTextEdit->textChanged();
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorTypewriter, checked);
        break;
    case Has::ExtraScrolls:
        askToggleExtraScrolls(checked);
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleScrollsPrevNext, checked);
        break;
    case Has::KeyFilter:
        hasKeyFilter = checked;
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleKeyFilter, checked);
        break;
    case Has::LineHighlight:
        hasLineHighlight = checked;
        plainTextEdit->highlightCurrentLine();
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleLineHighlight, checked);
        break;
    case Has::LineNumberArea:
        askToggleLineNumberArea(checked);
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleLineNumberArea, checked);
        break;
    case Has::Scrolls:
        askToggleScrolls(checked);
        break;
    case Has::Shadow:
        hasShadow = checked;
        setStyle(askTheme());
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleEditorShadow, checked);
        break;
    case Has::Theme:
        hasTheme = checked;
        setStyle(askTheme());
        UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::ToggleEditorTheme, checked);
        break;
    }
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
    auto theme_path = Path::toStdFs(selection->data());
    auto editor_style = Style::editorStyle(theme_path, hasTheme, hasShadow);
    for (auto& widget : QVector<QWidget*>{ shadow, overlay, underlay, plainTextEdit })
        widget->setStyleSheet(editor_style.styleSheet);
    plainTextEdit->cursorColorHex = editor_style.cursorColor;
    plainTextEdit->cursorUnderColorHex = editor_style.underCursorColor;
    UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::EditorTheme, Path::toQString(theme_path));
}

void Editor::handleFont(QAction* selection, int sliderValue)
{
    if (selection == nullptr) return;
    auto path = selection->data();
    plainTextEdit->handleFont(Path::toStdFs(path), sliderValue);
    UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::EditorFont, path);
    UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::EditorFontSize, sliderValue);
}

void Editor::setTabStop(int distance)
{
    if (distance == -1)
        distance = 40;
    plainTextEdit->setTabStopDistance(distance);
    UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::TabStop, distance);
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
    UserData::saveConfig(UserData::IniGroup::Editor, UserData::IniValue::WrapMode, mode);
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
    connect(this, &Editor::askToggleCenterOnScroll, plainTextEdit, &PlainTextEdit::toggleCenterOnScroll);
    connect(this, &Editor::askToggleLineNumberArea, plainTextEdit, &PlainTextEdit::toggleLineNumberArea);
    connect(this, &Editor::askToggleScrolls, plainTextEdit, &PlainTextEdit::toggleScrolls);
    connect(this, &Editor::askToggleExtraScrolls, plainTextEdit, &PlainTextEdit::toggleExtraScrolls);
    connect(this, &Editor::startBlinker, this, [&]()
        {
            if (!hasCursorBlink) return;
            cursorBlink->start(1000);
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
    connect(plainTextEdit, &PlainTextEdit::askHasKeyFilter, this, [&]() { return hasKeyFilter; });
    connect(plainTextEdit, &PlainTextEdit::askHasCursorBlink, this, [&]() { return hasCursorBlink; });
    connect(plainTextEdit, &PlainTextEdit::askHasCursorBlock, this, [&]() { return hasCursorBlock; });
    connect(plainTextEdit, &PlainTextEdit::askHasCursorEnsureVisible, this, [&]() { return hasCursorEnsureVisible; });
    connect(plainTextEdit, &PlainTextEdit::askHasCursorTypewriter, this, [&]() { return hasCursorTypewriter; });
    connect(plainTextEdit, &PlainTextEdit::askCursorVisible, this, [&]() { return cursorVisible; });
    connect(plainTextEdit, &PlainTextEdit::cursorPositionChanged, this, [&]() { cursorPositionChanged(); });
    connect(plainTextEdit, &PlainTextEdit::selectionChanged, this, [&]() { selectionChanged(); });
    connect(plainTextEdit, &PlainTextEdit::askGoNext, this, [&]() { askGoNext(); });
    connect(plainTextEdit, &PlainTextEdit::askGoPrevious, this, [&]() { askGoPrevious(); });
    connect(plainTextEdit, &PlainTextEdit::sendBlockNumber, this, [&](int blockNumber) { sendBlockNumber(blockNumber); });
    connect(plainTextEdit, &PlainTextEdit::cursorPositionChanged, this, [&]()
        {
            if (plainTextEdit->textCursor().hasSelection() || !hasCursorBlink) return;
            cursorVisible = true;
            startBlinker();
        });
}

void Editor::shortcuts()
{
    auto cycle_fonts = new QShortcut(Qt::ALT | Qt::Key_F10, this);
    auto cycle_core_themes = new QShortcut(Qt::Key_F11, this);
    auto cycle_themes = new QShortcut(Qt::ALT | Qt::Key_F11, this);
    auto zoom_out = new QShortcut(Qt::ALT | Qt::Key_Minus, this);
    auto zoom_in = new QShortcut(Qt::ALT | Qt::Key_Equal, this);
    auto nav_previous = new QShortcut(Qt::ALT | Qt::Key_Insert, this);
    auto nav_next = new QShortcut(Qt::ALT | Qt::Key_Delete, this);
    connect(cycle_core_themes, &QShortcut::activated, this, &Editor::cycleCoreThemes);
    connect(cycle_fonts, &QShortcut::activated, this, [&]() { Style::actionCycle(askFonts()); });
    connect(cycle_themes, &QShortcut::activated, this, [&]() { Style::actionCycle(askThemes()); });
    connect(nav_previous, &QShortcut::activated, this, [&]() { scroll(PlainTextEdit::Step::Previous); });
    connect(nav_next, &QShortcut::activated, this, [&]() { scroll(PlainTextEdit::Step::Next); });
    connect(zoom_out, &QShortcut::activated, this, [&]() { askFontSliderZoom(PlainTextEdit::Zoom::Out); });
    connect(zoom_in, &QShortcut::activated, this, [&]() { askFontSliderZoom(PlainTextEdit::Zoom::In); });
    for (const auto& shortcut : { cycle_fonts, cycle_core_themes, cycle_themes, zoom_out, zoom_in, nav_previous, nav_next })
        shortcut->setAutoRepeat(false);
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

void Editor::cycleCoreThemes()
{
    auto themes = askThemes();
    auto actions = themes->actions();
    auto text = themes->checkedAction()->text();
    auto break_it = false;
    auto theme_1 = QStringLiteral("Amber");
    auto theme_2 = QStringLiteral("Green");
    for (auto& action : actions)
    {
        auto action_text = action->text();
        if (text != theme_1 && text != theme_2 && action_text == theme_1)
            break_it = true;
        else if (text == theme_1 && action_text == theme_2)
            break_it = true;
        else if (text == theme_2 && action_text == QStringLiteral("Grey"))
            break_it = true;
        if (break_it)
        {
            action->setChecked(true);
            break;
        }
    }
}

// Editor.cpp, Fernanda
