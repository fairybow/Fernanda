// fernanda.cpp, Fernanda

#include "fernanda.h"

Fernanda::Fernanda(bool dev, FsPath story, QWidget* parent)
    : QMainWindow(parent)
{
    Ud::setName(name(dev));
    addWidgets();
    connections();
    shortcuts();
    Ud::userData();
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
    Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Position, geometry());
}

void Fernanda::moveEvent(QMoveEvent* event)
{
    QMainWindow::moveEvent(event);
    Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Position, geometry());
}

void Fernanda::closeEvent(QCloseEvent* event)
{
    auto state = windowState();
    Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::State, state.toInt());
    setWindowState(Qt::WindowState::WindowActive);
    auto quit = confirmStoryClose(true);
    if (!quit)
    {
        setWindowState(state);
        event->ignore();
        colorBar->run(ColorBar::Run::Green);
        return;
    }
    Ud::clear(Ud::userData(Ud::Op::GetTemp), true);
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

void Fernanda::openLocalFolder(FsPath path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(Path::toQString(path)));
}

const QStringList Fernanda::devPrintRenames(QVector<Io::ArcRename> renames)
{
    QStringList result;
    auto i = 0;
    for (auto& rename : renames)
    {
        ++i;
        QString entry = QString::number(i) + "\nKey: " + rename.key + "\nRel Path: " + Path::toQString(rename.relPath);
        (rename.origRelPath.has_value())
            ? entry = entry + "\nOrig Path: " + Path::toQString(rename.origRelPath.value())
            : entry = entry + "\nNew: " + QString((rename.typeIfNewOrCut.value() == Path::Type::Dir) ? "dir" : "file");
        result << entry;
    }
    return result;
}

const QString Fernanda::name(bool dev)
{
    QString result;
    if (dev)
        isDev = dev;
    (isDev)
        ? result = "Fernanda (dev)"
        : result = "Fernanda";
    return result;
}

void Fernanda::addWidgets()
{
    setCentralWidget(Layout::stackWidgets({ colorBar, splitter }));
    splitter->addWidgets({ pane, editor });
    statusBar->setSizeGripEnabled(true);
    setMenuBar(menuBar);
    setStatusBar(statusBar);
    aot->setCheckable(true);
    aot->setText(Icon::draw(Icon::Name::Pushpin));
    auto aot_effect = new QGraphicsOpacityEffect(this);
    aot_effect->setOpacity(0.8);
    aot->setGraphicsEffect(aot_effect);
    statusBar->addPermanentWidget(indicator, 0);
    statusBar->addPermanentWidget(spacer, 1);
    statusBar->addPermanentWidget(aot, 0);
    statusBar->setMaximumHeight(22);
    setObjectName(QStringLiteral("mainWindow"));
    menuBar->setObjectName(QStringLiteral("menuBar"));
    statusBar->setObjectName(QStringLiteral("statusBar"));
    fontSlider->setObjectName(QStringLiteral("fontSlider"));
    spacer->setObjectName(QStringLiteral("spacer"));
    aot->setObjectName(QStringLiteral("aot"));
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
    connect(aot, &QPushButton::toggled, this, &Fernanda::aotToggled);
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
    connect(this, &Fernanda::startAutoTempSave, this, [&]() { autoTempSave->start(30000); });
    connect(this, &Fernanda::askToggleStartUpBar, colorBar, [&](bool checked) { colorBar->toggle(checked, ColorBar::Has::RunOnStartUp); });
    connect(this, &Fernanda::askToggleScrolls, editor, [&](bool checked) { editor->toggle(checked, Editor::Has::Scrolls); });
    connect(autoTempSave, &QTimer::timeout, this, [&]()
        {
            activeStory.value().autoTempSave(editor->toPlainText());
        });
    connect(editor, &Editor::askNavNext, pane, [&]() { pane->nav(Pane::Nav::Next); });
    connect(editor, &Editor::askNavPrevious, pane, [&]() { pane->nav(Pane::Nav::Previous); });
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
    auto new_story = new QAction(tr("&New project..."), this);
    auto open_story = new QAction(tr("&Open an existing project..."), this);
    auto save = new QAction(tr("&Save"), this);
    auto quit = new QAction(tr("&Quit"), this);
    save->setShortcut(Qt::CTRL | Qt::Key_S);
    quit->setShortcut(Qt::CTRL | Qt::Key_Q);
    for (const auto& action : { save, quit })
        action->setAutoRepeat(false);
    connect(new_story, &QAction::triggered, this, [&]()
        {
            auto file_name = QFileDialog::getSaveFileName(this, tr("Create a new story..."), Path::toQString(Ud::userData(Ud::Op::GetDocs)), tr("Fernanda story file (*.story)"));
            openStory(Path::toFs(file_name));
        });
    connect(open_story, &QAction::triggered, this, [&]()
        {
            auto file_name = QFileDialog::getOpenFileName(this, tr("Open an existing story..."), Path::toQString(Ud::userData(Ud::Op::GetDocs)), tr("Fernanda story file (*.story)"));
            openStory(Path::toFs(file_name));
        });
    connect(save, &QAction::triggered, this, &Fernanda::fileMenuSave);
    connect(quit, &QAction::triggered, this, &QCoreApplication::quit, Qt::QueuedConnection);
    auto file = menuBar->addMenu(tr("&File"));
    for (const auto& action : { new_story, open_story })
        file->addAction(action);
    file->addSeparator();
    file->addAction(save);
    file->addSeparator();
    file->addAction(quit);
}

void Fernanda::makeStoryMenu()
{
    auto item1 = new QAction(tr("&(Empty)"), this);
    connect(item1, &QAction::triggered, this, [&]() {});
    item1->setEnabled(false);
    auto story = menuBar->addMenu(tr("&Story"));
    story->addAction(item1);
    story->menuAction()->setVisible(false);
    connect(this, &Fernanda::storyMenuVisible, story->menuAction(), &QAction::setVisible);
}

void Fernanda::makeSetMenu()
{
    auto user_data = Ud::userData(Ud::Op::GetUserData);
    QVector<Res::DataPair> bar_alignments = {
        Res::DataPair{ "Top", "Top" },
        Res::DataPair{ "Bottom", "Bottom" }
    };
    auto win_theme_list = Res::iterateResources(":/themes/window/", { "*.fernanda_wintheme" }, user_data);
    auto font_list = Res::iterateResources(":/fonts/", { "*.otf", "*.ttf" }, user_data);
    auto editor_theme_list = Res::iterateResources(":/themes/editor/", { "*.fernanda_theme" }, user_data);
    QVector<Res::DataPair> tab_list = {
        Res::DataPair{ "20", "20 px" },
        Res::DataPair{ "40", "40 px" },
        Res::DataPair{ "60", "60 px" },
        Res::DataPair{ "80", "80 px" }
    };
    QVector<Res::DataPair> wrap_list = {
        Res::DataPair{ "NoWrap", "No wrap" },
        Res::DataPair{ "WordWrap", "Wrap at word boundaries" },
        Res::DataPair{ "WrapAnywhere", "Wrap anywhere" },
        Res::DataPair{ "WrapAt", "Wrap at word boundaries or anywhere" }
    };
    barAlignments = makeViewToggles(bar_alignments, [&]() { askSetBarAlignment(getSetting<QString>(barAlignments)); });
    auto toggle_col_pos = new QAction(tr("&Column position"), this);
    auto toggle_line_pos = new QAction(tr("&Line position"), this);
    auto toggle_char_count = new QAction(tr("&Character count"), this);
    auto toggle_line_count = new QAction(tr("&Line count"), this);
    auto toggle_word_count = new QAction(tr("&Word count"), this);
    windowThemes = makeViewToggles(win_theme_list, &Fernanda::setStyle);
    editorFonts = makeViewToggles(font_list, [&]()
        {
            editor->handleFont(editorFonts->checkedAction(), fontSlider->value());
        });
    auto font_size_label = new QAction(tr("&Editor font size:"), this);
    auto font_size = new QWidgetAction(this);
    font_size->setDefaultWidget(fontSlider);
    fontSlider->setMinimum(8);
    fontSlider->setMaximum(40);
    editorThemes = makeViewToggles(editor_theme_list, [&]() { editor->setStyle(editorThemes->checkedAction()); });
    tabStops = makeViewToggles(tab_list, [&]() { sendSetTabStop(getSetting<int>(tabStops)); });
    wrapModes = makeViewToggles(wrap_list, [&]() { sendSetWrapMode(getSetting<QString>(wrapModes)); });
    connect(toggle_col_pos, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::ColPos); });
    connect(toggle_line_pos, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::LinePos); });
    connect(toggle_char_count, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::CharCount); });
    connect(toggle_line_count, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::LineCount); });
    connect(toggle_word_count, &QAction::toggled, this, [&](bool checked) { indicator->toggle(checked, Indicator::Has::WordCount); });
    connect(fontSlider, &QSlider::valueChanged, this, [&](int value) { editor->handleFont(editorFonts->checkedAction(), value); });
    for (const auto& action : {
        toggle_col_pos,
        toggle_line_pos,
        toggle_char_count,
        toggle_line_count,
        toggle_word_count
        })
        action->setCheckable(true);
    font_size_label->setEnabled(false);
    loadViewConfig(barAlignments->actions(), Ud::ConfigGroup::Window, Ud::ConfigVal::BarAlign, "Top");
    loadMenuToggle(toggle_col_pos, Ud::ConfigGroup::Window, Ud::ConfigVal::PosCol, true);
    loadMenuToggle(toggle_line_pos, Ud::ConfigGroup::Window, Ud::ConfigVal::PosLine, true);
    loadMenuToggle(toggle_char_count, Ud::ConfigGroup::Window, Ud::ConfigVal::CountChar, false);
    loadMenuToggle(toggle_line_count, Ud::ConfigGroup::Window, Ud::ConfigVal::CountLine, true);
    loadMenuToggle(toggle_word_count, Ud::ConfigGroup::Window, Ud::ConfigVal::CountWord, true);
    loadViewConfig(windowThemes->actions(), Ud::ConfigGroup::Window, Ud::ConfigVal::WinTheme, ":/themes/window/Light.fernanda_wintheme");
    fontSlider->setValue(Ud::loadConfig(Ud::ConfigGroup::Editor, Ud::ConfigVal::FontSlider, 16, Ud::Type::Int).toInt());
    loadViewConfig(editorFonts->actions(), Ud::ConfigGroup::Editor, Ud::ConfigVal::Font, ":/fonts/Cascadia Mono.ttf");
    loadViewConfig(editorThemes->actions(), Ud::ConfigGroup::Editor, Ud::ConfigVal::EditorTheme, ":/themes/editor/Amber.fernanda_theme");
    loadViewConfig(tabStops->actions(), Ud::ConfigGroup::Editor, Ud::ConfigVal::TabStop, "40");
    loadViewConfig(wrapModes->actions(), Ud::ConfigGroup::Editor, Ud::ConfigVal::Wrap, "WrapAt");
    auto set = menuBar->addMenu(tr("&Set"));
    auto bar_alignment = set->addMenu(tr("&Color bar alignment"));
    bar_alignment->addActions(barAlignments->actions());
    auto indicator_items = set->addMenu(tr("&Indicator"));
    for (const auto& action : { toggle_col_pos, toggle_line_pos })
        indicator_items->addAction(action);
    indicator_items->addSeparator();
    for (const auto& action : { toggle_char_count, toggle_line_count, toggle_word_count })
        indicator_items->addAction(action);
    auto window_themes = set->addMenu(tr("&Window theme"));
    window_themes->addActions(windowThemes->actions());
    set->addSeparator();
    auto fonts = set->addMenu(tr("&Editor font"));
    fonts->addActions(editorFonts->actions());
    set->addAction(font_size_label);
    set->addAction(font_size);
    auto editor_themes = set->addMenu(tr("&Editor theme"));
    editor_themes->addActions(editorThemes->actions());
    auto tab_stops = set->addMenu(tr("&Tab stop distance"));
    tab_stops->addActions(tabStops->actions());
    auto wrap_modes = set->addMenu(tr("&Wrap mode"));
    wrap_modes->addActions(wrapModes->actions());
}

void Fernanda::makeToggleMenu()
{
    auto toggle_aot = new QAction(tr("&Always-on-top button"), this);
    auto toggle_bar = new QAction(tr("&Color bar"), this);
    auto toggle_indicator = new QAction(tr("&Indicator"), this);
    auto toggle_pane = new QAction(tr("&Pane"), this);
    auto toggle_statusbar = new QAction(tr("&Status bar"), this);
    auto toggle_win_theme = new QAction(tr("&Window theme"), this);
    auto toggle_cursor_blink = new QAction(tr("&Blink"), this);
    auto toggle_block_cursor = new QAction(tr("&Block"), this);
    auto toggle_line_highlight = new QAction(tr("&Current line highlight"), this);
    auto toggle_shadow = new QAction(tr("&Editor shadow"), this);
    auto toggle_theme = new QAction(tr("&Editor theme"), this);
    auto toggle_keyfilter = new QAction(tr("&Key filters"), this);
    auto toggle_line_numbers = new QAction(tr("&Line number area"), this);
    auto toggle_scrolls = new QAction(tr("&Scrolls previous and next"), this);
    auto load_recent = new QAction(tr("&Load most recent project on open"), this);
    connect(toggle_aot, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(aot, Ud::ConfigGroup::Window, Ud::ConfigVal::T_AotBtn, checked);
        });
    connect(toggle_bar, &QAction::toggled, this, [&](bool checked) { colorBar->toggle(checked, ColorBar::Has::Self); });
    connect(toggle_indicator, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(indicator, Ud::ConfigGroup::Window, Ud::ConfigVal::T_Indicator, checked);
        });
    connect(toggle_pane, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(pane, Ud::ConfigGroup::Window, Ud::ConfigVal::T_Pane, checked);
        });
    connect(toggle_statusbar, &QAction::toggled, this, [&](bool checked)
        {
            toggleWidget(statusBar, Ud::ConfigGroup::Window, Ud::ConfigVal::T_StatusBar, checked);
        });
    connect(toggle_win_theme, &QAction::toggled, this, [&](bool checked)
        {
            hasTheme = checked;
            setStyle();
            Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::T_WinTheme, checked);
        });
    connect(toggle_cursor_blink, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::CursorBlink); });
    connect(toggle_block_cursor, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::BlockCursor); });
    connect(toggle_line_highlight, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::LineHighlight); });
    connect(toggle_shadow, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Shadow); });
    connect(toggle_theme, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Theme); });
    connect(toggle_keyfilter, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::Keyfilter); });
    connect(toggle_line_numbers, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::LineNumberArea); });
    connect(toggle_scrolls, &QAction::toggled, this, [&](bool checked) { editor->toggle(checked, Editor::Has::ExtraScrolls); });
    connect(load_recent, &QAction::toggled, this, [&](bool checked)
        {
            Ud::saveConfig(Ud::ConfigGroup::Data, Ud::ConfigVal::T_Lmr, checked); // move to story?
        });
    for (const auto& action : {
        toggle_aot,
        toggle_bar,
        toggle_indicator,
        toggle_pane,
        toggle_statusbar,
        toggle_win_theme,
        toggle_cursor_blink,
        toggle_block_cursor,
        toggle_line_highlight,
        toggle_shadow,
        toggle_theme,
        toggle_keyfilter,
        toggle_line_numbers,
        toggle_scrolls,
        load_recent
        })
        action->setCheckable(true);
    loadMenuToggle(toggle_aot, Ud::ConfigGroup::Window, Ud::ConfigVal::T_AotBtn, false);
    loadMenuToggle(toggle_bar, Ud::ConfigGroup::Window, Ud::ConfigVal::T_ColorBar, true);
    loadMenuToggle(toggle_indicator, Ud::ConfigGroup::Window, Ud::ConfigVal::T_Indicator, false);
    loadMenuToggle(toggle_pane, Ud::ConfigGroup::Window, Ud::ConfigVal::T_Pane, true);
    loadMenuToggle(toggle_statusbar, Ud::ConfigGroup::Window, Ud::ConfigVal::T_StatusBar, true);
    loadMenuToggle(toggle_win_theme, Ud::ConfigGroup::Window, Ud::ConfigVal::T_WinTheme, true);
    loadMenuToggle(toggle_cursor_blink, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_CursorBlink, true);
    loadMenuToggle(toggle_block_cursor, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Cursor, true);
    loadMenuToggle(toggle_line_highlight, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_LineHighlight, true);
    loadMenuToggle(toggle_shadow, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Shadow, false);
    loadMenuToggle(toggle_theme, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_EditorTheme, true);
    loadMenuToggle(toggle_keyfilter, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Keyfilter, true);
    loadMenuToggle(toggle_line_numbers, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Lna, false);
    loadMenuToggle(toggle_scrolls, Ud::ConfigGroup::Editor, Ud::ConfigVal::T_Nav, true);
    loadMenuToggle(load_recent, Ud::ConfigGroup::Data, Ud::ConfigVal::T_Lmr, false);
    auto toggle = menuBar->addMenu(tr("&Toggle"));
    for (const auto& action : { toggle_aot, toggle_bar, toggle_indicator, toggle_pane, toggle_statusbar, toggle_win_theme })
        toggle->addAction(action);
    toggle->addSeparator();
    auto cursor = toggle->addMenu(tr("&Cursor"));
    for (const auto& action : { toggle_cursor_blink, toggle_block_cursor })
        cursor->addAction(action);
    for (const auto& action : { toggle_line_highlight, toggle_shadow, toggle_theme, toggle_keyfilter, toggle_line_numbers, toggle_scrolls })
        toggle->addAction(action);
    toggle->addSeparator();
    toggle->addAction(load_recent);
}

void Fernanda::makeHelpMenu()
{
    auto about = new QAction(tr("&About..."), this);
    auto check_update = new QAction(tr("&Check for updates..."), this);
    auto shortcuts = new QAction(tr("&Shortcuts..."), this);
    auto open_docs = new QAction(tr("&Documents..."), this);
    auto open_install = new QAction(tr("&Installation folder..."), this);
    auto open_ud = new QAction(tr("&User data..."), this);
    auto sample_project = new QAction(tr("&Create sample project"), this);
    auto sample_themes = new QAction(tr("&Create sample themes..."), this);
    connect(about, &QAction::triggered, this, [&]() { Popup::about(this); });
    connect(check_update, &QAction::triggered, this, &Fernanda::helpMenuUpdate);
    connect(shortcuts, &QAction::triggered, this, [&]() { Popup::shortcuts(); });
    connect(open_docs, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Ud::userData(Ud::Op::GetDocs));
        });
    connect(open_install, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Path::toFs(QCoreApplication::applicationDirPath()).parent_path());
        });
    connect(open_ud, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Ud::userData(Ud::Op::GetUserData));
        });
    connect(sample_project, &QAction::triggered, this, &Fernanda::helpMenuMakeSampleProject);
    connect(sample_themes, &QAction::triggered, this, &Fernanda::helpMenuMakeSampleRes);
    auto help = menuBar->addMenu(tr("&Help"));
    for (const auto& action : { about, check_update, shortcuts })
        help->addAction(action);
    help->addSeparator();
    auto open = help->addMenu(tr("&Open"));
    for (const auto& action : { open_docs, open_install, open_ud })
        open->addAction(action);
    help->addSeparator();
    for (const auto& action : { sample_project, sample_themes })
        help->addAction(action);
}

void Fernanda::makeDevMenu()
{
    auto print_cursors = new QAction(tr("&Print cursor positions"), this);
    auto print_cuts = new QAction(tr("&Print cuts"), this);
    auto print_dom = new QAction(tr("&Print DOM"), this);
    auto print_dom_initial = new QAction(tr("&Print DOM (Initial)"), this);
    auto print_edited_delegate = new QAction(tr("&Print edited keys (Delegate)"), this);
    auto print_edited_story = new QAction(tr("&Print edited keys (Story)"), this);
    auto print_renames = new QAction(tr("&Print renames"), this);
    auto open_docs = new QAction(tr("&Open documents..."), this);
    auto open_install = new QAction(tr("&Open installation folder..."), this);
    auto open_temps = new QAction(tr("&Open temps..."), this);
    auto open_ud = new QAction(tr("&Open user data..."), this);
    connect(print_cursors, &QAction::triggered, this, [&]()
        {
            devMenuWrite("__Cursor positions.txt", editor->devGetCursorPositions().join(Text::newLines()));
        });
    connect(print_cuts, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__Cuts.xml", activeStory.value().devGetDom(Dom::Doc::Cuts));
        });
    connect(print_dom, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__DOM.xml", activeStory.value().devGetDom());
        });
    connect(print_dom_initial, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            devMenuWrite("__DOM (Initial).xml", activeStory.value().devGetDom(Dom::Doc::Initial));
        });
    connect(print_edited_delegate, &QAction::triggered, this, [&]()
        {
            devMenuWrite("__Edited keys (Delegate).txt", pane->devGetEditedKeys().join(Text::newLines()));
        });
    connect(print_edited_story, &QAction::triggered, this, [&]()
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
    connect(open_docs, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Ud::userData(Ud::Op::GetDocs));
        });
    connect(open_install, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Path::toFs(QCoreApplication::applicationDirPath()).parent_path());
        });
    connect(open_temps, &QAction::triggered, this, [&]()
        {
            if (!activeStory.has_value()) return;
            openLocalFolder(activeStory.value().devGetActiveTemp());
        });
    connect(open_ud, &QAction::triggered, this, [&]()
        {
            openLocalFolder(Ud::userData(Ud::Op::GetUserData));
        });
    auto dev = menuBar->addMenu(tr("&Dev"));
    for (const auto& action : { print_cursors, print_cuts, print_dom, print_dom_initial, print_edited_delegate, print_edited_story, print_renames })
        dev->addAction(action);
    dev->addSeparator();
    for (const auto& action : { open_docs, open_install, open_temps, open_ud })
        dev->addAction(action);
}

void Fernanda::loadConfigs(FsPath story)
{
    loadWinConfigs();
    splitter->loadConfig(geometry());
    auto is_empty = story.empty();
    auto load_most_recent = Ud::loadConfig(Ud::ConfigGroup::Data, Ud::ConfigVal::T_Lmr, false, Ud::Type::Bool).toBool();
    if (load_most_recent || !is_empty)
        askToggleStartUpBar(false);
    if (!is_empty)
    {
        openStory(story);
        return;
    }
    if (!load_most_recent) return;
    auto project = Path::toFs(Ud::loadConfig(Ud::ConfigGroup::Data, Ud::ConfigVal::Project));
    if (!QFile(project).exists() || project.empty()) return;
    openStory(project);
}

void Fernanda::loadWinConfigs()
{
    auto geometry = Ud::loadConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Position, QRect(0, 0, 1000, 666), Ud::Type::QRect).toRect();
    setGeometry(geometry);
    auto win_state = Ud::loadConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::State).toInt();
    if (win_state == 1) setWindowState(Qt::WindowState::WindowMinimized);
    else if (win_state == 2) setWindowState(Qt::WindowState::WindowMaximized);
    else if (win_state == 4) setWindowState(Qt::WindowState::WindowFullScreen);
    auto checked = Ud::loadConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Aot, false).toBool();
    aot->setChecked(checked);
}

void Fernanda::loadViewConfig(QVector<QAction*> actions, Ud::ConfigGroup group, Ud::ConfigVal valueType, QVariant fallback)
{
    auto resource = Ud::loadConfig(group, valueType, fallback);
    for (auto& action : actions)
        if (Path::toFs(action->data()) == Path::toFs(resource))
        {
            action->setChecked(true);
            return;
        }
    for (auto& action : actions)
        if (Path::toFs(action->data()) == Path::toFs(fallback))
        {
            action->setChecked(true);
            return;
        }
    actions.first()->setChecked(true);
}

void Fernanda::loadMenuToggle(QAction* action, Ud::ConfigGroup group, Ud::ConfigVal valueType, QVariant fallback)
{
    auto toggle_state = Ud::loadConfig(group, valueType, fallback, Ud::Type::Bool).toBool();
    action->setChecked(!toggle_state); // whyyyyyyyyy
    action->setChecked(toggle_state);
}

void Fernanda::openStory(FsPath fileName, Story::Op opt)
{
    if (fileName.empty())
    {
        colorBar->run(ColorBar::Run::Red);
        return;
    }
    auto change = confirmStoryClose();
    if (!change) return;
    Ud::clear(Ud::userData(Ud::Op::GetTemp));
    activeStory = Story(fileName, opt);
    auto& story = activeStory.value();
    storyMenuVisible(true);
    askEditorClose(true);
    sendItems(story.items());
    colorBar->run(ColorBar::Run::Green);
    Ud::saveConfig(Ud::ConfigGroup::Data, Ud::ConfigVal::Project, Path::toQString(fileName));
}

void Fernanda::toggleWidget(QWidget* widget, Ud::ConfigGroup group, Ud::ConfigVal valueType, bool value)
{
    widget->setVisible(value);
    Ud::saveConfig(group, valueType, value);
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
        auto theme_path = Path::toFs(selection->data());
        auto window_style = Style::windowStyle(theme_path, hasTheme);
        setStyleSheet(window_style);
        askToggleScrolls(hasTheme);
        Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::WinTheme, Path::toQString(theme_path));
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

void Fernanda::aotToggled(bool checked)
{
    if (checked)
    {
        setWindowFlags(windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
        aot->setText(Icon::draw(Icon::Name::Balloon));
    }
    else
    {
        setWindowFlags(windowFlags() ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
        aot->setText(Icon::draw(Icon::Name::Pushpin));
    }
    show();
    Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::Aot, checked);
}

void Fernanda::fileMenuSave()
{
    if (!activeStory.has_value()) return;
    auto& story = activeStory.value();
    if (!story.hasChanges()) return;
    story.save(editor->toPlainText());
    Ud::clear(Ud::userData(Ud::Op::GetTemp));
    editor->textChanged();
    sendItems(story.items());
    colorBar->run(ColorBar::Run::Green);
}

void Fernanda::helpMenuMakeSampleProject()
{
    auto path = Ud::userData(Ud::Op::GetDocs) / "Candide.story";
    openStory(path, Story::Op::Sample);
}

void Fernanda::helpMenuMakeSampleRes()
{
    auto path = Ud::userData(Ud::Op::GetUserData);
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
    auto request = QNetworkRequest(QUrl(QStringLiteral("https://api.github.com/repos/fairybow/fernanda/releases")));
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
    auto docs = Ud::userData(Ud::Op::GetDocs);
    Io::writeFile(docs / name.toStdString(), value);
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

void Fernanda::domMove(QString pivotKey, QString fulcrumKey, Io::Move pos)
{
    auto& story = activeStory.value();
    story.move(pivotKey, fulcrumKey, pos);
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
