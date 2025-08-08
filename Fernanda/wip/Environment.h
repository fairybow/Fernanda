#pragma once

#include <QAction>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QString>

#include "Coco/Debug.h"
#include "Coco/Log.h"
#include "Coco/Path.h"

#include "dialogs/About.h"
#include "ecosystems/Ecosystem.h"
#include "FileTypes.h"
#include "Tr.h"
#include "window/Window.h"

class Environment : public QObject
{
    Q_OBJECT

public:
    explicit Environment(QObject* parent = nullptr)
        : QObject(parent)
    {
        initialize_();
    }

    virtual ~Environment() override { COCO_TRACER; }

    void onStartCopAppRelaunched(QStringList args)
    {
        COCO_LOG_THIS(QString("Relaunch attempted with the following arguments: %0")
            .arg(args.join(",")));
    }

private:
    Coco::Path userDataDirectory_ = Coco::Path::Home(".fernanda");
    Coco::Path notepadRoot_ = Coco::Path::Documents("Fernanda");

    Ecosystem* notepad_ = nullptr;
    //TestNotebook* testNotebook_ = nullptr;
    //QList<Notebook*> notebooks_{};

    void initialize_()
    {
        Coco::PathUtil::mkdir(userDataDirectory_);
        initializeNotepad_();

        // May read session files here
        {
            // Get:
            // - Windows, their rects, open files in each, and active indexes in each.
            // - Plus auto-saved content
            // - Notebook paths with information held in each Notebook file for all the above.
        }

        // Open notebooks if necessary and any notepad files. If no Notebooks
        // were open and no Notepad files were open, open a blank Notepad.

        // Temporary start procedures
        notepad_->openWindow();
        COCO_SST(1500, [&] { notepad_->beCute(); });
    }

    void initializeNotepad_()
    {
        Coco::PathUtil::mkdir(notepadRoot_);

        notepad_ = new Ecosystem(notepadRoot_, userDataDirectory_ / "Settings.ini", this);
        notepad_->setMenuPopulator(this, &Environment::ecosystemsMenuPopulator_);
        notepad_->setFileInterceptor(this, &Environment::notepadFileInterceptor_);
        connect(notepad_, &Ecosystem::lastWindowClosed, this, [&] { COCO_LOG_THIS("Last Notepad Window closed."); });

        // Repeat for TestNotebook, but use BOTH Notepad AND Notebook settings
        // config paths for tiered settings functionality
    }

    void ecosystemsMenuPopulator_(QMenuBar* menuBar)
    {
        if (!menuBar) return;

        auto menu = new QMenu(Tr::Env::help(), menuBar);
        auto about = menu->addAction(Tr::Env::about(), this, [=] { Dialogs::About::exec(); });
        about->setAutoRepeat(false);
        menuBar->addMenu(menu);
    }

    bool notepadFileInterceptor_(const Coco::Path& path)
    {
        if (FileTypes::is(FileTypes::SevenZip, path))
        {
            // For now, simply don't open 7zips.
            COCO_LOG_THIS("Interceptor true.");
            return true;
        }

        return false;
    }
};
