#pragma once

#include <functional>

#include <QApplication>
#include <QDockWidget> // May move to TreeView
#include <QFileSystemModel> // May move to TreeView
#include <QFont>
#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QStatusBar>
#include <QString>
#include <Qt>
#include <QVariant>

#include "Coco/Debug.h"
#include "Coco/Log.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"
#include "Coco/Utility.h"

#include "ColorBar.h"
#include "dialogs/Unsaved.h"
#include "EcosystemMenuManager.h"
#include "files/FileManager.h"
#include "files/IFile.h"
#include "files/IFileView.h"
#include "files/NoOpFile.h"
#include "files/NoOpFileView.h"
#include "files/TextFile.h"
#include "files/TextFileView.h"
#include "FileTypes.h"
#include "plain-text-edit/PlainTextEdit.h"
#include "settings/Ini.h"
#include "settings/Settings.h"
#include "settings/SettingsDialog.h"
#include "tab-view/TabView.h"
#include "Tr.h"
#include "TreeView.h"
#include "window/Window.h"
#include "window/WindowManager.h"
#include "WordCounter.h"

class Ecosystem : public QObject
{
    Q_OBJECT

public:
    explicit Ecosystem
    (
        const Coco::Path& root,
        const Coco::Path& configPath,
        const Coco::Path& fallbackConfigPath,
        QObject* parent = nullptr
    )
        : QObject(parent)
        , root_(root)
        , settings_(new Settings(configPath, fallbackConfigPath, this))
    {
        initialize_();
    }

    explicit Ecosystem
    (
        const Coco::Path& root,
        const Coco::Path& configPath,
        QObject* parent = nullptr
    )
        : Ecosystem(root, configPath, {}, parent)
    {
    }

    virtual ~Ecosystem() override { COCO_TRACER; }

private:
    void initialize_()
    {
        initializeWindowManager_();

        // Right now, call this here, as we may open files at some point (but
        // they may open after this ctor anyway, so who knows yet). Although, we
        // DO call this after creating new windows already, so maybe this
        // initializing can go into window manager initialization...
        menuManager_->initializeGlobalMenuToggles();
    }

    /// ======================================================================== ///
    /// *** SETTINGS ***                                                         ///
    /// ======================================================================== ///

private:
    Settings* settings_;
    QPointer<SettingsDialog> settingsDialog_ = nullptr;

    void applyEditorValues_(TextFileView* textFileView, const Ini::Editor::Values& values)
    {
        textFileView->setHasCenterOnScroll(values.centerOnScroll);
        textFileView->setHasOverwrite(values.overwrite);
        textFileView->setTabStopDistance(values.tabStopPx);
        textFileView->setWordWrapMode(values.wordWrapMode);
    }

    void applyEditorValuesToAllEditors_(const Ini::Editor::Values& values)
    {
        // This looks bad, I think...
        for (auto& window : windowManager_->windows())
            if (auto tab_view = tabViewIn_(window))
                for (auto i = 0; i < tab_view->count(); ++i)
                    if (auto text_file_view = to<TextFileView*>(fileViewAt_(tab_view, i)))
                        applyEditorValues_(text_file_view, values);
    }

    void applyFontToAllEditors_(const QFont& font)
    {
        // ^ ditto
        for (auto& window : windowManager_->windows())
            if (auto tab_view = tabViewIn_(window))
                for (auto i = 0; i < tab_view->count(); ++i)
                    if (auto text_file_view = to<TextFileView*>(fileViewAt_(tab_view, i)))
                        text_file_view->setFont(font);
    }

    void openSettingsDialog_()
    {
        if (settingsDialog_)
        {
            settingsDialog_->raise();
            settingsDialog_->activateWindow();
            return;
        }

        settingsDialog_ = new SettingsDialog(settings_);

        connect
        (
            settingsDialog_,
            &SettingsDialog::finished,
            this,
            [&](int result)
            {
                (void)result;
                if (settingsDialog_) settingsDialog_->deleteLater();
            }
        );

        connect
        (
            settingsDialog_,
            &SettingsDialog::editorValuesChanged,
            this,
            [&](const Ini::Editor::Values& values)
            {
                applyEditorValuesToAllEditors_(values);
            }
        );

        connect
        (
            settingsDialog_,
            &SettingsDialog::fontChanged,
            this,
            [&](const QFont& font)
            {
                applyFontToAllEditors_(font);
            }
        );

        settingsDialog_->open();
    }

    /// ======================================================================== ///
    /// *** WINDOW MANAGER ***                                                   ///
    /// ======================================================================== ///

signals:
    void lastWindowClosed();

private:
    WindowManager* windowManager_ = new WindowManager(this);

    void initializeWindowManager_()
    {
        windowManager_->setCloseAcceptor(this, &Ecosystem::onCloseAllInWindow_);
        connect(windowManager_, &WindowManager::activeWindowChanged, this, &Ecosystem::onWindowManagerActiveWindowChanged_);
        connect(windowManager_, &WindowManager::lastWindowClosed, this, &Ecosystem::onWindowManagerLastWindowClosed_);
    }

private slots:
    void onWindowManagerActiveWindowChanged_(Window* window)
    {
        (!window)
            ? COCO_LOG_THIS("No active window")
            : COCO_LOG_THIS(QString("Active window: %0").arg(COCO_PTR_QSTR(window)));
    }

    void onWindowManagerLastWindowClosed_()
    {
        COCO_LOG_THIS("Last window closed");
        emit lastWindowClosed();
    }

    /// ======================================================================== ///
    /// *** WINDOWS ***                                                          ///
    /// ======================================================================== ///

public:
    void openWindow(const QRect& geometry = {}, QList<Coco::Path> paths = {}, int activeIndex = -1)
    {
        // Currently unused, but the extra args are for Sessions
        auto window = newWindow_(geometry, paths, activeIndex);
        if (window) window->show();
    }

private:
    Window* newWindow_(const QRect& geometry = {}, QList<Coco::Path> paths = {}, int activeIndex = -1)
    {
        // Currently unused, but the extra args are for Sessions
        auto window = windowManager_->make(geometry);
        if (!window) return nullptr;

        initializeNewWindow_(window, paths, activeIndex);
        return window;
    }

    Window* newWindow_(const QPoint& pos, QList<Coco::Path> paths = {}, int activeIndex = -1)
    {
        auto window = windowManager_->make(pos);
        if (!window) return nullptr;

        initializeNewWindow_(window, paths, activeIndex);
        return window;
    }

    void initializeNewWindow_(Window* window, QList<Coco::Path> paths, int activeIndex)
    {
        menuManager_->initializeWindowMenus(window);
        initializeWindowStatusBar_(window);
        initializeWindowTabView_(window);
        //initializeWindowTreeView_(window);
        colorBars_[window] = new ColorBar(window); // ColorBar floats outside layouts

        initializeWindowCleanup_(window);

        // For sessions, later
        if (!paths.isEmpty())
        {
            // Open all
            // Set index
        }

        // Call this here since we will (later) be opening files AFTER menu initialization per window
        menuManager_->updateGlobalMenuToggles(); // For prev/next window
    }

    void initializeWindowCleanup_(Window* window)
    {
        if (!window) return;
        connect(window, &Window::destroyed, this, &Ecosystem::onWindowDestroyed_);
    }

private slots:
    void onWindowDestroyed_(Window* window)
    {
        if (!window) return;

        // Disconnect from active file if any
        if (auto cx = activeFileConnections_.take(window); !cx.isEmpty())
            for (auto& connection : cx)
                disconnect(connection);

        colorBars_.remove(window);
        treeViewDockWidgets_.remove(window);
        wordCounters_.remove(window);
        activeFileViews_.remove(window);

        menuManager_->cleanup(window);
        menuManager_->updateGlobalMenuToggles(); // For prev/next window
    }

    /// ======================================================================== ///
    /// *** COLOR BARS ***                                                       ///
    /// ======================================================================== ///

public:
    void beCute() const { runAllColorBars_(ColorBar::Pastel); }

private:
    QHash<Window*, ColorBar*> colorBars_{};

    void runColorBarFor_(Window* window, ColorBar::Color color) const
    {
        if (!window) return;

        if (auto color_bar = colorBars_[window])
            color_bar->run(color);
    }

    void runAllColorBars_(ColorBar::Color color) const
    {
        for (auto& color_bar : colorBars_)
            if (color_bar)
                color_bar->run(color);
    }

    void runColorBarOnSaveResult_(Window* window, IFile::Save result) const
    {
        switch (result)
        {
        default: case IFile::Save::NoOp: return;

        case IFile::Save::Success:
            runColorBarFor_(window, ColorBar::Green);
            return;
        case IFile::Save::Fail:
            runColorBarFor_(window, ColorBar::Red);
            return;
        }
    }

    void runAllColorBarsOnSaveResult_(IFile::Save result) const
    {
        switch (result)
        {
        default: case IFile::Save::NoOp: return;

        case IFile::Save::Success:
            runAllColorBars_(ColorBar::Green);
            return;
        case IFile::Save::Fail:
            runAllColorBars_(ColorBar::Red);
            return;
        }
    }

    /// ======================================================================== ///
    /// *** MENUS ***                                                            ///
    /// ======================================================================== ///

public:
    using MenuPopulator = std::function<void(QMenuBar*)>;
    MenuPopulator menuPopulator() const noexcept { return menuPopulator_; }
    void setMenuPopulator(const MenuPopulator& menuPopulator) { menuPopulator_ = menuPopulator; }

    template <typename ClassT>
    void setMenuPopulator(ClassT* object, void (ClassT::* method)(QMenuBar*))
    {
        menuPopulator_ = [object, method](QMenuBar* menuBar)
            {
                (object->*method)(menuBar);
            };
    }

private:
    MenuPopulator menuPopulator_ = nullptr;
    friend class EcosystemMenuManager;
    EcosystemMenuManager* menuManager_ = new EcosystemMenuManager(this);

    /// ======================================================================== ///
    /// *** STATUS BARS ***                                                      ///
    /// ======================================================================== ///

private:
    void initializeWindowStatusBar_(Window* window)
    {
        auto status_bar = new QStatusBar(window);

        auto word_counter = new WordCounter(window);
        wordCounters_[window] = word_counter;
        status_bar->addPermanentWidget(word_counter);

        window->setStatusBar(status_bar);
    }

    /// ======================================================================== ///
    /// *** TOOLS ***                                                            ///
    /// ======================================================================== ///

private:
    QHash<Window*, WordCounter*> wordCounters_{};

    void setWordCounterOnActiveView_(IFileView* fileView, Window* window)
    {
        if (!window) return;
        auto counter = wordCounters_[window];
        if (!counter) return;

        PlainTextEdit* text_edit = nullptr;

        // qobject_cast can take nullptr fileView
        if (auto text_file_view = qobject_cast<TextFileView*>(fileView))
            text_edit = text_file_view->widget<PlainTextEdit*>();

        counter->setTextEdit(text_edit);
    }

    /// ======================================================================== ///
    /// *** FILES ***                                                            ///
    /// ======================================================================== ///

public:
    using FileInterceptor = std::function<bool(const Coco::Path&)>;
    FileInterceptor fileInterceptor() const noexcept { return fileInterceptor_; }
    void setFileInterceptor(const FileInterceptor& fileInterceptor) { fileInterceptor_ = fileInterceptor; }

    template <typename ClassT>
    void setFileInterceptor(ClassT* object, bool (ClassT::* method)(const Coco::Path&))
    {
        fileInterceptor_ = [object, method](const Coco::Path& path)
            {
                return (object->*method)(path);
            };
    }

private:
    Coco::Path root_;

    FileInterceptor fileInterceptor_ = nullptr;
    FileManager* fileManager_ = new FileManager(this);
    QHash<Window*, QList<QMetaObject::Connection>> activeFileConnections_{};

    bool isMultiWindow_(IFile* file) const
    {
        if (!file) return false;

        QSet<Window*> unique_windows{};

        for (auto& view : fileManager_->viewsOn(file))
        {
            if (auto window = Coco::Utility::findParent<Window*>(view))
            {
                unique_windows << window;
                if (unique_windows.size() >= 2) return true;
            }
        }

        return false;
    }

    /// ======================================================================== ///
    /// *** NEW FILES ***                                                        ///
    /// ======================================================================== ///

private:
    void connectNewFile_(IFile* file)
    {
        if (!file) return;

        connect
        (
            file,
            &IFile::displayPropertiesChanged,
            this,
            [=]
            {
                for (auto& file_view : fileManager_->viewsOn(file))
                {
                    if (auto tab_view = Coco::Utility::findParent<TabView*>(file_view))
                    {
                        auto index = tab_view->indexOf(file_view);

                        if (index > -1)
                        {
                            tab_view->setTabText(index, file->title());
                            tab_view->setTabToolTip(index, file->toolTip());
                        }
                    }
                }
            }
        );

        if (!file->canEdit()) return;

        connect
        (
            file,
            &IFile::modificationChanged,
            this,
            [=](bool changed)
            {
                for (auto& file_view : fileManager_->viewsOn(file))
                {
                    if (auto tab_view = Coco::Utility::findParent<TabView*>(file_view))
                    {
                        auto index = tab_view->indexOf(file_view);
                        if (index > -1) tab_view->setTabFlagged(index, changed);
                    }
                }

                // This will toggle both when we save (making the file
                // unmodified) and modify
                menuManager_->updateGlobalMenuToggles();
            }
        );
    }

    // Todo: Eventually, we may want a config for only allowing file open in one
    //       view at a time
    // Note: This file is only ever a text file right now
    void openNewFile_(Window* window)
    {
        if (!window) return;

        auto tab_view = tabViewIn_(window);
        if (!tab_view) return;

        auto file = fileManager_->makeFile<TextFile*>();
        auto file_view = fileManager_->makeView<TextFileView*>(file, tab_view);
        if (!file || !file_view) return;

        addFileViewToTabView_(file_view, tab_view);
        connectNewFile_(file);

        menuManager_->updateGlobalMenuToggles();
    }

    // Todo: We'll need a method, later that just takes the path (for tree view
    //       openings) and wrap it with the call for a dialog
    void openExistingFile_(Window* window)
    {
        // Could be any kind of file
        if (!window) return;

        auto path = Coco::PathUtil::Dialog::file
        (
            window,
            Tr::Eco::openFile(),
            root_
        );

        if (path.isEmpty())
        {
            qWarning() << "No path selected!"; // Log instead?
            return;
        }

        if (!path.exists())
        {
            qWarning() << "Path doesn't exist: " << path; // Log instead?
            return;
        }

        // Allow Environment to handle certain files if set
        if (fileInterceptor_ && fileInterceptor_(path)) return;

        // Check if model already exists
        IFile* file = nullptr;
        IFileView* file_view = nullptr;
        auto tab_view = tabViewIn_(window);
        if (!tab_view) return;

        file = fileManager_->find(path);

        if (file)
        {
            // File exists (make a new view). Is this the best method, or do we
            // want to deduce from model type?
            switch (FileTypes::type(path))
            {
            case FileTypes::PlainText:
                file_view = fileManager_->makeView<TextFileView*>(file, tab_view);
                break;

            default:
                file_view = fileManager_->makeView<NoOpFileView*>(file, tab_view);
                break;
            }

            if (!file_view) return;
            addFileViewToTabView_(file_view, tab_view);
        }
        else // !file
        {
            switch (FileTypes::type(path))
            {
            case FileTypes::PlainText:
                file = fileManager_->makeFile<TextFile*>(path);
                file_view = fileManager_->makeView<TextFileView*>(file, tab_view);
                break;

            default:
                file = fileManager_->makeFile<NoOpFile*>(path);
                file_view = fileManager_->makeView<NoOpFileView*>(file, tab_view);
                break;
            }

            if (!file || !file_view) return;

            addFileViewToTabView_(file_view, tab_view);
            connectNewFile_(file);
        }

        menuManager_->updateGlobalMenuToggles();
    }

    // openExistingUnsaved (for restoring auto-saved unsaved/new files from
    // session)

    /// ======================================================================== ///
    /// *** FILE EDITING ***                                                     ///
    /// ======================================================================== ///

private:
    void undoOnActiveFile_(Window* window)
    {
        if (!window) return;

        if (auto file_view = activeFileViewIn_(window))
        {
            if (auto file = file_view->model())
            {
                file->undo();
                menuManager_->updateEditToggles(window);
            }
        }
    }

    void redoOnActiveFile_(Window* window)
    {
        if (!window) return;

        if (auto file_view = activeFileViewIn_(window))
        {
            if (auto file = file_view->model())
            {
                file->redo();
                menuManager_->updateEditToggles(window);
            }
        }
    }

    /// ======================================================================== ///
    /// *** FILE SAVING ***                                                      ///
    /// ======================================================================== ///

private:
    COCO_BOOL(SkipIfMultiWindow);

    bool anySavableFilesIn_(Window* window, SkipIfMultiWindow skip = SkipIfMultiWindow::No) const
    {
        auto tab_view = tabViewIn_(window);
        if (!tab_view) return false;

        for (auto i = 0; i < tab_view->count(); ++i)
        {
            auto file_view = fileViewAt_(tab_view, i);
            if (!file_view) continue;

            auto file = file_view->model();
            if (!file) continue;

            if (FileManager::isEdited(file))
            {
                if (skip && isMultiWindow_(file)) continue;
                return true;
            }
        }

        return false;
    }

    // Todo: Optimize? Maybe not needed, as this will almost always likely
    //       return very soon after finding the first one that qualifies, but
    //       IDK, it could be worth it
    bool anySavableFilesInAnyWindow_() const
    {
        for (auto& window : windowManager_->windows())
            if (window && anySavableFilesIn_(window))
                return true;

        return false;
    }

    /// RENAME?
    IFile::Save saveFileCascade_(IFile* file, Window* window)
    {
        if (file && window)
        {
            if (FileManager::isEditedOnDisk(file))
                return file->save();
            else if (FileManager::isEditedUnsaved(file))
                return saveFileAs_(file, window);
        }

        return IFile::Save::NoOp;
    }

    IFile::Save saveFileAs_(IFile* file, Window* window)
    {
        if (!file || !window || !FileManager::canSaveAs(file)) return IFile::Save::NoOp;

        auto path = Coco::PathUtil::Dialog::save
        (
            window,
            Tr::Eco::saveFileAs(),
            file->isOnDisk() ? file->path() : root_ / file->title() += file->preferredExt()
        );

        if (path.isEmpty()) return IFile::Save::NoOp;

        // Prevent conflicts by ensuring we don't save over any other open file
        // path
        auto extant_with_path = fileManager_->find(path);
        if (extant_with_path && extant_with_path != file)
        {
            qWarning() << "Attempting to overwrite an open file!"; // Log instead?
            return IFile::Save::NoOp;
        }

        return file->saveAs(path);
    }

    IFile::Save saveActiveFile_(Window* window)
    {
        if (window)
            if (auto file_view = activeFileViewIn_(window))
                if (auto file = file_view->model())
                    return saveFileCascade_(file, window);

        return IFile::Save::NoOp;
    }

    IFile::Save saveActiveFileAs_(Window* window)
    {
        if (window)
            if (auto file_view = activeFileViewIn_(window))
                if (auto file = file_view->model())
                    return saveFileAs_(file, window);

        return IFile::Save::NoOp;
    }

    IFile::Save saveAllInWindow_(Window* window, SkipIfMultiWindow skip = SkipIfMultiWindow::No)
    {
        if (!window) return IFile::Save::NoOp;

        auto tab_view = tabViewIn_(window);
        if (!tab_view) return IFile::Save::NoOp;

        auto any_fails = false;
        auto any_successes = false;

        for (auto i = 0; i < tab_view->count(); ++i)
        {
            auto file_view = fileViewAt_(tab_view, i);
            if (!file_view) continue;

            auto file = file_view->model();
            if (!file) continue;

            if (skip && isMultiWindow_(file)) continue;

            switch (saveFileCascade_(file, window))
            {
            case IFile::Save::Success:          any_successes = true; break;
            case IFile::Save::Fail:             any_fails = true; break;
            default: case IFile::Save::NoOp:    break;
            }
        }

        // For now, prioritize showing failure
        return any_fails ? IFile::Save::Fail
            : any_successes ? IFile::Save::Success
            : IFile::Save::NoOp;
    }

    IFile::Save saveAllInAllWindows_()
    {
        auto any_fails = false;
        auto any_successes = false;

        for (auto& window : windowManager_->windows())
        {
            switch (saveAllInWindow_(window))
            {
            case IFile::Save::Success:          any_successes = true; break;
            case IFile::Save::Fail:             any_fails = true; break;
            default: case IFile::Save::NoOp:    break;
            }
        }

        // For now, prioritize showing failure
        return any_fails ? IFile::Save::Fail
            : any_successes ? IFile::Save::Success
            : IFile::Save::NoOp;
    }

    /// ======================================================================== ///
    /// *** FILE VIEWS ***                                                       ///
    /// ======================================================================== ///

private:
    QHash<Window*, IFileView*> activeFileViews_{};

    // Todo: Make this a static IFileView function? (With matching function for
    //       IFile?)
    template <IFileViewPointer T>
    T to(IFileView* fileView)
    {
        return qobject_cast<T>(fileView);
    }

    // Note: May return nullptr
    IFileView* activeFileViewIn_(Window* window)
    {
        if (!window) return nullptr;
        return activeFileViews_[window];
    }

    // Note: May return nullptr
    IFileView* fileViewAt_(TabView* tabView, int index) const
    {
        if (!tabView || index < 0) return nullptr;
        return tabView->widgetAt<IFileView*>(index);
    }

    void addFileViewToTabView_(IFileView* fileView, TabView* tabView)
    {
        if (!fileView || !tabView) return;

        auto file = fileView->model();
        if (!file) return;

        // Apply plain text editor settings if text file view
        if (auto text_file_view = to<TextFileView*>(fileView))
        {
            text_file_view->setFont(Ini::EditorFont::load(settings_));
            applyEditorValues_(text_file_view, Ini::Editor::load(settings_));
        }

        auto index = tabView->addTab(fileView, file->title());
        tabView->setTabToolTip(index, file->toolTip());
        tabView->setTabFlagged(index, file->isEdited());
        tabView->setCurrentIndex(index);
        fileView->setFocus();
    }

    // Note: Active file view can be nullptr
    void setActiveFileView_(Window* window, IFileView* fileView)
    {
        // While modification changed for each file is always connected to
        // modifying all views' tabs' flags (showing edited state), we also must
        // track the current view for certain operations and states as well as
        // temporarily connect each active file's signals to update the menu
        // actions
        if (!window) return;

        // Clean up old active file connections. Old view will be replaced when
        // new active view is connected
        if (auto old_file_view = activeFileViews_[window])
            if (auto old_cx = activeFileConnections_.take(window); !old_cx.isEmpty())
                for (auto& connection : old_cx)
                    disconnect(connection);

        // Update active file view tracking
        activeFileViews_[window] = fileView;

        if (fileView)
        {
            if (auto file = fileView->model())
            {
                auto& cx = activeFileConnections_[window];

                cx << connect
                (
                    file,
                    &IFile::modificationChanged,
                    this,
                    [=](bool c) { (void)c; menuManager_->updatePerWindowMenuToggles(window); }
                );

                cx << connect
                (
                    file,
                    &IFile::undoAvailable,
                    this,
                    [=](bool a) { (void)a; menuManager_->updateEditToggles(window); }
                );

                cx << connect
                (
                    file,
                    &IFile::redoAvailable,
                    this,
                    [=](bool a) { (void)a; menuManager_->updateEditToggles(window); }
                );

                cx << connect
                (
                    fileView,
                    &IFileView::selectionChanged,
                    this,
                    [=] { menuManager_->updateEditToggles(window); }
                );

                cx << connect
                (
                    fileView,
                    &IFileView::clipboardDataChanged,
                    this,
                    [=] { menuManager_->updateEditToggles(window); }
                );
            }
        }

        // Update initial menu state for new active file view
        menuManager_->updatePerWindowMenuToggles(window);
        menuManager_->updateEditToggles(window);

        setWordCounterOnActiveView_(fileView, window);
    }

    /// ======================================================================== ///
    /// *** VIEW EDITING ***                                                     ///
    /// ======================================================================== ///

private:
    void cutOnActiveFileView_(Window* window)
    {
        if (!window) return;

        if (auto view = activeFileViewIn_(window))
            view->cut();
    }

    void copyOnActiveFileView_(Window* window)
    {
        if (!window) return;

        if (auto view = activeFileViewIn_(window))
            view->copy();
    }

    void pasteOnActiveFileView_(Window* window)
    {
        if (!window) return;

        if (auto view = activeFileViewIn_(window))
            view->paste();
    }

    void deleteOnActiveFileView_(Window* window)
    {
        if (!window) return;

        if (auto view = activeFileViewIn_(window))
            view->deleteSelection();
    }

    void selectAllOnActiveFileView_(Window* window)
    {
        if (!window) return;

        if (auto view = activeFileViewIn_(window))
            view->selectAll();
    }

    /// ======================================================================== ///
    /// *** TAB VIEWS ***                                                        ///
    /// ======================================================================== ///

private:
    void initializeWindowTabView_(Window* window)
    {
        if (!window) return;

        auto tab_view = new TabView(window);
        window->setCentralWidget(tab_view);
        tab_view->setDragValidator(this, &Ecosystem::tabViewDragValidator_);

        connect(tab_view, &TabView::currentChanged, this, [=](int index) { onTabViewCurrentChanged_(window, tab_view, index); });
        connect(tab_view, &TabView::addTabRequested, this, [=] { openNewFile_(window); });
        connect(tab_view, &TabView::closeTabRequested, this, [=](int index) { onCloseTabRequested_(window, tab_view, index); });
        connect(tab_view, &TabView::tabDragged, this, &Ecosystem::onTabViewTabDragged_);
        connect(tab_view, &TabView::tabDraggedToDesktop, this, &Ecosystem::onTabViewTabDraggedToDesktop_);
        connect(tab_view, &TabView::tabCountChanged, this, [=] { menuManager_->updateViewToggles(window); });
    }

    bool tabViewDragValidator_(TabView* source, TabView* destination)
    {
        // Find windows containing these TabViews
        auto source_window = Coco::Utility::findParent<Window*>(source);
        auto target_window = Coco::Utility::findParent<Window*>(destination);

        // Allow drag if both windows belong to our WindowManager
        auto is_valid = source_window && target_window
            && windowManager_->contains(source_window)
            && windowManager_->contains(target_window);

        if (!is_valid) COCO_LOG_THIS("Rejected tab drag between different Ecosystems!");
        return is_valid;
    }

    // Note: May return nullptr
    TabView* tabViewIn_(Window* window) const
    {
        if (!window) return nullptr;
        return qobject_cast<TabView*>(window->centralWidget());
    }

    void onTabViewCurrentChanged_(Window* window, TabView* tabView, int index)
    {
        setActiveFileView_(window, fileViewAt_(tabView, index));
        COCO_LOG_THIS(QString("Active tab index: %0 [%1]").arg(index)
            .arg(COCO_PTR_QSTR(window)));
    }

    void onTabViewTabDragged_(const TabView::Location& old, const TabView::Location& now)
    {
        if (!old.isValid() || !now.isValid()) return;

        // Find the windows that contain these TabViews
        auto old_window = Coco::Utility::findParent<Window*>(old.tabView);
        auto new_window = Coco::Utility::findParent<Window*>(now.tabView);

        if (!old_window || !new_window) return;

        // If dragging within the same window, no state updates needed
        if (old_window == new_window) return;

        menuManager_->updateGlobalMenuToggles(); // Updates save all, close all action states, and prev/next window

        COCO_LOG_THIS(QString("Tab dragged from %0 to %1")
            .arg(COCO_PTR_QSTR(old_window))
            .arg(COCO_PTR_QSTR(new_window)));

        // Focus the dragged tab's view
        new_window->activate();

        if (auto view = fileViewAt_(now.tabView, now.index))
            view->setFocus();

        if (old.tabView->isEmpty()) old_window->close();
    }

    void onTabViewTabDraggedToDesktop_(TabView* source, const QPoint& dropPos, const TabView::TabSpec& tabSpec)
    {
        if (!source || !tabSpec.isValid()) return;

        // Calculate new window position (unsure how to calculate this in any
        // other way than just raw approximate)
        constexpr auto initial_tab_top_left_approximate = QPoint(0, 38);
        auto max_tab_size = source->maximumTabSize();

        // Account for tree view if present...
        // Also new window should inherit tree view state
        auto new_window_top_left = dropPos
            - initial_tab_top_left_approximate
            - tabSpec.relPos(max_tab_size.width(), max_tab_size.height());

        auto new_window = newWindow_(new_window_top_left);
        auto new_tab_view = tabViewIn_(new_window);

        if (!new_tab_view)
        {
            new_window->close();
            return;
        }

        // Allow window to show early to prevent weird bugs, like a missing
        // separator in WordCounter...
        new_window->show();

        auto index = new_tab_view->addTab(tabSpec.widget, tabSpec.text);
        new_tab_view->setTabData(index, tabSpec.userData);
        new_tab_view->setTabToolTip(index, tabSpec.toolTip);
        new_tab_view->setTabFlagged(index, tabSpec.isFlagged);
        new_tab_view->setCurrentIndex(index);

        menuManager_->updateGlobalMenuToggles();

        new_window->activate();
        tabSpec.widget->setFocus();

        auto old_window = Coco::Utility::findParent<Window*>(source);
        if (old_window && source->isEmpty()) old_window->close();
    }

    /// ======================================================================== ///
    /// *** TREE VIEW ***                                                        ///
    /// ======================================================================== ///

private:
    QHash<Window*, QDockWidget*> treeViewDockWidgets_{};

    TreeView* treeViewIn_(Window* window) const
    {
        if (auto dock_widget = treeViewDockWidgets_[window])
            return qobject_cast<TreeView*>(dock_widget->widget());

        return nullptr;
    }

    void initializeWindowTreeView_(Window* window)
    {
        if (!window) return;

        auto dock_widget = new QDockWidget(window);
        treeViewDockWidgets_[window] = dock_widget;

        auto tree_view = new TreeView(dock_widget);
        dock_widget->setWidget(tree_view);

        auto fs_model = new QFileSystemModel(tree_view);

        auto root_str = root_.toQString();
        fs_model->setRootPath(root_str);
        tree_view->setModel(fs_model);
        tree_view->setRootIndex(fs_model->index(root_str));

        window->addDockWidget(Qt::LeftDockWidgetArea, dock_widget);

        // Temporary sizing (will retrieve later from settings or session)
        window->resizeDocks
        (
            { dock_widget },
            { (window->width() / 3) },
            Qt::Horizontal
        );
    }

    /// ======================================================================== ///
    /// *** FILE VIEW CLOSING ***                                                ///
    /// ======================================================================== ///

private:
    bool anyCloseableViewsIn_(Window* window) const
    {
        auto tab_view = tabViewIn_(window);
        return tab_view && tab_view->count() > 0; // Replace with tab_view && fileViewAt_(tab_view, 0)? Same diff?
    }

    bool anyCloseableViewsInAnyWindow_() const
    {
        for (auto& window : windowManager_->windows())
            if (window && anyCloseableViewsIn_(window))
                return true;

        return false;
    }

    void cleanUpFileView_(IFileView* fileView)
    {
        if (!fileView) return;

        auto file = fileView->model();

        delete fileView;

        if (file && !fileManager_->hasViews(file))
        {
            fileManager_->remove(file);
            delete file;
        }

        menuManager_->updateGlobalMenuToggles();
    }

    void closeAllTabsInTabView_(TabView* tabView)
    {
        if (!tabView) return;

        // Close from back to front to avoid index shifting
        for (auto i = tabView->count() - 1; i >= 0; --i)
        {
            if (i < 0) continue;

            auto file_view = tabView->removeTab<IFileView*>(i);
            if (!file_view) continue;
            cleanUpFileView_(file_view);
        }
    }

    void closeAllTabsInWindow_(Window* window)
    {
        if (!window) return;

        auto tab_view = tabViewIn_(window);
        if (!tab_view) return;

        closeAllTabsInTabView_(tab_view);
    }

    template<typename CheckerT, typename SaverT>
    bool promptAndSaveIfNeeded_(Window* window, CheckerT checker, SaverT saver)
    {
        if (!window) return false; // Shouldn't happen

        if (checker())
        {
            switch (Dialogs::Unsaved::exec(window))
            {
            case Dialogs::Unsaved::Save:
            {
                // Proceed with close if successful (not if failed or no-op
                // (user canceled dialog, most likely))
                return saver() == IFile::Save::Success;
            }
            case Dialogs::Unsaved::Discard:
                return true; // Proceed with close
            default:
            case Dialogs::Unsaved::Cancel:
                return false;
            }
        }
        return true; // Proceed with close
    }

    // Note: We likely no longer need the bool return
    bool closeSingleFileView_(Window* window, IFileView* fileView)
    {
        if (!window || !fileView) return false;

        auto file = fileView->model();
        if (!file) return false;

        if (!promptAndSaveIfNeeded_(
            window,
            // If this file is open elsewhere, we don't need to save it:
            [=] { return FileManager::isEdited(file) && fileManager_->viewCount(file) < 2; },
            [=] { return saveFileCascade_(file, window); }
        )) return false;

        auto tab_view = tabViewIn_(window);
        if (!tab_view) return false;

        auto index = tab_view->indexOf(fileView);
        if (index >= 0) tab_view->removeTab(index); // Shouldn't be < 0 but always clean up

        cleanUpFileView_(fileView);
        return true;
    }

    void onCloseTabRequested_(Window* window, TabView* tabView, int index)
    {
        if (!window || !tabView || index < 0) return;

        auto file_view = fileViewAt_(tabView, index);
        if (!file_view) return;

        closeSingleFileView_(window, file_view);
    }

    void onCloseActiveFileView_(Window* window)
    {
        if (!window) return;

        auto file_view = activeFileViewIn_(window);
        if (!file_view) return;

        closeSingleFileView_(window, file_view);
    }

    // Note: Regarding bool return, see note on closeSingleFileView_
    bool onCloseAllInWindow_(Window* window)
    {
        if (!window) return false;

        if (!promptAndSaveIfNeeded_(
            window,
            [=] { return anySavableFilesIn_(window, SkipIfMultiWindow::Yes); },
            [=] { return saveAllInWindow_(window, SkipIfMultiWindow::Yes); }
        )) return false;

        closeAllTabsInWindow_(window);

        return true;
    }

    void closeAllInAllWindows_()
    {
        auto active_window = windowManager_->active();
        if (!active_window) return; // Shouldn't happen

        if (!promptAndSaveIfNeeded_(
            active_window,
            [&] { return anySavableFilesInAnyWindow_(); },
            [&] { return saveAllInAllWindows_(); }
        )) return;

        for (auto& window : windowManager_->windowsReversed())
            closeAllTabsInWindow_(window);
    }
};

//COCO_TRIVIAL_CLASS(TestNotebook, Ecosystem);
