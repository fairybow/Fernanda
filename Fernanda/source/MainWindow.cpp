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

// MainWindow.cpp, Fernanda

#include "MainWindow.h"

MainWindow::MainWindow(bool hasDevArgument, StdFsPath story, QWidget* parent)
    : isDev(hasDevArgument), QMainWindow(parent)
{
    UserData::setName(name());
    adjustTitle();
    addWidgets();
    connections();
    shortcuts();
    UserData::doThis();
    makeMenuBar();
    loadConfigs(story);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (isInitialized || event->spontaneous()) return;
    if (askHasStartUpBar())
        colorBar->delayedStartUp();
    isInitialized = true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    auto state = windowState();
    showNormal();
    auto quit = confirmStoryClose(true);
    if (!quit)
    {
        setWindowState(state);
        event->ignore();
        colorBar->run(ColorBar::Run::Green);
        return;
    }
    splitter->saveConfig();
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WindowState, state.toInt());
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WindowPosition, geometry());
    UserData::clear(UserData::doThis(UserData::Operation::GetActiveTemp), true);
    event->accept();
}

const QStringList MainWindow::devGetSize()
{
    QStringList result;
    auto& size = geometry();
    result << "X: " + QString::number(size.x());
    result << "Y: " + QString::number(size.y());
    result << "Width: " + QString::number(size.width());
    result << "Height: " + QString::number(size.height());
    return result;
}

bool MainWindow::confirmStoryClose(bool isQuit)
{
    if (!activeStory.has_value() || !activeStory.value().hasChanges()) return true;
    auto result = false;
    switch (Popup::confirm(isQuit)) {
    case Popup::OnClose::Close:
        result = true;
        break;
    case Popup::OnClose::Return:
        break;
    case Popup::OnClose::SaveAndClose:
        fileMenuSave();
        result = true;
        break;
    }
    return result;
}

void MainWindow::addWidgets()
{
    setCentralWidget(Layout::stackWidgets({ colorBar, splitter }));
    splitter->addWidgets({ pane, editor, preview });
    statusBar->setSizeGripEnabled(true);
    setMenuBar(menuBar);
    setStatusBar(statusBar);
    statusBar->addPermanentWidget(indicator, 0);
    statusBar->addPermanentWidget(spacer, 1);
    statusBar->addPermanentWidget(alwaysOnTop, 0);
    statusBar->addPermanentWidget(stayAwake, 0);
    statusBar->addPermanentWidget(timer, 0);
    statusBar->setMaximumHeight(22);
    setObjectName(QStringLiteral("mainWindow"));
    menuBar->setObjectName(QStringLiteral("menuBar"));
    statusBar->setObjectName(QStringLiteral("statusBar"));
    fontSlider->setObjectName(QStringLiteral("menuSlider"));
    spacer->setObjectName(QStringLiteral("spacer"));
}

void MainWindow::connections()
{
    connect(this, &MainWindow::askSetBarAlignment, colorBar, &ColorBar::setAlignment);
    connect(this, &MainWindow::askHasStartUpBar, colorBar, &ColorBar::hasStartUp);
    connect(this, &MainWindow::askEditorClose, editor, &Editor::close);
    connect(this, &MainWindow::sendSetTabStop, editor, &Editor::setTabStop);
    connect(this, &MainWindow::sendSetWrapMode, editor, &Editor::setWrapMode);
    connect(this, &MainWindow::sendItems, pane, &Pane::receiveItems);
    connect(this, &MainWindow::sendEditsList, pane, &Pane::receiveEditsList);
    connect(this, &MainWindow::askPaneAdd, pane, &Pane::add);
    connect(this, &MainWindow::askSetPreviewType, preview, &Preview::setType);
    connect(this, &MainWindow::askSetCountdown, timer, &Tool::setCountdown);
    connect(editor, &Editor::askFontSliderZoom, this, &MainWindow::handleFontSlider);
    connect(editor, &Editor::askHasProject, this, &MainWindow::replyHasProject);
    connect(editor, &Editor::sendBlockNumber, preview, &Preview::scrollToBlock);
    connect(editor, &Editor::textChanged, this, &MainWindow::sendEditedText);
    connect(editor, &Editor::textChanged, this, &MainWindow::adjustTitle);
    connect(pane, &Pane::askDomMove, this, &MainWindow::domMove);
    connect(pane, &Pane::askAddElement, this, &MainWindow::domAdd);
    connect(pane, &Pane::askRenameElement, this, &MainWindow::domRename);
    connect(pane, &Pane::askCutElement, this, &MainWindow::domCut);
    connect(pane, &Pane::askHasProject, this, &MainWindow::replyHasProject);
    connect(pane, &Pane::askSendToEditor, this, &MainWindow::handleEditorOpen);
    connect(pane, &Pane::askTitleCheck, this, &MainWindow::adjustTitle);
    connect(this, &MainWindow::startAutoTempSave, this, [&]() { autoTempSave->start(20000); });
    connect(this, &MainWindow::askToggleStartUpBar, colorBar, [&](bool checked) { colorBar->toggle(checked, ColorBar::Has::RunOnStartUp); });
    connect(this, &MainWindow::askToggleScrolls, editor, [&](bool checked) { editor->toggle(checked, Editor::Has::Scrolls); });
    connect(autoTempSave, &QTimer::timeout, this, [&]()
        {
            activeStory.value().autoTempSave(editor->toPlainText());
        });
    connect(editor, &Editor::textChanged, this, [&]()
        {
            if (!preview->isVisible()) return;
            preview->setText(editor->toPlainText());
        });
    connect(editor, &Editor::selectionChanged, this, [&]()
        {
            editor->hasSelection()
                ? indicator->signalFilter(Indicator::Type::Selection, true)
                : indicator->signalFilter(Indicator::Type::Counts, true);
        });
    connect(editor, &Editor::askTheme, this, [&]() { return editorThemes->checkedAction(); });
    connect(editor, &Editor::askThemes, this, [&]() { return editorThemes; });
    connect(editor, &Editor::askFonts, this, [&]() { return editorFonts; });
    connect(editor, &Editor::textChanged, indicator, [&]() { indicator->signalFilter(Indicator::Type::Counts); });
    connect(editor, &Editor::cursorPositionChanged, indicator, [&]() { indicator->signalFilter(Indicator::Type::Positions); });
    connect(editor, &Editor::askGoNext, pane, [&]() { pane->navigate(Pane::Go::Next); });
    connect(editor, &Editor::askGoPrevious, pane, [&]() { pane->navigate(Pane::Go::Previous); });
    connect(indicator, &Indicator::askForCounts, this, [&](bool isSelection)
        {
            if (isSelection) return Indicator::Counts{ editor->selectedText(), editor->selectedLineCount()};
            return Indicator::Counts{ editor->toPlainText(), editor->blockCount() };
        });
    connect(indicator, &Indicator::askForPositions, this, [&]()
        {
            return Indicator::Positions{ editor->cursorBlockNumber(), editor->cursorPositionInBlock() };
        });
    connect(indicator, &Indicator::askEditorFocusReturn, editor, [&]() { editor->setFocus(); });
    connect(manager, &QNetworkAccessManager::finished, this, [](QNetworkReply* reply) { reply->deleteLater(); });
    connect(pane, &Pane::askSetExpansion, this, [&](QString key, bool isExpanded)
        {
            activeStory.value().setItemExpansion(key, isExpanded);
        });
    connect(preview, &Preview::askEmitTextChanged, this, [&]() { editor->textChanged(); });
    connect(splitter, &Splitter::askWindowSize, this, [&]() { return geometry(); });
    connect(timer, &Tool::resetCountdown, this, [&]() { return getSetting<int>(timerValues); });
}

void MainWindow::shortcuts()
{
    auto cycle_window_themes = new QShortcut(Qt::ALT | Qt::Key_F12, this);
    connect(cycle_window_themes, &QShortcut::activated, this, [&]() { Style::actionCycle(windowThemes); });
    for (const auto& shortcut : { cycle_window_themes })
        shortcut->setAutoRepeat(false);
}

void MainWindow::makeMenuBar()
{
    makeFileMenu();
    makeStoryMenu();
    makeSetMenu();
    makeToggleMenu();
    makeHelpMenu();
    if (!isDev) return;
    makeDevMenu();
}

void MainWindow::makeFileMenu()
{
    auto new_story_project = new QAction(tr("&New story project..."), this);
    auto open_story_project = new QAction(tr("&Open an existing story project..."), this);
    auto save = new QAction(tr("&Save"), this);
    auto quit = new QAction(tr("&Quit"), this);
    save->setShortcut(Qt::CTRL | Qt::Key_S);
    quit->setShortcut(Qt::CTRL | Qt::Key_Q);
    for (const auto& action : { save, quit })
        action->setAutoRepeat(false);
    connect(new_story_project, &QAction::triggered, this, [&]()
        {
            auto file_name = QFileDialog::getSaveFileName(this, tr("Create a new story project..."), Path::toQString(UserData::doThis(UserData::Operation::GetDocuments)), tr("Fernanda story file (*.story)"));
            openStory(Path::toStdFs(file_name));
        });
    connect(open_story_project, &QAction::triggered, this, [&]()
        {
            auto file_name = QFileDialog::getOpenFileName(this, tr("Open an existing story project..."), Path::toQString(UserData::doThis(UserData::Operation::GetDocuments)), tr("Fernanda story file (*.story)"));
            openStory(Path::toStdFs(file_name));
        });
    connect(save, &QAction::triggered, this, &MainWindow::fileMenuSave);
    connect(quit, &QAction::triggered, this, &QCoreApplication::quit, Qt::QueuedConnection);
    auto file = menuBar->addMenu(tr("&File"));
    for (const auto& action : {
        new_story_project,
        open_story_project,
        file->addSeparator(),
        save,
        file->addSeparator(),
        quit
        })
        file->addAction(action);
    save->setEnabled(false);
    connect(this, &MainWindow::storyMenuVisible, save, &QAction::setEnabled);
}

void MainWindow::makeStoryMenu()
{
    auto new_folder = new QAction(tr("&New folder..."), this);
    auto new_file = new QAction(tr("&New file..."), this);
    auto total_counts = new QAction(tr("&Total counts..."), this);
    auto export_directory = new QAction(tr("&To directory (last saved state)..."), this);
    auto export_PDF = new QAction(tr("&To PDF (current state)..."), this);
    auto export_plain_text = new QAction(tr("&To plain text (current state)..."), this);
    connect(new_folder, &QAction::triggered, this, [&]() { askPaneAdd(Path::Type::Dir); });
    connect(new_file, &QAction::triggered, this, [&]() { askPaneAdd(Path::Type::File); });
    connect(total_counts, &QAction::triggered, this, &MainWindow::storyMenuTotals);
    connect(export_directory, &QAction::triggered, this, [&]()
        {
            auto directory = QFileDialog::getExistingDirectory(this, "Choose a directory...", Path::toQString(UserData::doThis(UserData::Operation::GetDocuments)));
            activeStory.value().exportTo(Path::toStdFs(directory), Story::To::Directory);
        });
    connect(export_PDF, &QAction::triggered, this, [&]()
        {
            storyMenuFileExport("Create a new PDF...", "Portable Document Format (*.pdf)", Story::To::PDF);
        });
    connect(export_plain_text, &QAction::triggered, this, [&]()
        {
            storyMenuFileExport("Create a new text document...", "Text documents (*.txt)", Story::To::PlainText);
        });
    auto story = menuBar->addMenu(tr("&Story"));
    for (const auto& action : {
        new_folder,
        new_file,
        story->addSeparator(),
        total_counts,
        story->addSeparator()
        })
        story->addAction(action);
    auto exporting = story->addMenu(tr("&Export"));
    for (const auto& action : { export_directory, export_PDF, export_plain_text })
        exporting->addAction(action);
    story->menuAction()->setVisible(false);
    connect(this, &MainWindow::storyMenuVisible, story->menuAction(), &QAction::setVisible);
}

void MainWindow::makeSetMenu()
{
    auto user_data = UserData::doThis(UserData::Operation::GetUserData);
    auto editor_fonts_list = Resource::iterate(":/fonts/", { "*.otf", "*.ttf" }, user_data);
    auto editor_themes_list = Resource::iterate(":/themes/editor/", { "*.fernanda_editor" }, user_data);
    QVector<Resource::DataPair> editor_tabs_list = {
        Resource::DataPair{ "20", "20 pixels" },
        Resource::DataPair{ "40", "40 pixels" },
        Resource::DataPair{ "60", "60 pixels" },
        Resource::DataPair{ "80", "80 pixels" }
    };
    QVector<Resource::DataPair> editor_wraps_list = {
        Resource::DataPair{ "NoWrap", "No wrap" },
        Resource::DataPair{ "WordWrap", "Wrap at word boundaries" },
        Resource::DataPair{ "WrapAnywhere", "Wrap anywhere" },
        Resource::DataPair{ "WrapAt", "Wrap at word boundaries or anywhere" }
    };
    QVector<Resource::DataPair> preview_types_list = {
        Resource::DataPair{ "Fountain", "Fountain" },
        Resource::DataPair{ "Markdown", "Markdown" }
    };
    QVector<Resource::DataPair> window_color_bar_alignments_list = {
        Resource::DataPair{ "Top", "Top" },
        Resource::DataPair{ "Bottom", "Bottom" }
    };
    QVector<Resource::DataPair> window_tool_timer_list = {
        Resource::DataPair{ "300", "5 minutes" },
        Resource::DataPair{ "600", "10 minutes" },
        Resource::DataPair{ "900", "15 minutes" },
        Resource::DataPair{ "1200", "20 minutes" },
        Resource::DataPair{ "1500", "25 minutes" },
        Resource::DataPair{ "1800", "30 minutes" }
    };
    if (isDev)
        window_tool_timer_list << QVector<Resource::DataPair>{ Resource::DataPair{ "5", "5 seconds (Test)" }, Resource::DataPair{ "30", "30 seconds (Test)" } };
    auto window_themes_list = Resource::iterate(":/themes/window/", { "*.fernanda_window" }, user_data);
    editorFonts = makeViewToggles(editor_fonts_list, [&]()
        {
            editor->handleFont(editorFonts->checkedAction(), fontSlider->value());
        });
    auto editor_font_size_label = new QAction(tr("&Editor font size:"), this);
    auto editor_font_size = new QWidgetAction(this);
    editor_font_size->setDefaultWidget(fontSlider);
    fontSlider->setMinimum(8);
    fontSlider->setMaximum(40);
    editorThemes = makeViewToggles(editor_themes_list, [&]() { editor->setStyle(editorThemes->checkedAction()); });
    tabStops = makeViewToggles(editor_tabs_list, [&]() { sendSetTabStop(getSetting<int>(tabStops)); });
    wrapModes = makeViewToggles(editor_wraps_list, [&]() { sendSetWrapMode(getSetting<QString>(wrapModes)); });
    previewTypes = makeViewToggles(preview_types_list, [&]() { askSetPreviewType(getSetting<QString>(previewTypes)); });
    colorBarAlignments = makeViewToggles(window_color_bar_alignments_list, [&]() { askSetBarAlignment(getSetting<QString>(colorBarAlignments)); });
    auto window_indicator_column_position = new QAction(tr("&Column position"), this);
    auto window_indicator_line_position = new QAction(tr("&Line position"), this);
    auto window_indicator_char_count = new QAction(tr("&Character count"), this);
    auto window_indicator_line_count = new QAction(tr("&Line count"), this);
    auto window_indicator_word_count = new QAction(tr("&Word count"), this);
    timerValues = makeViewToggles(window_tool_timer_list, [&]() { askSetCountdown(getSetting<int>(timerValues)); });
    windowThemes = makeViewToggles(window_themes_list, &MainWindow::setStyle);
    connect(fontSlider, &QSlider::valueChanged, this, [&](int value) { editor->handleFont(editorFonts->checkedAction(), value); });
    connect(window_indicator_column_position, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, UserData::IniValue::ColumnPosition); });
    connect(window_indicator_line_position, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, UserData::IniValue::LinePosition); });
    connect(window_indicator_char_count, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, UserData::IniValue::CharCount); });
    connect(window_indicator_line_count, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, UserData::IniValue::LineCount); });
    connect(window_indicator_word_count, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, UserData::IniValue::WordCount); });
    for (const auto& action : { editor_font_size_label })
        action->setEnabled(false);
    for (const auto& action : { window_indicator_column_position, window_indicator_line_position, window_indicator_char_count, window_indicator_line_count, window_indicator_word_count })
        action->setCheckable(true);
    fontSlider->setValue(UserData::loadConfig(UserData::IniGroup::Editor, UserData::IniValue::EditorFontSize, 16, UserData::Type::Int).toInt());
    loadViewConfig(editorFonts->actions(), UserData::IniGroup::Editor, UserData::IniValue::EditorFont, ":/fonts/Mononoki.ttf");
    loadViewConfig(editorThemes->actions(), UserData::IniGroup::Editor, UserData::IniValue::EditorTheme, ":/themes/editor/Snooze.fernanda_editor");
    loadViewConfig(tabStops->actions(), UserData::IniGroup::Editor, UserData::IniValue::TabStop, "40");
    loadViewConfig(wrapModes->actions(), UserData::IniGroup::Editor, UserData::IniValue::WrapMode, "WrapAt");
    loadViewConfig(previewTypes->actions(), UserData::IniGroup::Preview, UserData::IniValue::PreviewType, "Markdown");
    loadViewConfig(colorBarAlignments->actions(), UserData::IniGroup::Window, UserData::IniValue::ColorBarAlignment, "Top");
    loadMenuToggle(window_indicator_column_position, UserData::IniGroup::Window, UserData::IniValue::ColumnPosition, true);
    loadMenuToggle(window_indicator_line_position, UserData::IniGroup::Window, UserData::IniValue::LinePosition, true);
    loadMenuToggle(window_indicator_char_count, UserData::IniGroup::Window, UserData::IniValue::CharCount, false);
    loadMenuToggle(window_indicator_line_count, UserData::IniGroup::Window, UserData::IniValue::LineCount, true);
    loadMenuToggle(window_indicator_word_count, UserData::IniGroup::Window, UserData::IniValue::WordCount, true);
    loadViewConfig(timerValues->actions(), UserData::IniGroup::Window, UserData::IniValue::ToolTimer, "900");
    loadViewConfig(windowThemes->actions(), UserData::IniGroup::Window, UserData::IniValue::WindowTheme, ":/themes/window/Light.fernanda_window");
    auto set = menuBar->addMenu(tr("&Set"));
    auto editor_font = set->addMenu(tr("&Editor font"));
    editor_font->addActions(editorFonts->actions());
    set->addAction(editor_font_size_label);
    set->addAction(editor_font_size);
    auto editor_theme = set->addMenu(tr("&Editor theme"));
    editor_theme->addActions(editorThemes->actions());
    auto tab_stop_distance = set->addMenu(tr("&Tab stop distance"));
    tab_stop_distance->addActions(tabStops->actions());
    auto wrap_mode = set->addMenu(tr("&Wrap mode"));
    wrap_mode->addActions(wrapModes->actions());
    set->addSeparator();
    auto preview_types = set->addMenu(tr("&Preview"));
    preview_types->addActions(previewTypes->actions());
    set->addSeparator();
    auto color_bar_alignment = set->addMenu(tr("&Color bar alignment"));
    color_bar_alignment->addActions(colorBarAlignments->actions());
    auto indicator_items = set->addMenu(tr("&Indicator"));
    for (const auto& action : { window_indicator_column_position, window_indicator_line_position })
        indicator_items->addAction(action);
    indicator_items->addSeparator();
    for (const auto& action : { window_indicator_char_count, window_indicator_line_count, window_indicator_word_count })
        indicator_items->addAction(action);
    auto timer_values = set->addMenu(tr("&Timer"));
    timer_values->addActions(timerValues->actions());
    auto window_themes = set->addMenu(tr("&Window theme"));
    window_themes->addActions(windowThemes->actions());
}

void MainWindow::makeToggleMenu()
{
    auto data_load_most_recent = new QAction(tr("&Load most recent project on open"), this);
    auto editor_cursor_blink = new QAction(tr("&Blink"), this);
    auto editor_cursor_block = new QAction(tr("&Block"), this);
    auto editor_cursor_center_on_scroll = new QAction(tr("&Center on scroll"), this);
    auto editor_cursor_ensure_visible = new QAction(tr("&Ensure visible"), this);
    auto editor_cursor_typewriter = new QAction(tr("&Typewriter"), this);
    auto editor_current_line_highlight = new QAction(tr("&Current line highlight"), this);
    auto editor_shadow = new QAction(tr("&Editor shadow"), this);
    auto editor_theme = new QAction(tr("&Editor theme"), this);
    auto editor_key_filter = new QAction(tr("&Key filters"), this);
    auto editor_line_number_area = new QAction(tr("&Line number area"), this);
    auto editor_scrolls_previous_next = new QAction(tr("&Scrolls previous and next"), this);
    auto preview_scroll_sync = new QAction(tr("&Preview scroll sync"), this);
    auto window_color_bar = new QAction(tr("&Color bar"), this);
    auto window_indicator = new QAction(tr("&Indicator"), this);
    auto window_preview = new QAction(tr("&Preview"), this);
    auto window_status_bar = new QAction(tr("&Status bar"), this);
    auto window_tools_aot = new QAction(tr("&Always on top"), this);
    auto window_tools_stay_awake = new QAction(tr("&Stay awake"), this);
    auto window_tools_timer = new QAction(tr("&Timer"), this);
    auto window_theme = new QAction(tr("&Window theme"), this);
    connect(data_load_most_recent, &QAction::toggled, this, [&](bool checked)
        {
            UserData::saveConfig(UserData::IniGroup::Data, UserData::IniValue::ToggleLoadMostRecent, checked); // move to story?
        });
    connect(editor_cursor_blink, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorBlink); });
    connect(editor_cursor_block, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorBlock); });
    connect(editor_cursor_center_on_scroll, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorCenterOnScroll); });
    connect(editor_cursor_ensure_visible, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorEnsureVisible); });
    connect(editor_cursor_typewriter, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorTypewriter); });
    connect(editor_current_line_highlight, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::LineHighlight); });
    connect(editor_shadow, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Shadow); });
    connect(editor_theme, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Theme); });
    connect(editor_key_filter, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::KeyFilter); });
    connect(editor_line_number_area, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::LineNumberArea); });
    connect(editor_scrolls_previous_next, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::ExtraScrolls); });
    connect(preview_scroll_sync, &QAction::toggled, this, [&](bool checked) { preview->toggle(checked, Preview::Has::ScrollSync); });
    connect(window_color_bar, &QAction::toggled, this, [&](bool checked) { colorBar->toggle(checked, ColorBar::Has::Self); });
    connect(window_indicator, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(indicator, UserData::IniGroup::Window, UserData::IniValue::ToggleIndicator, checked);
        });
    connect(window_preview, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(preview, UserData::IniGroup::Window, UserData::IniValue::TogglePreview, checked);
        });
    connect(window_status_bar, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(statusBar, UserData::IniGroup::Window, UserData::IniValue::ToggleStatusBar, checked);
        });
    connect(window_tools_aot, &QAction::toggled, this, [&](bool checked) { alwaysOnTop->toggle(checked); });
    connect(window_tools_stay_awake, &QAction::toggled, this, [&](bool checked) { stayAwake->toggle(checked); });
    connect(window_tools_timer, &QAction::toggled, this, [&](bool checked) { timer->toggle(checked); });
    connect(window_theme, &QAction::toggled, this, [&](bool checked)
        {
            hasTheme = checked;
            setStyle();
            UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::ToggleWindowTheme, checked);
        });
    for (const auto& action : {
        data_load_most_recent,
        editor_cursor_blink,
        editor_cursor_block,
        editor_cursor_center_on_scroll,
        editor_cursor_ensure_visible,
        editor_cursor_typewriter,
        editor_current_line_highlight,
        editor_shadow,
        editor_theme,
        editor_key_filter,
        editor_line_number_area,
        editor_scrolls_previous_next,
        preview_scroll_sync,
        window_color_bar,
        window_indicator,
        window_preview,
        window_status_bar,
        window_tools_aot,
        window_tools_stay_awake,
        window_tools_timer,
        window_theme
        })
        action->setCheckable(true);
    loadMenuToggle(data_load_most_recent, UserData::IniGroup::Data, UserData::IniValue::ToggleLoadMostRecent, false);
    loadMenuToggle(editor_cursor_blink, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorBlink, true);
    loadMenuToggle(editor_cursor_block, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorBlock, true);
    loadMenuToggle(editor_cursor_center_on_scroll, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorCenterOnScroll, false);
    loadMenuToggle(editor_cursor_ensure_visible, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorEnsureVisible, true);
    loadMenuToggle(editor_cursor_typewriter, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorTypewriter, false);
    loadMenuToggle(editor_current_line_highlight, UserData::IniGroup::Editor, UserData::IniValue::ToggleLineHighlight, true);
    loadMenuToggle(editor_shadow, UserData::IniGroup::Editor, UserData::IniValue::ToggleEditorShadow, true);
    loadMenuToggle(editor_theme, UserData::IniGroup::Editor, UserData::IniValue::ToggleEditorTheme, true);
    loadMenuToggle(editor_key_filter, UserData::IniGroup::Editor, UserData::IniValue::ToggleKeyFilters, true);
    loadMenuToggle(editor_line_number_area, UserData::IniGroup::Editor, UserData::IniValue::ToggleLineNumberArea, true);
    loadMenuToggle(editor_scrolls_previous_next, UserData::IniGroup::Editor, UserData::IniValue::ToggleScrollsPrevNext, true);
    loadMenuToggle(preview_scroll_sync, UserData::IniGroup::Preview, UserData::IniValue::ToggleScrollSync, true);
    loadMenuToggle(window_color_bar, UserData::IniGroup::Window, UserData::IniValue::ToggleColorBar, true);
    loadMenuToggle(window_indicator, UserData::IniGroup::Window, UserData::IniValue::ToggleIndicator, true);
    loadMenuToggle(window_preview, UserData::IniGroup::Window, UserData::IniValue::TogglePreview, false);
    loadMenuToggle(window_status_bar, UserData::IniGroup::Window, UserData::IniValue::ToggleStatusBar, true);
    loadMenuToggle(window_tools_aot, UserData::IniGroup::Window, UserData::IniValue::ToggleToolAOT, false);
    loadMenuToggle(window_tools_stay_awake, UserData::IniGroup::Window, UserData::IniValue::ToggleToolSA, false);
    loadMenuToggle(window_tools_timer, UserData::IniGroup::Window, UserData::IniValue::ToggleToolTimer, false);
    loadMenuToggle(window_theme, UserData::IniGroup::Window, UserData::IniValue::ToggleWindowTheme, true);
    auto toggle = menuBar->addMenu(tr("&Toggle"));
    toggle->addAction(data_load_most_recent);
    toggle->addSeparator();
    auto cursor = toggle->addMenu(tr("&Cursor"));
    for (const auto& action : { editor_cursor_blink, editor_cursor_block, editor_cursor_center_on_scroll, editor_cursor_ensure_visible, editor_cursor_typewriter })
        cursor->addAction(action);
    for (const auto& action : { editor_current_line_highlight, editor_shadow, editor_theme, editor_key_filter, editor_line_number_area, editor_scrolls_previous_next })
        toggle->addAction(action);
    toggle->addSeparator();
    for (const auto& action : { preview_scroll_sync })
        toggle->addAction(action);
    toggle->addSeparator();
    for (const auto& action : { window_color_bar, window_indicator, window_preview, window_status_bar })
        toggle->addAction(action);
    auto tools = toggle->addMenu(tr("&Tools"));
    for (const auto& action : {
        window_tools_aot,

#ifdef Q_OS_WINDOWS

        window_tools_stay_awake,

#endif

        window_tools_timer
        })
        tools->addAction(action);
    for (const auto& action : { window_theme })
        toggle->addAction(action);   
}

void MainWindow::makeHelpMenu()
{
    auto about = new QAction(tr("&About..."), this);
    auto check_for_updates = new QAction(tr("&Check for updates..."), this);
    auto shortcuts = new QAction(tr("&Shortcuts..."), this);
    auto documents = new QAction(tr("&Documents..."), this);
    auto installation_folder = new QAction(tr("&Installation folder..."), this);
    auto user_data_folder = new QAction(tr("&User data..."), this);
    auto create_sample_project = new QAction(tr("&Create sample project"), this);
    auto create_sample_themes = new QAction(tr("&Create sample themes..."), this);
    connect(about, &QAction::triggered, this, [&]()
        {
            if (!Popup::about(this)) return;
            helpMenuUpdate();
        });
    connect(check_for_updates, &QAction::triggered, this, &MainWindow::helpMenuUpdate);
    connect(shortcuts, &QAction::triggered, this, [&]() { Popup::shortcuts(); });
    connect(documents, &QAction::triggered, this, [&]()
        {
            openLocalFolder(UserData::doThis(UserData::Operation::GetDocuments));
        });
    connect(installation_folder, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Path::toStdFs(QCoreApplication::applicationDirPath()).parent_path());
        });
    connect(user_data_folder, &QAction::triggered, this, [&]()
        {
            openLocalFolder(UserData::doThis(UserData::Operation::GetUserData));
        });
    connect(create_sample_project, &QAction::triggered, this, &MainWindow::helpMenuMakeSampleProject);
    connect(create_sample_themes, &QAction::triggered, this, &MainWindow::helpMenuMakeSampleRes);
    auto help = menuBar->addMenu(tr("&Help"));
    for (const auto& action : { about, check_for_updates, shortcuts, help->addSeparator() })
        help->addAction(action);
    auto open = help->addMenu(tr("&Open"));
    for (const auto& action : { documents, installation_folder, user_data_folder })
        open->addAction(action);
    for (const auto& action : { help->addSeparator(), create_sample_project, create_sample_themes })
        help->addAction(action);
}

void MainWindow::makeDevMenu()
{
    auto remove_all_themes = new QAction(tr("&Remove all themes"), this);
    auto dump_editor_themes = new QAction(tr("&Dump editor theme files"), this);
    auto dump_fonts = new QAction(tr("&Dump font files"), this);
    auto dump_window_themes = new QAction(tr("&Dump window theme files"), this);
    auto print_cursor_positions = new QAction(tr("&Print cursor positions"), this);
    auto print_cuts = new QAction(tr("&Print cuts"), this);
    auto print_dom = new QAction(tr("&Print DOM"), this);
    auto print_dom_initial = new QAction(tr("&Print DOM (Initial)"), this);
    auto print_edited_keys_delegate = new QAction(tr("&Print edited keys (Delegate)"), this);
    auto print_edited_keys_story = new QAction(tr("&Print edited keys (Story)"), this);
    auto print_renames = new QAction(tr("&Print renames"), this);
    auto print_splitter_startup = new QAction(tr("&Print Splitter startup sizes"), this);
    auto print_splitter_states = new QAction(tr("&Print stored Splitter states"), this);
    auto print_window_position = new QAction(tr("&Print window position"), this);
    auto open_documents = new QAction(tr("&Open documents..."), this);
    auto open_installation_folder = new QAction(tr("&Open installation folder..."), this);
    auto open_temps = new QAction(tr("&Open temps..."), this);
    auto open_user_data = new QAction(tr("&Open user data..."), this);
    connect(remove_all_themes, &QAction::triggered, this, [&]()
        {
            setStyleSheet(nullptr);
            editor->devRemoveStyle();
        });
    connect(dump_editor_themes, &QAction::triggered, this, [&]()
        {
            Style::dump(editorThemes, UserData::doThis(UserData::Operation::GetDocuments));
        });
    connect(dump_fonts, &QAction::triggered, this, [&]()
        {
            Style::dump(editorFonts, UserData::doThis(UserData::Operation::GetDocuments));
        });
    connect(dump_window_themes, &QAction::triggered, this, [&]()
        {
            Style::dump(windowThemes, UserData::doThis(UserData::Operation::GetDocuments));
        });
    connect(print_cursor_positions, &QAction::triggered, this, [&]()
        {
            devMenuWrite("__Cursor positions.txt", editor->devGetCursorPositions().join(Text::newLines()));
        });
    connect(print_cuts, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__Cuts.xml", activeStory.value().devGetDom(Dom::Document::Cuts));
        });
    connect(print_dom, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__DOM.xml", activeStory.value().devGetDom());
        });
    connect(print_dom_initial, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__DOM (Initial).xml", activeStory.value().devGetDom(Dom::Document::Initial));
        });
    connect(print_edited_keys_delegate, &QAction::triggered, this, [&]()
        {
            devMenuWrite("__Edited keys (Delegate).txt", pane->devGetEditedKeys().join(Text::newLines()));
        });
    connect(print_edited_keys_story, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__Edited keys (Story).txt", activeStory.value().devGetEditedKeys().join(Text::newLines()));
        });
    connect(print_renames, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            auto renames = activeStory.value().devGetRenames();
            devMenuWrite("__Pending renames.txt", renames.join(Text::newLines()));
        });
    connect(print_splitter_startup, &QAction::triggered, this, [&]()
        {
            auto states = splitter->devGetStartUpSizes();
            devMenuWrite("__Splitter startup sizes.txt", states.join(Text::newLines()));
        });
    connect(print_splitter_states, &QAction::triggered, this, [&]()
        {
            auto states = splitter->devGetStates();
            devMenuWrite("__Stored Splitter states.txt", states.join(Text::newLines()));
        });
    connect(print_window_position, &QAction::triggered, this, [&]()
        {
            devMenuWrite("__Window position.txt", devGetSize().join(Text::newLines()));
        });
    connect(open_documents, &QAction::triggered, this, [&]()
        {
            openLocalFolder(UserData::doThis(UserData::Operation::GetDocuments));
        });
    connect(open_installation_folder, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Path::toStdFs(QCoreApplication::applicationDirPath()).parent_path());
        });
    connect(open_temps, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            openLocalFolder(activeStory.value().devGetActiveTemp());
        });
    connect(open_user_data, &QAction::triggered, this, [&]()
        {
            openLocalFolder(UserData::doThis(UserData::Operation::GetUserData));
        });
    auto dev = menuBar->addMenu(tr("&Dev"));
    for (const auto& action : {
        remove_all_themes,
        dev->addSeparator(),
        dump_editor_themes,
        dump_fonts,
        dump_window_themes,
        dev->addSeparator(),
        print_cursor_positions,
        print_cuts,
        print_dom,
        print_dom_initial,
        print_edited_keys_delegate,
        print_edited_keys_story,
        print_renames,
        print_splitter_startup,
        print_splitter_states,
        print_window_position,
        dev->addSeparator(),
        open_documents,
        open_installation_folder,
        open_temps,
        open_user_data
        })
        dev->addAction(action);
}

void MainWindow::loadConfigs(StdFsPath story)
{
    loadWinConfigs();
    splitter->loadConfig();
    auto is_empty = story.empty();
    auto load_most_recent = UserData::loadConfig(UserData::IniGroup::Data, UserData::IniValue::ToggleLoadMostRecent, false, UserData::Type::Bool).toBool();
    if (load_most_recent || !is_empty)
        askToggleStartUpBar(false);
    if (!is_empty)
    {
        openStory(story);
        return;
    }
    if (!load_most_recent) return;
    auto project = Path::toStdFs(UserData::loadConfig(UserData::IniGroup::Data, UserData::IniValue::MostRecent));
    if (!QFile(project).exists() || project.empty()) return;
    openStory(project);
}

void MainWindow::loadWinConfigs()
{
    auto geometry = UserData::loadConfig(UserData::IniGroup::Window, UserData::IniValue::WindowPosition, QRect(0, 0, 1000, 666), UserData::Type::QRect).toRect();
    setGeometry(geometry);
    auto win_state = UserData::loadConfig(UserData::IniGroup::Window, UserData::IniValue::WindowState).toInt();
    if (win_state == 1) setWindowState(Qt::WindowState::WindowMinimized);
    else if (win_state == 2) setWindowState(Qt::WindowState::WindowMaximized);
    else if (win_state == 4) setWindowState(Qt::WindowState::WindowFullScreen);
    stayAwake->setChecked(UserData::loadConfig(UserData::IniGroup::Window, UserData::IniValue::StayAwake, false).toBool());
    alwaysOnTop->setChecked(UserData::loadConfig(UserData::IniGroup::Window, UserData::IniValue::AlwaysOnTop, false).toBool());
}

void MainWindow::loadViewConfig(QVector<QAction*> actions, UserData::IniGroup group, UserData::IniValue valueType, QVariant fallback)
{
    auto resource = UserData::loadConfig(group, valueType, fallback);
    for (auto& action : actions)
        if (Path::toStdFs(action->data()) == Path::toStdFs(resource))
        {
            action->setChecked(true);
            return;
        }
    for (auto& action : actions)
        if (Path::toStdFs(action->data()) == Path::toStdFs(fallback))
        {
            action->setChecked(true);
            return;
        }
    actions.first()->setChecked(true);
}

void MainWindow::loadMenuToggle(QAction* action, UserData::IniGroup group, UserData::IniValue valueType, QVariant fallback)
{
    auto toggle_state = UserData::loadConfig(group, valueType, fallback, UserData::Type::Bool).toBool();
    action->setChecked(!toggle_state);
    action->setChecked(toggle_state);
}

void MainWindow::openStory(StdFsPath fileName, Story::Mode mode)
{
    if (fileName.empty())
    {
        colorBar->run(ColorBar::Run::Red);
        return;
    }
    auto change = confirmStoryClose();
    if (!change) return;
    UserData::clear(UserData::doThis(UserData::Operation::GetActiveTemp));
    activeStory = Story(fileName, mode);
    auto& story = activeStory.value();
    storyMenuVisible(true);
    askEditorClose(true);
    sendItems(story.items());
    colorBar->run(ColorBar::Run::Green);
    UserData::saveConfig(UserData::IniGroup::Data, UserData::IniValue::MostRecent, Path::toQString(fileName));
}

void MainWindow::toggleWidget(QWidget* widget, UserData::IniGroup group, UserData::IniValue valueType, bool value)
{
    widget->setVisible(value);
    UserData::saveConfig(group, valueType, value);
}

void MainWindow::storyMenuFileExport(const char* caption, const char* extensionFilter, Story::To type)
{
    auto& story = activeStory.value();
    auto file_name = QFileDialog::getSaveFileName(this, tr(caption), Path::toQString(UserData::doThis(UserData::Operation::GetDocuments) / story.name<StdFsPath>()), tr(extensionFilter));
    story.exportTo(Path::toStdFs(file_name), type);
}

void MainWindow::adjustTitle()
{
    auto current_title = windowTitle();
    auto title = name();
    if (activeStory.has_value())
    {
        auto& story = activeStory.value();
        story.hasChanges()
            ? title = "*" + story.name<QString>() + " - " + title
            : title = story.name<QString>() + " - " + title;
    }
    if (current_title == title) return;
    setWindowTitle(title);
}

void MainWindow::setStyle()
{
    if (auto selection = windowThemes->checkedAction(); selection != nullptr)
    {
        auto theme_path = Path::toStdFs(selection->data());
        auto window_style = Style::windowStyle(theme_path, hasTheme);
        setStyleSheet(window_style);
        askToggleScrolls(hasTheme);
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WindowTheme, Path::toQString(theme_path));
    }
}

void MainWindow::handleFontSlider(PlainTextEdit::Zoom direction)
{
    switch (direction) {
    case PlainTextEdit::Zoom::In:
        fontSlider->setValue(fontSlider->value() + 2);
        break;
    case PlainTextEdit::Zoom::Out:
        fontSlider->setValue(fontSlider->value() - 2);
        break;
    }
}

void MainWindow::fileMenuSave()
{
    auto& story = activeStory.value();
    if (!story.hasChanges()) return;
    story.save(editor->toPlainText());
    UserData::clear(UserData::doThis(UserData::Operation::GetActiveTemp));
    editor->textChanged();
    sendItems(story.items());
    colorBar->run(ColorBar::Run::Green);
}

void MainWindow::storyMenuTotals()
{
    auto& story = activeStory.value();
    story.autoTempSave(editor->toPlainText());
    auto totals = story.totalCounts();
    Popup::totalCounts(totals.lines, totals.words, totals.characters);
}

void MainWindow::helpMenuMakeSampleProject()
{
    auto path = UserData::doThis(UserData::Operation::GetDocuments) / "Candide.story";
    openStory(path, Story::Mode::Sample);
}

void MainWindow::helpMenuMakeSampleRes()
{
    auto path = UserData::doThis(UserData::Operation::GetUserData);
    Sample::makeRc(path);
    colorBar->run(ColorBar::Run::Pastels);
    switch (Popup::sample()) {
    case Popup::Action::Accept:
        break;
    case Popup::Action::Open:
        openLocalFolder(path);
        break;
    }
}

void MainWindow::helpMenuUpdate()
{
    auto request = QNetworkRequest(QUrl(Text::gitHubApi()));
    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [=]()
        {
            Text::VersionCheck result{};
            QString latest = nullptr;
            if (reply->error() == QNetworkReply::NoError)
            {
                auto document = QJsonDocument::fromJson(reply->readAll());
                QList<QVariant> list = document.toVariant().toList();
                QMap<QString, QVariant> map = list[0].toMap();
                latest = map["tag_name"].toString();
                (latest == QString(VER_FILEVERSION_STR))
                    ? result = Text::VersionCheck::Latest
                    : result = Text::VersionCheck::Old;
            }
            else
                result = Text::VersionCheck::Error;
            Popup::update(result, latest);
        });
}

void MainWindow::handleEditorOpen(QString key)
{
    QString old_key = nullptr;
    if (activeStory.has_value())
        old_key = activeStory.value().key();
    auto action = editor->handleKeySwap(old_key, key);
    switch (action) {
    case Editor::Action::None:
        break;
    case Editor::Action::AcceptNew:
        {
            auto text = activeStory.value().tempSaveOld_openNew(key, editor->toPlainText());
            editor->handleTextSwap(key, text);
            startAutoTempSave();
            if (!indicator->autoCountActive())
                indicator->signalFilter(Indicator::Type::Counts, true);
        }
        break;
    }
}

void MainWindow::sendEditedText()
{
    if (!activeStory.has_value()) return;
    sendEditsList(activeStory.value().edits(editor->toPlainText()));
}

void MainWindow::domMove(QString pivotKey, QString fulcrumKey, Io::Move position)
{
    auto& story = activeStory.value();
    story.move(pivotKey, fulcrumKey, position);
    sendItems(story.items());
}

void MainWindow::domAdd(QString newName, Path::Type type, QString parentKey)
{
    auto& story = activeStory.value();
    story.add(newName, type, parentKey);
    sendItems(story.items());
    editor->textChanged();
}

void MainWindow::domRename(QString newName, QString key)
{
    auto& story = activeStory.value();
    story.rename(newName, key);
    sendItems(story.items());
}

void MainWindow::domCut(QString key)
{
    auto& story = activeStory.value();
    auto active_key_cut = story.cut(key);
    sendItems(story.items());
    if (!active_key_cut)
    {
        editor->setFocus();
        return;
    }
    askEditorClose();
}

// MainWindow.cpp, Fernanda
