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

// MainWindow.h, Fernanda

#pragma once

#include "ColorBar.h"
#include "Editor.h"
#include "Indicator.h"
#include "Pane.h"
#include "Preview.h"
#include "Resource.h"
#include "Splitter.h"
#include "Story.h"
#include "Tool.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMap>
#include <QMenuBar>
#include <QMoveEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QShowEvent>
#include <QSlider>
#include <QStatusBar>
#include <QUrl>
#include <QWidgetAction>

class MainWindow : public QMainWindow
{
    using StdFsPath = std::filesystem::path;

    Q_OBJECT

public:
    MainWindow(bool isDev, StdFsPath story, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QMenuBar* menuBar = new QMenuBar(this);
    QStatusBar* statusBar = new QStatusBar(this);
    Splitter* splitter = new Splitter(this);
    Pane* pane = new Pane(this);
    Editor* editor = new Editor(this);
    Preview* preview = new Preview(this);
    ColorBar* colorBar = new ColorBar(this);
    QActionGroup* windowThemes = new QActionGroup(this);
    QActionGroup* editorThemes = new QActionGroup(this);
    QActionGroup* editorFonts = new QActionGroup(this);
    QActionGroup* tabStops = new QActionGroup(this);
    QActionGroup* wrapModes = new QActionGroup(this);
    QActionGroup* colorBarAlignments = new QActionGroup(this);
    QActionGroup* previewTypes = new QActionGroup(this);
    QActionGroup* timerValues = new QActionGroup(this);
    QSlider* fontSlider = new QSlider(Qt::Horizontal);
    Indicator* indicator = new Indicator(this);
    QLabel* spacer = new QLabel(this);
    Tool* alwaysOnTop = new Tool(Tool::Type::AlwaysOnTop, this);
    Tool* stayAwake = new Tool(Tool::Type::StayAwake, this);
    Tool* timer = new Tool(Tool::Type::Timer, this);
    QTimer* autoTempSave = new QTimer(this); // move to editor or story?
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    std::optional<Story> activeStory;
    bool isDev = false;
    bool isInitialized = false;
    bool hasTheme = true;

    bool confirmStoryClose(bool isQuit = false);
    const QStringList devPrintRenames(QVector<Io::ArchiveRename> renames);
    void addWidgets();
    void connections();
    void shortcuts();
    void makeMenuBar();
    void makeFileMenu();
    void makeStoryMenu();
    void makeSetMenu();
    void makeToggleMenu();
    void makeHelpMenu();
    void makeDevMenu();
    void loadConfigs(StdFsPath story);
    void loadWinConfigs();
    void loadViewConfig(QVector<QAction*> actions, UserData::IniGroup group, UserData::IniValue valueType, QVariant fallback);
    void loadMenuToggle(QAction* action, UserData::IniGroup group, UserData::IniValue valueType, QVariant fallback);
    void openStory(StdFsPath fileName, Story::Mode mode = Story::Mode::Normal);
    void toggleWidget(QWidget* widget, UserData::IniGroup group, UserData::IniValue valueType, bool value);
    void storyMenuFileExport(const char* caption, const char* extensionFilter, Story::To type);

    const QString name() { return QString(isDev ? "Fernanda (dev)" : "Fernanda"); }
    void openLocalFolder(StdFsPath path) { QDesktopServices::openUrl(QUrl::fromLocalFile(Path::toQString(path))); }

    template<typename T>
    inline QActionGroup* makeViewToggles(QVector<Resource::DataPair>& dataLabelPairs, T slot)
    {
        auto group = new QActionGroup(this);
        for (auto& pair : dataLabelPairs)
        {
            auto& data = pair.path;
            auto label = pair.label.toUtf8();
            auto action = new QAction(tr(label), this);
            action->setData(Path::toQString(data));
            connect(action, &QAction::toggled, this, slot);
            action->setCheckable(true);
            group->addAction(action);
        }
        group->setExclusive(true);
        return group;
    }

    template<typename T>
    inline T getSetting(QActionGroup* settingsGroup)
    {
        T result{};
        if constexpr (std::is_same<T, int>::value)
            result = -1;
        if constexpr (std::is_same<T, QString>::value)
            result = nullptr;
        if (auto selection = settingsGroup->checkedAction(); selection != nullptr)
        {
            if constexpr (std::is_same<T, int>::value)
                result = selection->data().toInt();
            if constexpr (std::is_same<T, QString>::value)
                result = selection->data().toString();
        }
        return result;
    }

private slots:
    void adjustTitle();
    void setStyle();
    void handleFontSlider(PlainTextEdit::Zoom direction);
    void fileMenuSave();
    void storyMenuTotals();
    void helpMenuMakeSampleProject();
    void helpMenuMakeSampleRes();
    void helpMenuUpdate();
    void handleEditorOpen(QString key = nullptr);
    void sendEditedText();
    void domMove(QString pivotKey, QString fulcrumKey, Io::Move position);
    void domAdd(QString newName, Path::Type type, QString parentKey);
    void domRename(QString newName, QString key);
    void domCut(QString key);

    void devMenuWrite(QString name, QString value) { Io::writeFile(UserData::doThis(UserData::Operation::GetDocuments) / name.toStdString(), value); }
    bool replyHasProject() { return activeStory.has_value(); }

signals:
    void askEditorClose(bool isFinal = false);
    bool askHasStartUpBar();
    void askPaneAdd(Path::Type type);
    void askSetBarAlignment(QString alignment);
    void askSetCountdown(int seconds);
    void askSetPreviewType(QString typeName);
    void askToggleScrolls(bool checked);
    void askToggleStartUpBar(bool checked);
    void sendEditsList(QStringList editedFiles);
    void sendItems(QVector<QStandardItem*> items);
    void sendSetTabStop(int distance);
    void sendSetWrapMode(QString mode);
    void startAutoTempSave();
    void storyMenuVisible(bool setVisible);    
};

// MainWindow.h, Fernanda
