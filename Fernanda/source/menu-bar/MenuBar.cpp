#include "MenuBar.h"

MenuBar::MenuBar(const char* name, StdFsPath userData, bool isDev, QWidget* parent)
	: Widget(name, parent),
	m_userData(userData),
	m_isDev(isDev)
{
	makeActionGroups();
	makeBespokeActionGroups();
}

void MenuBar::makeSubmenus()
{
	file();
	//project();
	view();
	help();
	if (m_isDev)
		dev();
}

void MenuBar::makeActionGroups()
{
	m_actionGroups[GROUP_EDITOR_THEMES] = ActionGroup::fromQrc(QRC_EDITOR,
		".fernanda_editor", m_userData, this, [&] {
			auto selection = selectedEditorTheme();
			if (selection == nullptr) return;
			emit askStyleEditor(Path::toStdFs(selection->data()));
		});

	m_actionGroups[GROUP_WINDOW_THEMES] = ActionGroup::fromQrc(QRC_MAIN_WINDOW,
		".fernanda_window", m_userData, this, [&] {
			auto selection = selectedWindowTheme();
			if (selection == nullptr) return;
			emit askStyleWindow(Path::toStdFs(selection->data()));
		});
}

void MenuBar::makeBespokeActionGroups()
{
	ActionGroup::BespokeList wrap_modes;
	wrap_modes << ActionGroup::bespoke("NoWrap", "No wrap");
	wrap_modes << ActionGroup::bespoke("WordWrap", "Wrap at word boundaries");
	wrap_modes << ActionGroup::bespoke("WrapAnywhere", "Wrap anywhere");
	wrap_modes << ActionGroup::bespoke("WrapAt", "Wrap at word boundaries or anywhere");
	m_actionGroups[GROUP_WRAPS] = ActionGroup::fromBespoke(wrap_modes, this, [&] {
		auto selection = selectedWrapMode();
		if (selection == nullptr) return;
		emit askSetWrapMode(selection->data().toString());
		});

	ActionGroup::BespokeList indicator_alignments;
	indicator_alignments << ActionGroup::bespoke("Top");
	indicator_alignments << ActionGroup::bespoke("Bottom");
	m_actionGroups[GROUP_INDICATOR_ALIGN] = ActionGroup::fromBespoke(indicator_alignments, this, [&] {
		auto selection = selectedIndicatorAlignment();
		if (selection == nullptr) return;
		emit askSetIndicatorAlignment(selection->data().toString());
		});

	ActionGroup::BespokeList previewer_types;
	previewer_types << ActionGroup::bespoke("Fountain");
	previewer_types << ActionGroup::bespoke("Markdown");
	m_actionGroups[GROUP_PREVIEWER] = ActionGroup::fromBespoke(previewer_types, this, [&] {
		auto selection = selectedPreviewerType();
		if (selection == nullptr) return;
		emit askSetPreviewerType(selection->data().toString());
		});
}

void MenuBar::file()
{
	auto new_file = new QAction(tr("&New..."), this);
	auto open = new QAction(tr("&Open..."), this);
	auto save = new QAction(tr("&Save"), this);
	auto quit = new QAction(tr("&Quit"), this);

	save->setShortcut(Qt::CTRL | Qt::Key_S);
	quit->setShortcut(Qt::CTRL | Qt::Key_Q);

	connect(new_file, &QAction::triggered, this, lambdaEmit(askOpenNewFile));
	connect(open, &QAction::triggered, this, lambdaEmit(askOpenFile));
	connect(save, &QAction::triggered, this, lambdaEmit(askSaveFile));
	connect(quit, &QAction::triggered,
		this, &QCoreApplication::quit, Qt::QueuedConnection);

	auto menu = addMenu(tr("&File"));
	for (const auto& action : { new_file, open, save, quit })
		menu->addAction(action);
}

void MenuBar::project()
{
	//
}

void MenuBar::view()
{
	auto appearance = new QAction(tr("&Appearance..."), this);
	connect(appearance, &QAction::triggered, this, &MenuBar::appearanceDialog);

	auto menu = addMenu(tr("&View"));
	for (const auto& action : { appearance })
		menu->addAction(action);
}

void MenuBar::help()
{
	auto about = new QAction(tr("&About..."), this);
	auto check_for_updates = new QAction(tr("&Check for updates..."), this);

	connect(about, &QAction::triggered, this, [&] {
		Popup::about(this);
		});
	connect(check_for_updates, &QAction::triggered, this, [&] {
		Popup::checkVersion(this);
		});

	auto menu = addMenu(tr("&Help"));
	for (const auto& action : { about, check_for_updates })
		menu->addAction(action);
	menu->addMenu(openLocalFolders());
}

void MenuBar::dev()
{
	auto open_logs = new QAction(tr("&Open log"), this);
	auto document_class = new QAction(tr("&Class info"), this);
	auto document_current = new QAction(tr("&Current document"), this);
	auto document_bank = new QAction(tr("&ID Bank"), this);
	auto stylist_class = new QAction(tr("&Class info"), this);
	auto stylist_stylesheets = new QAction(tr("&Style sheets"), this);
	auto stylist_unstyle = new QAction(tr("&Remove all styling"), this);
	auto tab_bar_current = new QAction(tr("&Current tab info"), this);

	connect(open_logs, &QAction::triggered, this, lambdaEmit(devOpenLogs));
	connect(document_class, &QAction::triggered, this, lambdaEmit(devDocsManager));
	connect(document_current, &QAction::triggered, this, lambdaEmit(devDocsManagerCurrent));
	connect(document_bank, &QAction::triggered, this, lambdaEmit(devDocsManagerBank));
	connect(stylist_class, &QAction::triggered, this, lambdaEmit(devStylist));
	connect(stylist_stylesheets, &QAction::triggered, this, lambdaEmit(devStylistStyleSheets));
	connect(stylist_unstyle, &QAction::triggered, this, lambdaEmit(devStylistUnstyle));
	connect(tab_bar_current, &QAction::triggered, this, lambdaEmit(devTabBarCurrent));

	auto menu = addMenu(tr("&Dev"));
	menu->addMenu(openLocalFolders());

	for (const auto& action : { open_logs })
		menu->addAction(action);

	auto document = menu->addMenu("&Document");
	for (const auto& action : { document_class, document_current, document_bank })
		document->addAction(action);

	auto stylist = menu->addMenu("&Stylist");	
	for (const auto& action : { stylist_class, stylist_stylesheets, stylist_unstyle })
		stylist->addAction(action);

	auto tab_bar = menu->addMenu("&Tab bar");
	for (const auto& action : { tab_bar_current })
		tab_bar->addAction(action);
}

void MenuBar::addActionsToBoxes(QComboBox* comboBox, ActionGroup* actionGroup)
{
	for (auto i = 0; i < actionGroup->actions().count(); ++i) {
		auto action = actionGroup->actions().at(i);
		comboBox->addItem(action->text(), QVariant::fromValue(action));
		if (action == actionGroup->checkedAction())
			comboBox->setCurrentIndex(i);
	}
}

void MenuBar::addFontDialog(QMdiArea* multiDocArea)
{
	auto dialog = multiDocArea->addSubWindow(fontDialog());
	dialog->setWindowFlags(Qt::FramelessWindowHint);
	dialog->showMaximized();
}

LiveFontDialog* MenuBar::fontDialog()
{
	auto dialog = new LiveFontDialog(m_userFont, this);
	dialog->setOptions(LiveFontDialog::NoButtons);
	connect(dialog, &LiveFontDialog::currentFontChanged, this, [&](const QFont& font) {
		emit askChangeFont(font);
		});
	return dialog;
}

QGroupBox* MenuBar::themesGroupBox()
{
	auto box = new QGroupBox(tr("Themes"));

	auto editor_theme_check = new QCheckBox;
	auto window_theme_check = new QCheckBox;
	auto editor_themes = new ComboBox;
	auto window_themes = new ComboBox;

	addActionsToBoxes(editor_themes, m_actionGroups[GROUP_EDITOR_THEMES]);
	addActionsToBoxes(window_themes, m_actionGroups[GROUP_WINDOW_THEMES]);
	for (auto& combo_box : { editor_themes, window_themes }) {
		connect(combo_box, &ComboBox::currentIndexChanged, this, [this, combo_box](int index) {
			auto action = combo_box->itemData(index).value<QAction*>();
			if (action == nullptr) return; // why is it ever, though
			action->trigger();
			});
	}

	editor_theme_check->setChecked(m_checkBoxStates[CHECK_BOX_EDITOR_THEME]);
	window_theme_check->setChecked(m_checkBoxStates[CHECK_BOX_WINDOW_THEME]);

	connect(editor_theme_check, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxEditorTheme(state);
		emit askToggleEditorTheme(state);
		});
	connect(window_theme_check, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxWindowTheme(state);
		emit askToggleWindowTheme(state);
		});

	auto labeled_editor_themes = Layout::labeledContainer(editor_themes, nullptr, "Editor");
	auto labeled_window_themes = Layout::labeledContainer(window_themes, nullptr, "Window");

	auto layout = Layout::grid(nullptr, box);
	layout->addWidget(editor_theme_check, 0, 0, 1, 1);
	layout->addWidget(labeled_editor_themes, 0, 1, 1, 11);
	layout->addWidget(window_theme_check, 0, 12, 1, 1);
	layout->addWidget(labeled_window_themes, 0, 13, 1, 11);
	Layout::setUniformSpacing(layout);
	return box;
}

QGroupBox* MenuBar::fontGroupBox()
{
	auto box = new QGroupBox(tr("Font"));

	auto mdi_area = new QMdiArea;
	addFontDialog(mdi_area);
	auto layout = Layout::box(Layout::Line::Horizontally, mdi_area, box);
	Layout::setUniformSpacing(layout);
	return box;
}

QGroupBox* MenuBar::editorGroupBox()
{
	auto box = new QGroupBox(tr("Editor"));

	auto wrap_modes = new ComboBox;
	addActionsToBoxes(wrap_modes, m_actionGroups[GROUP_WRAPS]);

	auto tab_stops_slider = new Slider("Slider", Qt::Horizontal, nullptr, "Tab stop distance", true, "pixels", 10);
	tab_stops_slider->setRange(1, 30);
	tab_stops_slider->setValue(m_sliderValues[SLIDER_TABS]);

	auto line_highlight = new QCheckBox("Line highlight");
	auto line_number_area = new QCheckBox("Line number area");
	auto shadow = new QCheckBox("Shadow");

	line_highlight->setChecked(m_checkBoxStates[CHECK_BOX_LINE_HIGHLIGHT]);
	line_number_area->setChecked(m_checkBoxStates[CHECK_BOX_LINE_NUMBERS]);
	shadow->setChecked(m_checkBoxStates[CHECK_BOX_SHADOW]);

	connect(wrap_modes, &ComboBox::currentIndexChanged, this, [&, wrap_modes](int index) {
		wrap_modes->itemData(index).value<QAction*>()->trigger();
		});
	connect(tab_stops_slider, &Slider::valueChanged, this, [&](int value) {
		setSelectedTabStop(value);
		emit askSetTabStop(value);
		});
	connect(line_highlight, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxLineHighlight(state);
		emit askToggleLineHighlight(state);
		});
	connect(line_number_area, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxLineNumbers(state);
		emit askToggleLineNumbers(state);
		});
	connect(shadow, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxShadow(state);
		emit askToggleShadow(state);
		});

	auto layout = Layout::box(Layout::Line::Vertically, { wrap_modes }, box);
	auto middle_layout = Layout::box(Layout::Line::Horizontally, { line_highlight, line_number_area, shadow });
	layout->addLayout(middle_layout);
	layout->addWidget(tab_stops_slider);
	layout->addWidget(cursorGroupBox());
	Layout::setUniformSpacing({ middle_layout, layout });
	return box;
}

QGroupBox* MenuBar::cursorGroupBox()
{
	auto box = new QGroupBox(tr("Cursor"));

	auto blink = new QCheckBox("Blink");
	auto block = new QCheckBox("Block");
	auto center_on_scroll = new QCheckBox("Center on scroll");
	auto ensure_visible = new QCheckBox("Ensure visible");
	auto typewriter = new QCheckBox("Typewriter");

	blink->setChecked(m_checkBoxStates[CHECK_BOX_BLINK]);
	block->setChecked(m_checkBoxStates[CHECK_BOX_BLOCK]);
	center_on_scroll->setChecked(m_checkBoxStates[CHECK_BOX_CENTER_ON_SCROLL]);
	ensure_visible->setChecked(m_checkBoxStates[CHECK_BOX_ENSURE_VISIBLE]);
	typewriter->setChecked(m_checkBoxStates[CHECK_BOX_TYPEWRITER]);

	connect(blink, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxBlink(state);
		emit askToggleBlink(state);
		});
	connect(block, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxBlock(state);
		emit askToggleBlock(state);
		});
	connect(center_on_scroll, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxCenterOnScroll(state);
		emit askToggleCenterOnScroll(state);
		});
	connect(ensure_visible, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxEnsureVisible(state);
		emit askToggleEnsureVisible(state);
		});
	connect(typewriter, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxTypewriter(state);
		emit askToggleTypewriter(state);
		});

	auto layout = Layout::box(Layout::Line::Horizontally, { blink, block, center_on_scroll, ensure_visible, typewriter }, box);
	Layout::setUniformSpacing(layout);
	return box;
}

QGroupBox* MenuBar::meterGroupBox()
{
	auto box = new QGroupBox(tr("Meter"));

	auto line_check_box = new QCheckBox("Line");
	auto column_check_box = new QCheckBox("Column");
	auto meter_separator = new QLabel(
		StringTools::flank("/", 4));
	meter_separator->setAlignment(Qt::AlignCenter);
	auto lines_check_box = new QCheckBox("Lines");
	auto words_check_box = new QCheckBox("Words");
	auto characters_check_box = new QCheckBox("Characters");

	line_check_box->setChecked(m_checkBoxStates[CHECK_BOX_LINE_POS]);
	column_check_box->setChecked(m_checkBoxStates[CHECK_BOX_COL_POS]);
	lines_check_box->setChecked(m_checkBoxStates[CHECK_BOX_LINES]);
	words_check_box->setChecked(m_checkBoxStates[CHECK_BOX_WORDS]);
	characters_check_box->setChecked(m_checkBoxStates[CHECK_BOX_CHARS]);

	auto positions_layout = Layout::box(Layout::Line::Horizontally, { line_check_box, column_check_box });
	auto counts_layout = Layout::box(Layout::Line::Horizontally, { lines_check_box, words_check_box, characters_check_box });
	auto layout = Layout::box(Layout::Line::Horizontally,
		nullptr, box);
	layout->addLayout(positions_layout, 0);
	layout->addWidget(meter_separator, 1);
	layout->addLayout(counts_layout, 0);

	connect(line_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxLinePosition(state);
		emit askToggleLinePosition(state);
		});
	connect(column_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxColumnPosition(state);
		emit askToggleColumnPosition(state);
		});
	connect(lines_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxLineCount(state);
		emit askToggleLineCount(state);
		});
	connect(words_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxWordCount(state);
		emit askToggleWordCount(state);
		});
	connect(characters_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxCharacterCount(state);
		emit askToggleCharacterCount(state);
		});

	Layout::setUniformSpacing({ positions_layout, counts_layout, layout });
	return box;
}

QGroupBox* MenuBar::toolsGroupBox()
{
	auto box = new QGroupBox(tr("Tools"));

	auto pomodoro_timer_check_box = new QCheckBox(
		QString(Emoji::TOMATO) + " Pomodoro timer");
	auto stay_awake_check_box = new QCheckBox(
		QString(Emoji::TEACUP) + " Stay awake");
	auto always_on_top_check_box = new QCheckBox(
		QString(Emoji::PUSHPIN) + " Always on top");
	auto check_boxes_layout = Layout::box(Layout::Line::Horizontally,
		{ pomodoro_timer_check_box, stay_awake_check_box, always_on_top_check_box });

	pomodoro_timer_check_box->setChecked(m_checkBoxStates[CHECK_BOX_POMODORO]);
	stay_awake_check_box->setChecked(m_checkBoxStates[CHECK_BOX_STAY_AWAKE]);
	always_on_top_check_box->setChecked(m_checkBoxStates[CHECK_BOX_ALWAYS_ON_TOP]);

	auto pomodoro_times_slider = new Slider("Slider", Qt::Horizontal, nullptr, "Pomodoro interval", true, "minutes");
	pomodoro_times_slider->setRange(1, 60);
	pomodoro_times_slider->setValue(m_sliderValues[SLIDER_POMODORO] / 60);

	connect(pomodoro_timer_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxPomodoroTimer(state);
		emit askTogglePomodoroTimer(state);
		});
	connect(stay_awake_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxStayAwake(state);
		emit askToggleStayAwake(state);
		});
	connect(always_on_top_check_box, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxAlwaysOnTop(state);
		emit askToggleAlwaysOnTop(state);
		});
	connect(pomodoro_times_slider, &Slider::valueChanged, this, [&](int value) {
		setSelectedPomodoroTime(value * 60);
		emit askSetPomodoroTime(value * 60);
		});

	auto layout = Layout::box(Layout::Line::Vertically, nullptr, box);
	layout->addLayout(check_boxes_layout);
	layout->addWidget(pomodoro_times_slider);
	Layout::setUniformSpacing({ layout, check_boxes_layout });
	return box;
}

QGroupBox* MenuBar::mixedGroupBox()
{
	auto box = new QGroupBox(tr(""));

	auto indicator_check = new QCheckBox;
	auto indicator_alignments = new ComboBox;
	addActionsToBoxes(indicator_alignments, m_actionGroups[GROUP_INDICATOR_ALIGN]);

	indicator_check->setChecked(m_checkBoxStates[CHECK_BOX_INDICATOR]);

	connect(indicator_check, &QCheckBox::stateChanged, this, [&](int state) {
		setCheckBoxIndicator(state);
		emit askToggleIndicator(state);
		});
	connect(indicator_alignments, &ComboBox::currentIndexChanged, this, [&, indicator_alignments](int index) {
		indicator_alignments->itemData(index).value<QAction*>()->trigger();
		});

	auto alignments_container = Layout::labeledContainer(indicator_alignments, nullptr, "Indicator alignment");

	auto layout = Layout::grid(nullptr, box);
	auto spacer_1 = new QWidget;
	auto spacer_2 = new QWidget;
	layout->addWidget(spacer_1, 0, 0, 1, 1);
	layout->addWidget(indicator_check, 0, 1, 1, 1);
	layout->addWidget(alignments_container, 0, 2, 1, 15);
	layout->addWidget(spacer_2, 0, 17, 1, 15);
	Layout::setUniformSpacing(layout);
	return box;
}

QMenu* MenuBar::openLocalFolders()
{
	auto open_documents = new QAction(tr("&Documents..."), this);
	auto open_user_data = new QAction(tr("&User data..."), this);
	auto open_installation = new QAction(tr("&Installation..."), this);

	connect(open_documents, &QAction::triggered, this, lambdaEmit(askOpenDocuments));
	connect(open_user_data, &QAction::triggered, this, lambdaEmit(askOpenUserData));
	connect(open_installation, &QAction::triggered, this, lambdaEmit(askOpenInstallation));

	auto menu = new QMenu("&Open folder", this);
	for (const auto& action : { open_documents, open_user_data, open_installation })
		menu->addAction(action);

	return menu;
}

void MenuBar::appearanceDialog() // split this dialog up into 2?
{
	QDialog dialog(this);

	auto full_layout = Layout::grid(nullptr, &dialog);
	full_layout->addWidget(themesGroupBox(), 0, 0, 1, 2);
	full_layout->addWidget(fontGroupBox(), 1, 0, 5, 2);
	full_layout->addWidget(editorGroupBox(), 0, 3, 3, 2);
	full_layout->addWidget(meterGroupBox(), 3, 3, 1, 2);
	full_layout->addWidget(toolsGroupBox(), 4, 3, 1, 2);
	full_layout->addWidget(mixedGroupBox(), 5, 3, 1, 2);

	dialog.setFixedSize(800, 500);
	Layout::setUniformSpacing(full_layout);
	dialog.exec();
}
