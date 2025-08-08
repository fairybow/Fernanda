#pragma once

#include <type_traits>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVariant>
#include <QWidget>

#include "Coco/Bool.h"
#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "IFile.h"
#include "IFileView.h"

class FileManager : public QObject
{
    Q_OBJECT

public:
    COCO_BOOL(Initialize);

    using QObject::QObject;
    virtual ~FileManager() override { COCO_TRACER; }

    template <IFilePointer T>
    T makeFile(const Coco::Path& path = {})
    {
        auto file = new std::remove_pointer_t<T>(path, this);
        if (!file) return nullptr;

        fileToViews_[file] = {};

        if (!path.isEmpty())
            pathToFile_[path] = file;

        connect
        (
            file,
            &IFile::pathChanged,
            this,
            [=](const Coco::Path& path) { updatePathFor_(file, path); }
        );

        return file;
    }

    template <IFileViewPointer ViewT, IFilePointer FileT>
    ViewT makeView
    (
        FileT file,
        QWidget* parent = nullptr,
        Initialize initialize = Initialize::Yes
    )
    {
        if (!file || !fileToViews_.contains(file)) return nullptr;

        auto file_view = new std::remove_pointer_t<ViewT>(file, parent);
        if (!file_view) return nullptr;

        fileToViews_[file] << file_view;

        connect
        (
            file_view,
            &QObject::destroyed,
            this,
            [=] { removeView_(file, file_view); }
        );

        if (initialize) file_view->initialize();
        return file_view;
    }

    void remove(IFile* file)
    {
        if (!file || !fileToViews_.contains(file)) return;

        fileToViews_.remove(file);
        pathToFile_.remove(file->path());
        file->disconnect(this);
    }

    IFile* find(const Coco::Path& path) const
    {
        return pathToFile_.value(path, nullptr);
    }

private:
    QHash<IFile*, QSet<IFileView*>> fileToViews_{};
    QHash<Coco::Path, IFile*> pathToFile_{}; // For fast path lookups

    void updatePathFor_(IFile* file, const Coco::Path& path)
    {
        Coco::Path old_path{};

        for (const auto& [key, value] : pathToFile_.asKeyValueRange())
        {
            if (value == file)
            {
                old_path = key;
                break;
            }
        }

        // This should be prevented from OUTSIDE
        if (!path.isEmpty() && pathToFile_.contains(path) && pathToFile_[path] != file)
        {
            qFatal("Critical error: Path %s is already assigned to a different file",
                qPrintable(path.toQString()));
        }

        if (!old_path.isEmpty()) pathToFile_.remove(old_path);
        if (!path.isEmpty()) pathToFile_[path] = file;
    }

    void removeView_(IFile* file, IFileView* fileView)
    {
        if (!fileToViews_.contains(file)) return;
        fileToViews_[file].remove(fileView);
    }

    /// ======================================================================== ///
    /// *** FILE QUERIES ***                                                     ///
    /// ======================================================================== ///

public:
    static bool isEditedOnDisk(IFile* file)
    {
        return file && file->canEdit() && file->isOnDisk() && file->isEdited();
    }

    static bool isEditedUnsaved(IFile* file)
    {
        return file && file->canEdit() && !file->isOnDisk() && file->isEdited();
    }

    static bool isEdited(IFile* file)
    {
        return isEditedOnDisk(file) || isEditedUnsaved(file);
    }

    // Note: We'll NOT allow changing the path (Save As) for uneditable (NoOp or
    //       view-only) files
    static bool canSaveAs(IFile* file)
    {
        return file && file->canEdit();
    }

    static bool canUndo(IFile* file)
    {
        return file && file->hasUndo();
    }

    static bool canRedo(IFile* file)
    {
        return file && file->hasRedo();
    }

    QSet<IFileView*> viewsOn(IFile* file) const
    {
        if (!file || !fileToViews_.contains(file)) return {};
        return fileToViews_[file];
    }

    int viewCount(IFile* file) const
    {
        if (!file || !fileToViews_.contains(file)) return -1;
        return fileToViews_[file].count();
    }

    bool hasViews(IFile* file) const
    {
        if (!file || !fileToViews_.contains(file)) return false;
        return !fileToViews_[file].isEmpty();
    }

    /// ======================================================================== ///
    /// *** VIEW QUERIES ***                                                     ///
    /// ======================================================================== ///

public:
    static bool hasSelection(IFileView* fileView)
    {
        return fileView && fileView->canSelect() && fileView->hasSelection();
    }

    static bool canPaste(IFileView* fileView)
    {
        return fileView && fileView->canPaste();
    }

    // Probably good enough like this
    static bool canSelectAll(IFileView* fileView)
    {
        return fileView && fileView->canSelect();
    }
};
