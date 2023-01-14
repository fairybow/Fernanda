// fernanda.h, Fernanda

#pragma once

#include "colorbar.h"
#include "editor.h"
#include "indicator.h"
#include "pane.h"
#include "popup.h"
#include "res.h"
#include "splitter.h"
#include "story.h"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMainWindow>
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

class Fernanda : public QMainWindow
{
    using FsPath = std::filesystem::path;

    Q_OBJECT

public:
    Fernanda(bool dev, FsPath story, QWidget* parent = nullptr);

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
    ColorBar* colorBar = new ColorBar(this);
    QActionGroup* windowThemes = new QActionGroup(this);
    QActionGroup* editorThemes = new QActionGroup(this);
    QActionGroup* editorFonts = new QActionGroup(this);
    QActionGroup* tabStops = new QActionGroup(this);
    QActionGroup* wrapModes = new QActionGroup(this);
    QActionGroup* barAlignments = new QActionGroup(this);
    QSlider* fontSlider = new QSlider(Qt::Horizontal);
    Indicator* indicator = new Indicator(this);
    QLabel* spacer = new QLabel(this);
    QPushButton* aot = new QPushButton(this);
    QTimer* autoTempSave = new QTimer(this);
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    std::optional<Story> activeStory;
    bool isDev = false;
    bool isInitialized = false;
    bool hasTheme = true;

    bool confirmStoryClose(bool isQuit = false);
    void openLocalFolder(FsPath path);
    const QStringList devPrintRenames(QVector<Io::ArcRename> renames);
    const QString name(bool dev = false);
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
    void loadConfigs(FsPath story);
    void loadWinConfigs();
    void loadViewConfig(QVector<QAction*> actions, Ud::ConfigGroup group, Ud::ConfigVal valueType, QVariant fallback);
    void loadMenuToggle(QAction* action, Ud::ConfigGroup group, Ud::ConfigVal valueType, QVariant fallback);
    void openStory(FsPath fileName, Story::Op opt = Story::Op::Normal);
    void toggleWidget(QWidget* widget, Ud::ConfigGroup group, Ud::ConfigVal valueType, bool value);

    template<typename T>
    inline QActionGroup* makeViewToggles(QVector<Res::DataPair>& dataLabelPairs, T slot)
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
    void aotToggled(bool checked);
    void fileMenuSave();
    void helpMenuMakeSampleProject();
    void helpMenuMakeSampleRes();
    void helpMenuUpdate();
    void devMenuWrite(QString name, QString value);
    void handleEditorOpen(QString key = nullptr);
    void sendEditedText();
    bool replyHasProject();
    void domMove(QString pivotKey, QString fulcrumKey, Io::Move pos);
    void domAdd(QString newName, Path::Type type, QString parentKey);
    void domRename(QString newName, QString key);
    void domCut(QString key);

signals:
    void askSetBarAlignment(QString alignment);
    bool askHasStartUpBar();
    void askToggleStartUpBar(bool checked);
    void askToggleScrolls(bool checked);
    void askUpdatePositions(const int cursorBlockNumber, const int cursorPosInBlock);
    void askUpdateCounts(const QString text, const int blockCount);
    void askUpdateSelection(const QString selectedText, const int lineCount);
    void askEditorClose(bool isFinal = false);
    void sendSetTabStop(int distance);
    void sendSetWrapMode(QString mode);
    void sendItems(QVector<QStandardItem*> items);
    void sendEditsList(QStringList editedFiles);
    void startAutoTempSave();
    void storyMenuVisible(bool setVisible);
};

// fernanda.h, Fernanda
