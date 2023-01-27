// fernanda.cpp, Fernanda

#include "fernanda.h"

Fernanda::Fernanda(bool isDev, StdFsPath story, QWidget* parent)
    : isDev(isDev), QMainWindow(parent)
{
    UserData::setName(name());
    addWidgets();
    connections();
    shortcuts();
    UserData::doThis();
    makeMenuBar();
    loadConfigs(story);
}

void Fernanda::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (isInitialized || event->spontaneous()) return;
    if (askHasStartUpBar())
        colorBar->delayedStartUp();
    isInitialized = true;
}

void Fernanda::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WindowPosition, geometry());
}

void Fernanda::moveEvent(QMoveEvent* event)
{
    QMainWindow::moveEvent(event);
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WindowPosition, geometry());
}

void Fernanda::closeEvent(QCloseEvent* event)
{
    auto state = windowState();
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::WindowState, state.toInt());
    setWindowState(Qt::WindowState::WindowActive);
    auto quit = confirmStoryClose(true);
    if (!quit)
    {
        setWindowState(state);
        event->ignore();
        colorBar->run(ColorBar::Run::Green);
        return;
    }
    UserData::clear(UserData::doThis(UserData::Operation::GetActiveTemp), true);
    event->accept();
}

bool Fernanda::confirmStoryClose(bool isQuit)
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

void Fernanda::openLocalFolder(StdFsPath path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(Path::toQString(path)));
}

const QStringList Fernanda::devPrintRenames(QVector<Io::ArchiveRename> renames)
{
    QStringList result;
    auto i = 0;
    for (auto& rename : renames)
    {
        ++i;
        QString entry = QString::number(i) + "\nKey: " + rename.key + "\nRelative Path: " + Path::toQString(rename.relativePath);
        (rename.originalRelativePath.has_value())
            ? entry = entry + "\nOriginal Path: " + Path::toQString(rename.originalRelativePath.value())
            : entry = entry + "\nNew: " + QString((rename.typeIfNewOrCut.value() == Path::Type::Dir) ? "directory" : "file");
        result << entry;
    }
    return result;
}

const QString Fernanda::name()
{
    QString result;
    (isDev) ? result = "Fernanda (dev)" : result = "Fernanda";
    return result;
}

void Fernanda::addWidgets()
{
    setCentralWidget(Layout::stackWidgets({ colorBar, splitter }));
    splitter->addWidgets({ pane, editor });
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

void Fernanda::connections()
{
    connect(this, &Fernanda::askSetBarAlignment, colorBar, &ColorBar::setAlignment);
    connect(this, &Fernanda::askHasStartUpBar, colorBar, &ColorBar::hasStartUp);
    connect(this, &Fernanda::askUpdatePositions, indicator, &Indicator::updatePositions);
    connect(this, &Fernanda::askUpdateCounts, indicator, &Indicator::updateCounts);
    connect(this, &Fernanda::askUpdateSelection, indicator, &Indicator::updateSelection);
    connect(this, &Fernanda::askEditorClose, editor, &Editor::close);
    connect(this, &Fernanda::sendSetTabStop, editor, &Editor::setTabStop);
    connect(this, &Fernanda::sendSetWrapMode, editor, &Editor::setWrapMode);
    connect(this, &Fernanda::sendItems, pane, &Pane::receiveItems);
    connect(this, &Fernanda::sendEditsList, pane, &Pane::receiveEditsList);
    connect(this, &Fernanda::askPaneAdd, pane, &Pane::add);
    connect(this, &Fernanda::askSetCountdown, timer, &Tool::setCountdown);
    connect(editor, &Editor::askFontSliderZoom, this, &Fernanda::handleFontSlider);
    connect(editor, &Editor::askHasProject, this, &Fernanda::replyHasProject);
    connect(editor, &Editor::textChanged, this, &Fernanda::sendEditedText);
    connect(editor, &Editor::textChanged, this, &Fernanda::adjustTitle);
    connect(pane, &Pane::askDomMove, this, &Fernanda::domMove);
    connect(pane, &Pane::askAddElement, this, &Fernanda::domAdd);
    connect(pane, &Pane::askRenameElement, this, &Fernanda::domRename);
    connect(pane, &Pane::askCutElement, this, &Fernanda::domCut);
    connect(pane, &Pane::askHasProject, this, &Fernanda::replyHasProject);
    connect(pane, &Pane::askSendToEditor, this, &Fernanda::handleEditorOpen);
    connect(pane, &Pane::askTitleCheck, this, &Fernanda::adjustTitle);
    connect(this, &Fernanda::startAutoTempSave, this, [&]() { autoTempSave->start(20000); });
    connect(this, &Fernanda::askToggleStartUpBar, colorBar, [&](bool checked) { colorBar->toggle(checked, ColorBar::Has::RunOnStartUp); });
    connect(this, &Fernanda::askToggleScrolls, editor, [&](bool checked) { editor->toggle(checked, Editor::Has::Scrolls); });
    connect(autoTempSave, &QTimer::timeout, this, [&]()
        {
            activeStory.value().autoTempSave(editor->toPlainText());
        });
    connect(editor, &Editor::askGoNext, pane, [&]() { pane->navigate(Pane::Go::Next); });
    connect(editor, &Editor::askGoPrevious, pane, [&]() { pane->navigate(Pane::Go::Previous); });
    connect(editor, &Editor::cursorPositionChanged, this, [&]()
        {
            askUpdatePositions(editor->cursorBlockNumber(), editor->cursorPositionInBlock());
        });
    connect(editor, &Editor::textChanged, this, [&]()
        {
            askUpdateCounts(editor->toPlainText(), editor->blockCount());
        });
    connect(editor, &Editor::selectionChanged, this, [&]()
        {
            (editor->hasSelection())
                ? askUpdateSelection(editor->selectedText(), editor->selectedLineCount())
                : editor->textChanged();
        });
    connect(editor, &Editor::askTheme, this, [&]() { return editorThemes->checkedAction(); });
    connect(editor, &Editor::askThemes, this, [&]() { return editorThemes; });
    connect(editor, &Editor::askFonts, this, [&]() { return editorFonts; });
    connect(indicator, &Indicator::askSignalCursorPositionChanged, this, [&]() { editor->cursorPositionChanged(); });
    connect(indicator, &Indicator::askSignalTextChanged, this, [&]() { editor->textChanged(); });
    connect(manager, &QNetworkAccessManager::finished, this, [](QNetworkReply* reply) { reply->deleteLater(); });
    connect(pane, &Pane::askSetExpansion, this, [&](QString key, bool isExpanded)
        {
            activeStory.value().setItemExpansion(key, isExpanded);
        });
    connect(timer, &Tool::resetCountdown, this, [&]() { return getSetting<int>(timerValues); });
}

void Fernanda::shortcuts()
{
    auto cycle_window_themes = new QShortcut(Qt::ALT | Qt::Key_F12, this);
    connect(cycle_window_themes, &QShortcut::activated, this, [&]() { Style::actionCycle(windowThemes); });
    for (const auto& shortcut : { cycle_window_themes })
        shortcut->setAutoRepeat(false);
}

void Fernanda::makeMenuBar()
{
    makeFileMenu();
    makeStoryMenu();
    makeSetMenu();
    makeToggleMenu();
    makeHelpMenu();
    if (!isDev) return;
    makeDevMenu();
}

void Fernanda::makeFileMenu()
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
    connect(save, &QAction::triggered, this, &Fernanda::fileMenuSave);
    connect(quit, &QAction::triggered, this, &QCoreApplication::quit, Qt::QueuedConnection);
    auto file = menuBar->addMenu(tr("&File"));
    for (const auto& action : { new_story_project, open_story_project })
        file->addAction(action);
    file->addSeparator();
    file->addAction(save);
    file->addSeparator();
    file->addAction(quit);
}

void Fernanda::makeStoryMenu()
{
    auto new_folder = new QAction(tr("&New folder..."), this);
    auto new_file = new QAction(tr("&New file..."), this);
    auto total_counts = new QAction(tr("&Total counts..."), this);
    connect(new_folder, &QAction::triggered, this, [&]() { askPaneAdd(Path::Type::Dir); });
    connect(new_file, &QAction::triggered, this, [&]() { askPaneAdd(Path::Type::File); });
    connect(total_counts, &QAction::triggered, this, [&]()
        {
            auto totals = activeStory.value().totalCounts();
            Popup::totalCounts(totals.lines, totals.words, totals.characters);
        });
    auto story = menuBar->addMenu(tr("&Story"));
    for (const auto& action : { new_folder, new_file })
        story->addAction(action);
    story->addSeparator();
    for (const auto& action : { total_counts })
        story->addAction(action);
    story->menuAction()->setVisible(false);
    connect(this, &Fernanda::storyMenuVisible, story->menuAction(), &QAction::setVisible);
}

void Fernanda::makeSetMenu()
{
    auto user_data = UserData::doThis(UserData::Operation::GetUserData);
    QVector<Resource::DataPair> color_bar_alignments_list = {
        Resource::DataPair{ "Top", "Top" },
        Resource::DataPair{ "Bottom", "Bottom" }
    };
    QVector<Resource::DataPair> timer_values_list = {
        Resource::DataPair{ "300", "5 minutes" },
        Resource::DataPair{ "600", "10 minutes" },
        Resource::DataPair{ "900", "15 minutes" },
        Resource::DataPair{ "1200", "20 minutes" },
        Resource::DataPair{ "1500", "25 minutes" },
        Resource::DataPair{ "1800", "30 minutes" }
    };
    if (isDev)
        timer_values_list << QVector<Resource::DataPair>{ Resource::DataPair{ "5", "5 seconds (Test)" }, Resource::DataPair{ "30", "30 seconds (Test)" } };
    auto window_themes_list = Resource::iterate(":/themes/window/", { "*.fernanda_window" }, user_data);
    auto fonts_list = Resource::iterate(":/fonts/", { "*.otf", "*.ttf" }, user_data);
    auto editor_themes_list = Resource::iterate(":/themes/editor/", { "*.fernanda_editor" }, user_data);
    QVector<Resource::DataPair> tab_stops_list = {
        Resource::DataPair{ "20", "20 pixels" },
        Resource::DataPair{ "40", "40 pixels" },
        Resource::DataPair{ "60", "60 pixels" },
        Resource::DataPair{ "80", "80 pixels" }
    };
    QVector<Resource::DataPair> wrap_modes_list = {
        Resource::DataPair{ "NoWrap", "No wrap" },
        Resource::DataPair{ "WordWrap", "Wrap at word boundaries" },
        Resource::DataPair{ "WrapAnywhere", "Wrap anywhere" },
        Resource::DataPair{ "WrapAt", "Wrap at word boundaries or anywhere" }
    };
    colorBarAlignments = makeViewToggles(color_bar_alignments_list, [&]() { askSetBarAlignment(getSetting<QString>(colorBarAlignments)); });
    auto column_position_set = new QAction(tr("&Column position"), this);
    auto line_position_set = new QAction(tr("&Line position"), this);
    auto character_count_set = new QAction(tr("&Character count"), this);
    auto line_count_set = new QAction(tr("&Line count"), this);
    auto word_count_set = new QAction(tr("&Word count"), this);
    timerValues = makeViewToggles(timer_values_list, [&]() { askSetCountdown(getSetting<int>(timerValues)); });
    windowThemes = makeViewToggles(window_themes_list, &Fernanda::setStyle);
    editorFonts = makeViewToggles(fonts_list, [&]()
        {
            editor->handleFont(editorFonts->checkedAction(), fontSlider->value());
        });
    auto font_size_label = new QAction(tr("&Editor font size:"), this);
    auto font_size = new QWidgetAction(this);
    font_size->setDefaultWidget(fontSlider);
    fontSlider->setMinimum(8);
    fontSlider->setMaximum(40);
    editorThemes = makeViewToggles(editor_themes_list, [&]() { editor->setStyle(editorThemes->checkedAction()); });
    tabStops = makeViewToggles(tab_stops_list, [&]() { sendSetTabStop(getSetting<int>(tabStops)); });
    wrapModes = makeViewToggles(wrap_modes_list, [&]() { sendSetWrapMode(getSetting<QString>(wrapModes)); });
    connect(column_position_set, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::ColumnPosition); });
    connect(line_position_set, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::LinePosition); });
    connect(character_count_set, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::CharCount); });
    connect(line_count_set, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::LineCount); });
    connect(word_count_set, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::WordCount); });
    connect(fontSlider, &QSlider::valueChanged, this, [&](int value) { editor->handleFont(editorFonts->checkedAction(), value); });
    for (const auto& action : { column_position_set, line_position_set, character_count_set, line_count_set, word_count_set })
        action->setCheckable(true);
    for (const auto& action : { font_size_label })
        action->setEnabled(false);
    loadViewConfig(colorBarAlignments->actions(), UserData::IniGroup::Window, UserData::IniValue::ColorBarAlignment, "Top");
    loadMenuToggle(column_position_set, UserData::IniGroup::Window, UserData::IniValue::ColumnPosition, true);
    loadMenuToggle(line_position_set, UserData::IniGroup::Window, UserData::IniValue::LinePosition, true);
    loadMenuToggle(character_count_set, UserData::IniGroup::Window, UserData::IniValue::CharCount, false);
    loadMenuToggle(line_count_set, UserData::IniGroup::Window, UserData::IniValue::LineCount, true);
    loadMenuToggle(word_count_set, UserData::IniGroup::Window, UserData::IniValue::WordCount, true);
    loadViewConfig(timerValues->actions(), UserData::IniGroup::Window, UserData::IniValue::ToolTimer, "900");
    loadViewConfig(windowThemes->actions(), UserData::IniGroup::Window, UserData::IniValue::WindowTheme, ":/themes/window/Light.fernanda_window");
    fontSlider->setValue(UserData::loadConfig(UserData::IniGroup::Editor, UserData::IniValue::EditorFontSize, 16, UserData::Type::Int).toInt());
    loadViewConfig(editorFonts->actions(), UserData::IniGroup::Editor, UserData::IniValue::EditorFont, ":/fonts/Cascadia Mono.ttf");
    loadViewConfig(editorThemes->actions(), UserData::IniGroup::Editor, UserData::IniValue::EditorTheme, ":/themes/editor/Amber.fernanda_editor");
    loadViewConfig(tabStops->actions(), UserData::IniGroup::Editor, UserData::IniValue::TabStop, "40");
    loadViewConfig(wrapModes->actions(), UserData::IniGroup::Editor, UserData::IniValue::WrapMode, "WrapAt");
    auto set = menuBar->addMenu(tr("&Set"));
    auto color_bar_alignment = set->addMenu(tr("&Color bar alignment"));
    color_bar_alignment->addActions(colorBarAlignments->actions());
    auto indicator_items = set->addMenu(tr("&Indicator"));
    for (const auto& action : { column_position_set, line_position_set })
        indicator_items->addAction(action);
    indicator_items->addSeparator();
    for (const auto& action : { character_count_set, line_count_set, word_count_set })
        indicator_items->addAction(action);
    auto timer_values = set->addMenu(tr("&Timer"));
    timer_values->addActions(timerValues->actions());
    auto window_themes = set->addMenu(tr("&Window theme"));
    window_themes->addActions(windowThemes->actions());
    set->addSeparator();
    auto editor_font = set->addMenu(tr("&Editor font"));
    editor_font->addActions(editorFonts->actions());
    set->addAction(font_size_label);
    set->addAction(font_size);
    auto editor_theme = set->addMenu(tr("&Editor theme"));
    editor_theme->addActions(editorThemes->actions());
    auto tab_stop_distance = set->addMenu(tr("&Tab stop distance"));
    tab_stop_distance->addActions(tabStops->actions());
    auto wrap_mode = set->addMenu(tr("&Wrap mode"));
    wrap_mode->addActions(wrapModes->actions());
}

void Fernanda::makeToggleMenu()
{
    auto color_bar_toggle = new QAction(tr("&Color bar"), this);
    auto indicator_toggle = new QAction(tr("&Indicator"), this);
    auto pane_toggle = new QAction(tr("&Pane"), this);
    auto status_bar_toggle = new QAction(tr("&Status bar"), this);
    auto aot_toggle = new QAction(tr("&Always-on-top"), this);
    auto stay_awake_toggle = new QAction(tr("&Stay awake"), this);
    auto timer_toggle = new QAction(tr("&Timer"), this);
    auto window_theme_toggle = new QAction(tr("&Window theme"), this);
    auto cursor_blink_toggle = new QAction(tr("&Blink"), this);
    auto cursor_block_toggle = new QAction(tr("&Block"), this);
    auto current_line_highlight_toggle = new QAction(tr("&Current line highlight"), this);
    auto editor_shadow_toggle = new QAction(tr("&Editor shadow"), this);
    auto editor_theme_toggle = new QAction(tr("&Editor theme"), this);
    auto key_filter_toggle = new QAction(tr("&Key filters"), this);
    auto line_number_area_toggle = new QAction(tr("&Line number area"), this);
    auto scrolls_previous_next_toggle = new QAction(tr("&Scrolls previous and next"), this);
    auto load_most_recent_toggle = new QAction(tr("&Load most recent project on open"), this);
    connect(color_bar_toggle, &QAction::toggled, this, [&](bool checked) { colorBar->toggle(checked, ColorBar::Has::Self); });
    connect(indicator_toggle, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(indicator, UserData::IniGroup::Window, UserData::IniValue::ToggleIndicator, checked);
        });
    connect(pane_toggle, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(pane, UserData::IniGroup::Window, UserData::IniValue::TogglePane, checked);
        });
    connect(status_bar_toggle, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(statusBar, UserData::IniGroup::Window, UserData::IniValue::ToggleStatusBar, checked);
        });
    connect(aot_toggle, &QAction::toggled, this, [&](bool checked) { alwaysOnTop->toggle(checked); });
    connect(stay_awake_toggle, &QAction::toggled, this, [&](bool checked) { stayAwake->toggle(checked); });
    connect(timer_toggle, &QAction::toggled, this, [&](bool checked) { timer->toggle(checked); });
    connect(window_theme_toggle, &QAction::toggled, this, [&](bool checked)
        {
            hasTheme = checked;
            setStyle();
            UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::ToggleWindowTheme, checked);
        });
    connect(cursor_blink_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorBlink); });
    connect(cursor_block_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::BlockCursor); });
    connect(current_line_highlight_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::LineHighlight); });
    connect(editor_shadow_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Shadow); });
    connect(editor_theme_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Theme); });
    connect(key_filter_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Keyfilter); });
    connect(line_number_area_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::LineNumberArea); });
    connect(scrolls_previous_next_toggle, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::ExtraScrolls); });
    connect(load_most_recent_toggle, &QAction::toggled, this, [&](bool checked)
        {
            UserData::saveConfig(UserData::IniGroup::Data, UserData::IniValue::ToggleLoadMostRecent, checked); // move to story?
        });
    for (const auto& action : {
        color_bar_toggle,
        indicator_toggle,
        pane_toggle,
        status_bar_toggle,
        aot_toggle,
        stay_awake_toggle,
        timer_toggle,
        window_theme_toggle,
        cursor_blink_toggle,
        cursor_block_toggle,
        current_line_highlight_toggle,
        editor_shadow_toggle,
        editor_theme_toggle,
        key_filter_toggle,
        line_number_area_toggle,
        scrolls_previous_next_toggle,
        load_most_recent_toggle
        })
        action->setCheckable(true);
    loadMenuToggle(color_bar_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleColorBar, true);
    loadMenuToggle(indicator_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleIndicator, true);
    loadMenuToggle(pane_toggle, UserData::IniGroup::Window, UserData::IniValue::TogglePane, true);
    loadMenuToggle(status_bar_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleStatusBar, true);
    loadMenuToggle(aot_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleToolAOT, false);
    loadMenuToggle(stay_awake_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleToolSA, false);
    loadMenuToggle(timer_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleToolTimer, false);
    loadMenuToggle(window_theme_toggle, UserData::IniGroup::Window, UserData::IniValue::ToggleWindowTheme, true);
    loadMenuToggle(cursor_blink_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorBlink, true);
    loadMenuToggle(cursor_block_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleCursorBlock, true);
    loadMenuToggle(current_line_highlight_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleLineHighlight, true);
    loadMenuToggle(editor_shadow_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleEditorShadow, true);
    loadMenuToggle(editor_theme_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleEditorTheme, true);
    loadMenuToggle(key_filter_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleKeyFilters, true);
    loadMenuToggle(line_number_area_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleLineNumberArea, true);
    loadMenuToggle(scrolls_previous_next_toggle, UserData::IniGroup::Editor, UserData::IniValue::ToggleScrollsPrevNext, true);
    loadMenuToggle(load_most_recent_toggle, UserData::IniGroup::Data, UserData::IniValue::ToggleLoadMostRecent, false);
    auto toggle = menuBar->addMenu(tr("&Toggle"));
    for (const auto& action : { color_bar_toggle, indicator_toggle, pane_toggle, status_bar_toggle })
        toggle->addAction(action);
    auto tools = toggle->addMenu(tr("&Tools"));
    for (const auto& action : {
        aot_toggle,
        
#ifdef Q_OS_WINDOWS

        stay_awake_toggle,

#endif

        timer_toggle
        })
        tools->addAction(action);
    for (const auto& action : { window_theme_toggle })
        toggle->addAction(action);
    toggle->addSeparator();
    auto cursor = toggle->addMenu(tr("&Cursor"));
    for (const auto& action : { cursor_blink_toggle, cursor_block_toggle })
        cursor->addAction(action);
    for (const auto& action : { current_line_highlight_toggle, editor_shadow_toggle, editor_theme_toggle, key_filter_toggle, line_number_area_toggle, scrolls_previous_next_toggle })
        toggle->addAction(action);
    toggle->addSeparator();
    toggle->addAction(load_most_recent_toggle);
}

void Fernanda::makeHelpMenu()
{
    auto about = new QAction(tr("&About..."), this);
    auto check_for_updates = new QAction(tr("&Check for updates..."), this);
    auto shortcuts = new QAction(tr("&Shortcuts..."), this);
    auto documents = new QAction(tr("&Documents..."), this);
    auto installation_folder = new QAction(tr("&Installation folder..."), this);
    auto user_data_folder = new QAction(tr("&User data..."), this);
    auto create_sample_project = new QAction(tr("&Create sample project"), this);
    auto create_sample_themes = new QAction(tr("&Create sample themes..."), this);
    connect(about, &QAction::triggered, this, [&]() { Popup::about(this); });
    connect(check_for_updates, &QAction::triggered, this, &Fernanda::helpMenuUpdate);
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
    connect(create_sample_project, &QAction::triggered, this, &Fernanda::helpMenuMakeSampleProject);
    connect(create_sample_themes, &QAction::triggered, this, &Fernanda::helpMenuMakeSampleRes);
    auto help = menuBar->addMenu(tr("&Help"));
    for (const auto& action : { about, check_for_updates, shortcuts })
        help->addAction(action);
    help->addSeparator();
    auto open = help->addMenu(tr("&Open"));
    for (const auto& action : { documents, installation_folder, user_data_folder })
        open->addAction(action);
    help->addSeparator();
    for (const auto& action : { create_sample_project, create_sample_themes })
        help->addAction(action);
}

void Fernanda::makeDevMenu()
{
    auto print_cursor_positions = new QAction(tr("&Print cursor positions"), this);
    auto print_cuts = new QAction(tr("&Print cuts"), this);
    auto print_dom = new QAction(tr("&Print DOM"), this);
    auto print_dom_initial = new QAction(tr("&Print DOM (Initial)"), this);
    auto print_edited_keys_delegate = new QAction(tr("&Print edited keys (Delegate)"), this);
    auto print_edited_keys_story = new QAction(tr("&Print edited keys (Story)"), this);
    auto print_renames = new QAction(tr("&Print renames"), this);
    auto open_documents = new QAction(tr("&Open documents..."), this);
    auto open_installation_folder = new QAction(tr("&Open installation folder..."), this);
    auto open_temps = new QAction(tr("&Open temps..."), this);
    auto open_user_data = new QAction(tr("&Open user data..."), this);
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
            auto renames = devPrintRenames(activeStory.value().devGetRenames());
            devMenuWrite("__Renames.txt", renames.join(Text::newLines()));
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
    for (const auto& action : { print_cursor_positions, print_cuts, print_dom, print_dom_initial, print_edited_keys_delegate, print_edited_keys_story, print_renames })
        dev->addAction(action);
    dev->addSeparator();
    for (const auto& action : { open_documents, open_installation_folder, open_temps, open_user_data })
        dev->addAction(action);
}

void Fernanda::loadConfigs(StdFsPath story)
{
    loadWinConfigs();
    splitter->loadConfig(geometry());
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

void Fernanda::loadWinConfigs()
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

void Fernanda::loadViewConfig(QVector<QAction*> actions, UserData::IniGroup group, UserData::IniValue valueType, QVariant fallback)
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

void Fernanda::loadMenuToggle(QAction* action, UserData::IniGroup group, UserData::IniValue valueType, QVariant fallback)
{
    auto toggle_state = UserData::loadConfig(group, valueType, fallback, UserData::Type::Bool).toBool();
    action->setChecked(!toggle_state);
    action->setChecked(toggle_state);
}

void Fernanda::openStory(StdFsPath fileName, Story::Mode mode)
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

void Fernanda::toggleWidget(QWidget* widget, UserData::IniGroup group, UserData::IniValue valueType, bool value)
{
    widget->setVisible(value);
    UserData::saveConfig(group, valueType, value);
}

void Fernanda::adjustTitle()
{
    auto current_title = windowTitle();
    auto title = name();
    if (activeStory.has_value())
    {
        auto& story = activeStory.value();
        (story.hasChanges())
            ? title = "*" + story.name<QString>() + " - " + title
            : title = story.name<QString>() + " - " + title;
    }
    if (current_title == title) return;
    setWindowTitle(title);
}

void Fernanda::setStyle()
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

void Fernanda::handleFontSlider(PlainTextEdit::Zoom direction)
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

void Fernanda::fileMenuSave()
{
    if (!activeStory.has_value()) return;
    auto& story = activeStory.value();
    if (!story.hasChanges()) return;
    story.save(editor->toPlainText());
    UserData::clear(UserData::doThis(UserData::Operation::GetActiveTemp));
    editor->textChanged();
    sendItems(story.items());
    colorBar->run(ColorBar::Run::Green);
}

void Fernanda::helpMenuMakeSampleProject()
{
    auto path = UserData::doThis(UserData::Operation::GetDocuments) / "Candide.story";
    openStory(path, Story::Mode::Sample);
}

void Fernanda::helpMenuMakeSampleRes()
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

void Fernanda::helpMenuUpdate()
{
    auto request = QNetworkRequest(QUrl(Text::ghApi()));
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

void Fernanda::devMenuWrite(QString name, QString value)
{
    Io::writeFile(UserData::doThis(UserData::Operation::GetDocuments) / name.toStdString(), value);
}

void Fernanda::handleEditorOpen(QString key)
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
        }
        break;
    }
}

void Fernanda::sendEditedText()
{
    if (!activeStory.has_value()) return;
    sendEditsList(activeStory.value().edits(editor->toPlainText()));
}

bool Fernanda::replyHasProject()
{
    if (activeStory.has_value()) return true;
    return false;
}

void Fernanda::domMove(QString pivotKey, QString fulcrumKey, Io::Move position)
{
    auto& story = activeStory.value();
    story.move(pivotKey, fulcrumKey, position);
    sendItems(story.items());
}

void Fernanda::domAdd(QString newName, Path::Type type, QString parentKey)
{
    auto& story = activeStory.value();
    story.add(newName, type, parentKey);
    sendItems(story.items());
    editor->textChanged();
}

void Fernanda::domRename(QString newName, QString key)
{
    auto& story = activeStory.value();
    story.rename(newName, key);
    sendItems(story.items());
}

void Fernanda::domCut(QString key)
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

// fernanda.cpp, Fernanda
