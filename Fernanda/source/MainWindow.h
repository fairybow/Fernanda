#pragma once

#include "common/Delayer.hpp"
#include "common/Layout.hpp"
#include "common/Path.hpp"
#include "common/Widget.hpp"
#include "documents/DocumentsManager.h"
#include "editor/Editor.h"
#include "menu-bar/MenuBar.h"
//#include "Previewer/Previewer.hpp"
#include "tab-bar/TabBar.h"
#include "tools/AlwaysOnTop.hpp"
#include "tools/StayAwake.hpp"
#include "tools/PomodoroTimer.h"
#include "user/User.hpp"
#include "Indicator.hpp"
#include "IniKeys.hpp"
#include "Meter.h"
#include "StatusBar.hpp"
#include "Stylist.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDirIterator>
#include <QMainWindow>
#include <QMessageBox>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QString>
#include <QStyle>
#include <QUrl>
#include <QUuid>
#include <QVariant>

#include <filesystem>
#include <functional>

class MainWindow : public Widget<QMainWindow>
{
	Q_OBJECT

public:
	using StdFsPath = std::filesystem::path;
	using PromptResult = QMessageBox::StandardButton;

	MainWindow(const char* name, bool isDev = false, StdFsPath file = StdFsPath(), QWidget* parent = nullptr);

	StdFsPath userData() const { return m_user->data(); }

public slots:
	void onSecondLaunch();

protected:
	virtual void closeEvent(QCloseEvent* event) override;
	virtual void moveEvent(QMoveEvent* event) override;
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void showEvent(QShowEvent* event) override;

private:
	const bool m_isDev;
	bool m_isInitialized = false;

	User* m_user = new User(QCoreApplication::applicationName(), this);
	DocsManager* m_docsManager = new DocsManager(
		{ m_user->documents(), m_user->temp(), m_user->backup() },
		this, 99);
	//Project* m_project = new Project(this);
	MenuBar* m_menuBar = new MenuBar("MenuBar", m_user->data(), m_isDev);
	StatusBar* m_statusBar = new StatusBar("StatusBar");
	Indicator* m_indicator = new Indicator("Indicator");
	TabBar* m_tabBar = new TabBar("TabBar", 100, 200);
	Editor* m_editor = new Editor("Editor", QFont("mononoki", 14));
	//Previewer* m_previewer = new Previewer("Previewer");
	Meter* m_meter = new Meter("Meter");
	PomodoroTimer* m_pomodoroTimer = new PomodoroTimer(this);
	StayAwake* m_stayAwake = new StayAwake;
	AlwaysOnTop* m_alwaysOnTop = new AlwaysOnTop(this);
	Stylist* m_stylist = new Stylist({ this, m_editor }, this);
	Delayer* m_tabFlagCheckDelayer = new Delayer(this, 1000);

	MainWindow* spawn();
	void setupWidgets();
	void connections();
	void docsManagerConnections();
	void tabBarConnections();
	void editorConnections();
	void meterConnections();
	//void previewerConnections();
	void menuBarConnections();
	void menuBarStyleConfigConnections();
	void menuBarEditorConfigConnections();
	void menuBarMeterConfigConnections();
	void menuBarToolConfigConnections();
	void menuBarMiscConfigConnections();
	void menuBarDevConnections();
	void loadConfigs();
	void loadEditorConfigs();
	//void loadPreviewerConfigs();
	void loadMenuBarStyleConfigs();
	void loadMenuBarEditorConfigs();
	void loadMenuBarMeterConfigs();
	void loadMenuBarToolConfigs();
	void loadMenuBarMiscConfigs();
	void saveGeometry();
	void closeEventConfigs(Qt::WindowStates priorState);
	void setUserFont(const QFont& font);
	PromptResult singleSavePrompt();
	void openSystemFolder(const StdFsPath& path);
	void openFileTab(const StdFsPath& path, DocsManager::PathType pathType = DocsManager::PathType::Extant);
	bool updateActiveDocRecord();

	void openNewTab() { onAddTabClick(); };

	template<typename T>
	void saveConfigPassthrough(T value, const QString& valueKey, QObject* associatedObject,
		std::function<void()> configurableAction = nullptr)
	{
		if (configurableAction)
			configurableAction();
		m_user->save(value, valueKey, associatedObject);
	}

	template<typename T>
	void loadConfigPassthrough(const QString& valueKey, QObject* associatedObject,
		std::function<void(T)> configurableAction, T fallbackValue = T())
	{
		auto value = m_user->load<T>(valueKey, associatedObject, fallbackValue);
		configurableAction(value);
	}

	template<typename T>
	T loadConfig(const QString& valueKey, QObject* associatedObject, T fallbackValue = T())
	{
		return m_user->load<T>(valueKey, associatedObject, fallbackValue);
	}

private slots:
	void onTabServe(const QUuid& id);
	void onAddTabClick();
	void onCloseTabClick(const QUuid& id);
	bool onSaveFile();
};
