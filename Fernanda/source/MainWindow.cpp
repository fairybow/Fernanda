#include "MainWindow.h"

MainWindow::MainWindow(const char* name, bool isDev, StdFsPath file, QWidget* parent)
	: Widget(name, parent), m_isDev(isDev)
{
	setupWidgets();
	connections();
	loadConfigs();

	openNewTab();
}

void MainWindow::onSecondLaunch()
{
	return;

	// allow in the future, along with moving tabs out into new windows
	auto new_main_window = spawn();
	auto x = new_main_window->x();
	auto y = new_main_window->y();
	auto offset = style()->pixelMetric(QStyle::PM_TitleBarHeight);
	new_main_window->move(x + offset, y + offset);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	QMessageBox::information(this,
		"Test Dialog", StringTools::flank("Closing", 30));

	//

	auto state = windowState();
	showNormal();
	// return if canceled
	closeEventConfigs(state);
	event->accept();
}

void MainWindow::moveEvent(QMoveEvent* event)
{
	QMainWindow::moveEvent(event);
	saveGeometry();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	saveGeometry();
}

void MainWindow::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);
	if (m_isInitialized || event->spontaneous()) return;
	m_indicator->pastel(1500);
	m_isInitialized = true;
}

MainWindow* MainWindow::spawn()
{
	auto spawn = new MainWindow(objectName().toLocal8Bit(), m_isDev);
	spawn->setAttribute(Qt::WA_DeleteOnClose);
	spawn->show();
	return spawn;
}

void MainWindow::setupWidgets()
{
	setMenuBar(m_menuBar);
	setStatusBar(m_statusBar);
	m_statusBar->addWidgets({ m_meter }, { m_pomodoroTimer, m_stayAwake, m_alwaysOnTop });
	m_menuBar->makeSubmenus();
	auto layout = Layout::box(Layout::Line::Vertically);
	layout->addWidget(m_tabBar);
	layout->addLayout(Layout::stack({ m_editor, m_indicator }));
	auto container = Layout::container(layout);
	setCentralWidget(container);
}

void MainWindow::connections()
{
	docsManagerConnections();
	tabBarConnections();
	editorConnections();
	meterConnections();
	//previewerConnections();
	menuBarConnections();
	menuBarStyleConfigConnections();
	menuBarEditorConfigConnections();
	menuBarMeterConfigConnections();
	menuBarToolConfigConnections();
	menuBarMiscConfigConnections();
	menuBarDevConnections();
}

void MainWindow::docsManagerConnections()
{
	//
}

void MainWindow::tabBarConnections()
{
	connect(m_tabBar, &TabBar::currentChanged, this, [&](const QUuid& id) {
		qDebug() << "TabBar::currentChanged emitted" << id << Qt::endl;
		onTabServe(id);
		});
	connect(m_tabBar, &TabBar::askAdd, this, &MainWindow::onAddTabClick);
	connect(m_tabBar, &TabBar::askClearForClose, this, &MainWindow::onCloseTabClick);

	connect(m_editor, &Editor::textChanged, this, [&] {
		if (!m_tabBar->isUntitled()) return;
		auto block = m_editor->firstBlock();
		m_tabBar->setUntitledDisplay(block);
		});

	connect(m_editor, &Editor::textChanged, this, [&] {
		auto text_length = m_editor->toPlainText().length();
		m_tabFlagCheckDelayer->delayedEmit(text_length);
		});
	connect(m_tabFlagCheckDelayer, &Delayer::signal, this, [&] {
		if (updateActiveDocRecord()) {
			auto document = m_docsManager->active();
			m_tabBar->updateEditedState(document->data().toUuid(), document->isEdited());
		}
		});
}

void MainWindow::editorConnections()
{
	//
}

void MainWindow::meterConnections()
{
	connect(m_meter, &Meter::askGiveCounts, this, [&](bool isSelection) {
		isSelection
			? m_meter->give(Meter::Counts{ m_editor->selectedText(), m_editor->selectedLineCount() })
			: m_meter->give(Meter::Counts{ m_editor->toPlainText(), m_editor->blockCount() });
		});
	connect(m_meter, &Meter::askGivePositions, this, [&] {
		auto positions = Meter::Positions{ m_editor->cursorBlockNumber(), m_editor->cursorPositionInBlock() };
		m_meter->give(positions);
		});

	connect(m_meter, &Meter::editorFocusReturn, m_editor, [&] {
		m_editor->setFocus();
		});

	connect(m_editor, &Editor::selectionChanged, this, [&] {
		m_editor->hasSelection()
			? m_meter->trigger(Meter::Type::Selection, true)
			: m_meter->trigger(Meter::Type::Counts, true);
		});
	connect(m_editor, &Editor::textChanged, m_meter, [&] {
		m_meter->trigger(Meter::Type::Counts);
		});
	connect(m_editor, &Editor::cursorPositionChanged, m_meter, [&] {
		m_meter->trigger(Meter::Type::Positions);
		});
}

/*void MainWindow::previewerConnections()
{
	//
}*/

void MainWindow::menuBarConnections()
{
	connect(m_menuBar, &MenuBar::askOpenNewFile, this, [&] {
		auto path = m_docsManager->newFileDialog();
		openFileTab(path, DocsManager::PathType::New);
		});
	connect(m_menuBar, &MenuBar::askOpenFile, this, [&] {
		auto path = m_docsManager->openFileDialog();
		openFileTab(path);
		});
	connect(m_menuBar, &MenuBar::askSaveFile, this, &MainWindow::onSaveFile);
}

void MainWindow::menuBarStyleConfigConnections()
{
	connect(m_menuBar, &MenuBar::askToggleEditorTheme, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::HAS_EDITOR_THEME, m_editor, [&] {
				m_stylist->setThemeEnabled(m_editor, state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleWindowTheme, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::HAS_WINDOW_THEME, this, [&] {
				m_stylist->setThemeEnabled(this, state);
			});
		});

	connect(m_menuBar, &MenuBar::askStyleEditor, this, [&](StdFsPath path) {
		saveConfigPassthrough(
			Path::toQString(path), Ini::EDITOR_THEME, m_editor, [&] {
				m_stylist->style(m_editor, path);
			});
		});

	connect(m_menuBar, &MenuBar::askStyleWindow, this, [&](StdFsPath path) {
		saveConfigPassthrough(
			Path::toQString(path), Ini::WINDOW_THEME, this, [&] {
				m_stylist->style(this, path);
			});
		});
}

void MainWindow::menuBarEditorConfigConnections()
{
	connect(m_menuBar, &MenuBar::askChangeFont, this, [&](const QFont& font) {
		saveConfigPassthrough(
			font, Ini::EDITOR_FONT, m_editor, [&] {
				setUserFont(font);
			});
		});

	connect(m_menuBar, &MenuBar::askSetTabStop, this, [&](int pixels) {
		saveConfigPassthrough(
			pixels, Ini::EDITOR_TAB_STOP, m_editor, [&] {
				m_editor->setTabStopDistance(pixels);
			});
		});

	connect(m_menuBar, &MenuBar::askSetWrapMode, this, [&](const QString& mode) {
		saveConfigPassthrough(
			mode, Ini::EDITOR_WRAP_MODE, m_editor, [&] {
				m_editor->setWrapMode(mode);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleLineHighlight, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::EDITOR_LINE_HIHGLIGHT, m_editor, [&] {
				m_editor->setHasLineHighlight(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleLineNumbers, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::EDITOR_LINE_NUMBER_AREA, m_editor, [&] {
				m_editor->setHasLineNumberArea(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleShadow, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::EDITOR_SHADOW, m_editor, [&] {
				m_editor->setHasShadow(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleBlink, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::CURSOR_BLINK, m_editor, [&] {
				m_editor->setHasCursorBlink(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleBlock, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::CURSOR_BLOCK, m_editor, [&] {
				m_editor->setHasCursorBlock(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleCenterOnScroll, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::CURSOR_CENTER_ON_SCROLL, m_editor, [&] {
				m_editor->setCenterOnScroll(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleEnsureVisible, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::CURSOR_ENSURE_VISIBLE, m_editor, [&] {
				m_editor->setHasCursorEnsureVisible(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleTypewriter, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::CURSOR_TYPEWRITER, m_editor, [&] {
				m_editor->setHasCursorTypewriter(state);
			});
		});
}

void MainWindow::menuBarMeterConfigConnections()
{
	// toggle entire meter

	connect(m_menuBar, &MenuBar::askToggleLinePosition, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::METER_LINE_POS, m_meter, [&] {
				m_meter->setHasLinePosition(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleColumnPosition, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::METER_COL_POS, m_meter, [&] {
				m_meter->setHasColumnPosition(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleLineCount, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::METER_LINE_COUNT, m_meter, [&] {
				m_meter->setHasLineCount(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleWordCount, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::METER_WORD_COUNT, m_meter, [&] {
				m_meter->setHasWordCount(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleCharacterCount, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::METER_CHAR_COUNT, m_meter, [&] {
				m_meter->setHasCharacterCount(state);
			});
		});
}

void MainWindow::menuBarToolConfigConnections()
{
	// toggle all tools

	connect(m_menuBar, &MenuBar::askTogglePomodoroTimer, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::TOOL_POMODORO, m_pomodoroTimer, [&] {
				m_pomodoroTimer->setVisible(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleStayAwake, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::TOOL_STAY_AWAKE, m_stayAwake, [&] {
				m_stayAwake->setVisible(state);
			});
		});

	connect(m_menuBar, &MenuBar::askToggleAlwaysOnTop, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::TOOL_ALWAYS_ON_TOP, m_alwaysOnTop, [&] {
				m_alwaysOnTop->setVisible(state);
			});
		});

	connect(m_menuBar, &MenuBar::askSetPomodoroTime, this, [&](int timeInSeconds) {
		saveConfigPassthrough(
			timeInSeconds, Ini::TOOL_POMO_INTERVAL, m_pomodoroTimer, [&] {
				m_pomodoroTimer->setCountdown(timeInSeconds);
			});
		});
}

void MainWindow::menuBarMiscConfigConnections()
{
	// to-do:
	//void askSetPreviewerType(const QString& type);

	connect(m_menuBar, &MenuBar::askToggleIndicator, this, [&](bool state) {
		saveConfigPassthrough(
			state, Ini::IDICATOR, m_indicator, [&] {
				m_indicator->setVisible(state);
			});
		});

	connect(m_menuBar, &MenuBar::askSetIndicatorAlignment, this, [&](const QString& alignment) {
		saveConfigPassthrough(
			alignment, Ini::IDICATOR_ALIGNMENT, m_indicator, [&] {
				m_indicator->setAlignment(alignment);
			});
		});
}

void MainWindow::menuBarDevConnections()
{
	connect(m_menuBar, &MenuBar::askOpenDocuments, this, [&] {
		openSystemFolder(m_user->documents());
		});
	connect(m_menuBar, &MenuBar::askOpenUserData, this, [&] {
		openSystemFolder(m_user->data());
		});
	connect(m_menuBar, &MenuBar::askOpenInstallation, this, [&] {
		openSystemFolder(Path::toStdFs(QCoreApplication::applicationDirPath()).parent_path());
		});
	connect(m_menuBar, &MenuBar::devOpenLogs, this, [&] {
		auto user_data = Path::toQString(m_user->data());
		QDirIterator it(user_data, { "*.log" }, QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			auto path = Path::toStdFs(it.filePath());
			openFileTab(path);
		}
		});
	connect(m_menuBar, &MenuBar::devDocsManager, this, [&] {
		m_docsManager->devClass();
		});
	connect(m_menuBar, &MenuBar::devDocsManagerCurrent, this, [&] {
		m_docsManager->devCurrentInfo();
		});
	connect(m_menuBar, &MenuBar::devDocsManagerBank, this, [&] {
		m_docsManager->devIdBank();
		});
	connect(m_menuBar, &MenuBar::devStylist, this, [&] {
		m_stylist->devClass();
		});
	connect(m_menuBar, &MenuBar::devStylistStyleSheets, this, [&] {
		m_stylist->devStyleSheets();
		});
	connect(m_menuBar, &MenuBar::devStylistUnstyle, this, [&] {
		m_stylist->unStyle();
		});
	connect(m_menuBar, &MenuBar::devTabBarCurrent, this, [&] {
		m_tabBar->devCurrentInfo();
		});
}

void MainWindow::loadConfigs()
{
	//auto state = loadConfig(WINDOW_STATE, this, Qt::WindowState::WindowNoState);
	//setWindowState(state); // behaves strangely, Windows issue I think
	auto geometry = loadConfig(Ini::WINDOW_GEOMETRY, this, QRect(0, 0, 1000, 600));
	setGeometry(geometry);
	loadEditorConfigs();
	//loadPreviewerConfigs();
	loadMenuBarStyleConfigs();
	loadMenuBarEditorConfigs();
	loadMenuBarMeterConfigs();
	loadMenuBarToolConfigs();
	loadMenuBarMiscConfigs();
}

void MainWindow::loadEditorConfigs()
{
	auto font = loadConfig<QFont>(Ini::EDITOR_FONT, m_editor, m_editor->defaulFont());
	setUserFont(font);
}

/*void MainWindow::loadPreviewerConfigs()
{
	//
}*/

void MainWindow::loadMenuBarStyleConfigs()
{
	loadConfigPassthrough<bool>(Ini::HAS_EDITOR_THEME, m_editor, [&](bool state) {
		m_stylist->setThemeEnabled(m_editor, state);
		m_menuBar->setCheckBoxEditorTheme(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::HAS_WINDOW_THEME, this, [&](bool state) {
		m_stylist->setThemeEnabled(this, state);
		m_menuBar->setCheckBoxWindowTheme(state);
		}, true);

	loadConfigPassthrough<QString>(Ini::EDITOR_THEME, m_editor, [&](QString theme) {
		auto fs_editor_theme = Path::toStdFs(theme);
		m_stylist->style(m_editor, fs_editor_theme);
		m_menuBar->setSelectedEditorTheme(fs_editor_theme);
		}, Path::toQString(m_menuBar->defaultEditorTheme()));

	loadConfigPassthrough<QString>(Ini::WINDOW_THEME, this, [&](QString theme) {
		auto fs_window_theme = Path::toStdFs(theme);
		m_stylist->style(this, fs_window_theme);
		m_menuBar->setSelectedWindowTheme(fs_window_theme);
		}, Path::toQString(m_menuBar->defaultWindowTheme()));
}

void MainWindow::loadMenuBarEditorConfigs()
{
	loadConfigPassthrough<int>(Ini::EDITOR_TAB_STOP, m_editor, [&](int pixels) {
		m_editor->setTabStopDistance(pixels);
		m_menuBar->setSelectedTabStop(pixels);
		}, m_editor->defaulTabStop());

	loadConfigPassthrough<QString>(Ini::EDITOR_WRAP_MODE, m_editor, [&](QString mode) {
		m_editor->setWrapMode(mode);
		m_menuBar->setSelectedWrapMode(mode);
		}, m_editor->defaultWrap());

	loadConfigPassthrough<bool>(Ini::EDITOR_LINE_HIHGLIGHT, m_editor, [&](bool state) {
		m_editor->setHasLineHighlight(state);
		m_menuBar->setCheckBoxLineHighlight(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::EDITOR_LINE_NUMBER_AREA, m_editor, [&](bool state) {
		m_editor->setHasLineNumberArea(state);
		m_menuBar->setCheckBoxLineNumbers(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::EDITOR_SHADOW, m_editor, [&](bool state) {
		m_editor->setHasShadow(state);
		m_menuBar->setCheckBoxShadow(state);
		}, false);

	loadConfigPassthrough<bool>(Ini::CURSOR_BLINK, m_editor, [&](bool state) {
		m_editor->setHasCursorBlink(state);
		m_menuBar->setCheckBoxBlink(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::CURSOR_BLOCK, m_editor, [&](bool state) {
		m_editor->setHasCursorBlock(state);
		m_menuBar->setCheckBoxBlock(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::CURSOR_CENTER_ON_SCROLL, m_editor, [&](bool state) {
		m_editor->setCenterOnScroll(state);
		m_menuBar->setCheckBoxCenterOnScroll(state);
		}, false);

	loadConfigPassthrough<bool>(Ini::CURSOR_ENSURE_VISIBLE, m_editor, [&](bool state) {
		m_editor->setHasCursorEnsureVisible(state);
		m_menuBar->setCheckBoxEnsureVisible(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::CURSOR_TYPEWRITER, m_editor, [&](bool state) {
		m_editor->setHasCursorTypewriter(state);
		m_menuBar->setCheckBoxTypewriter(state);
		}, false);
}

void MainWindow::loadMenuBarMeterConfigs()
{
	// load entire meter box

	loadConfigPassthrough<bool>(Ini::METER_LINE_POS, m_meter, [&](bool state) {
		m_meter->setHasLinePosition(state);
		m_menuBar->setCheckBoxLinePosition(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::METER_COL_POS, m_meter, [&](bool state) {
		m_meter->setHasColumnPosition(state);
		m_menuBar->setCheckBoxColumnPosition(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::METER_LINE_COUNT, m_meter, [&](bool state) {
		m_meter->setHasLineCount(state);
		m_menuBar->setCheckBoxLineCount(state);
		}, false);

	loadConfigPassthrough<bool>(Ini::METER_WORD_COUNT, m_meter, [&](bool state) {
		m_meter->setHasWordCount(state);
		m_menuBar->setCheckBoxWordCount(state);
		}, true);

	loadConfigPassthrough<bool>(Ini::METER_CHAR_COUNT, m_meter, [&](bool state) {
		m_meter->setHasCharacterCount(state);
		m_menuBar->setCheckBoxCharacterCount(state);
		}, false);
}

void MainWindow::loadMenuBarToolConfigs()
{
	// load entire tools box

	loadConfigPassthrough<bool>(Ini::TOOL_POMODORO, m_pomodoroTimer, [&](bool state) {
		m_pomodoroTimer->setVisible(state);
		m_menuBar->setCheckBoxPomodoroTimer(state);
		}, false);

	loadConfigPassthrough<bool>(Ini::TOOL_STAY_AWAKE, m_stayAwake, [&](bool state) {
		m_stayAwake->setVisible(state);
		m_menuBar->setCheckBoxStayAwake(state);
		}, false);

	loadConfigPassthrough<bool>(Ini::TOOL_ALWAYS_ON_TOP, m_alwaysOnTop, [&](bool state) {
		m_alwaysOnTop->setVisible(state);
		m_menuBar->setCheckBoxAlwaysOnTop(state);
		}, false);

	loadConfigPassthrough<int>(Ini::TOOL_POMO_INTERVAL, m_pomodoroTimer, [&](int timeInSeconds) {
		m_pomodoroTimer->setCountdown(timeInSeconds);
		m_menuBar->setSelectedPomodoroTime(timeInSeconds);
		}, m_pomodoroTimer->defaultInterval());
}

void MainWindow::loadMenuBarMiscConfigs()
{
	loadConfigPassthrough<bool>(Ini::IDICATOR, m_indicator, [&](bool state) {
		m_indicator->setVisible(state);
		m_menuBar->setCheckBoxIndicator(state);
		}, true);

	loadConfigPassthrough<QString>(Ini::IDICATOR_ALIGNMENT, m_indicator, [&](const QString& alignment) {
		m_indicator->setAlignment(alignment);
		m_menuBar->setSelectedIndicatorAlignment(alignment);
		}, QString("Top"));

	//void askSetPreviewerType(const QString& type);
}

void MainWindow::saveGeometry()
{
	saveConfigPassthrough(geometry(), Ini::WINDOW_GEOMETRY, this);
}

void MainWindow::closeEventConfigs(Qt::WindowStates priorState)
{
	saveConfigPassthrough(priorState, Ini::WINDOW_STATE, this);
	saveGeometry();
}

void MainWindow::setUserFont(const QFont& font)
{
	m_editor->setFont(font);
	m_menuBar->setUserFont(font);
}

MainWindow::PromptResult MainWindow::singleSavePrompt()
{
	return QMessageBox::question(
		this, "Hey!", "You have unsaved changes.",
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
}

void MainWindow::openSystemFolder(const StdFsPath& path)
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(Path::toQString(path)));
}

void MainWindow::openFileTab(const StdFsPath& path, DocsManager::PathType pathType)
{
	if (path.empty()) {
		m_indicator->red();
		return;
	}

	auto id = m_docsManager->fromDisk(pathType, path);
	auto title = m_docsManager->at(id)->title();
	m_tabBar->serve(id, title);
}

bool MainWindow::updateActiveDocRecord()
{
	if (!m_docsManager->hasActive()) return false;
	auto document = m_docsManager->active();
	document->setText(m_editor->toPlainText());
	document->setCursorSpan(m_editor->cursorPosition(), m_editor->cursorAnchor());
	return true;
}

void MainWindow::onTabServe(const QUuid& id)
{
	updateActiveDocRecord();
	auto incoming = m_docsManager->setActive(id);
	m_editor->setDocument(incoming);
}

void MainWindow::onAddTabClick()
{
	auto id = m_docsManager->newUnsaved();
	m_tabBar->serve(id);
}

void MainWindow::onCloseTabClick(const QUuid& id)
{
	if (m_docsManager->isActive(id))
		updateActiveDocRecord();

	if (m_docsManager->at(id)->isEdited()) {
		if (!m_docsManager->isActive(id))
			m_tabBar->serve(id);

		auto early_return = false;
		auto action = singleSavePrompt();
		switch (action) {
		case PromptResult::Save:
			onSaveFile();
			early_return = true;
			break;
		case PromptResult::Cancel:
			early_return = true;
			break;
		}
		if (early_return) return;
	}

	m_tabBar->close(id);
	m_docsManager->close(id);
	if (m_tabBar->isEmpty())
		openNewTab();
}

bool MainWindow::onSaveFile()
{
	if (!updateActiveDocRecord()
		|| !m_docsManager->active()->isEdited())
		return false;

	auto save_result = m_docsManager->toDisk();
	if (save_result) {
		auto document = m_docsManager->active();
		auto id = document->data().toUuid();
		auto path = m_docsManager->pathAt(id);
		m_tabBar->updateEditedState(id, document->isEdited());
		m_tabBar->updateTitle(id, Path::qStringName(path));
	}

	return m_indicator->onResult(save_result);
}
