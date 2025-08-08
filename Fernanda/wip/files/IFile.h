#pragma once

#include <QObject>
#include <QString>

#include "Coco/Concepts.h"
#include "Coco/Debug.h"
#include "Coco/Path.h"

class IFile : public QObject
{
    Q_OBJECT

public:
    enum class Save { NoOp = 0, Success, Fail };

    explicit IFile(const Coco::Path& path = {}, QObject* parent = nullptr)
        : QObject(parent)
        , path_(path)
    {
        updateDerivedProperties_();
    }

    virtual ~IFile() override { COCO_TRACER; }

    bool isOnDisk() const noexcept { return !path_.isEmpty(); }
    Coco::Path path() const noexcept { return path_; }
    QString title() const noexcept { return title_; }
    QString toolTip() const noexcept { return toolTip_; }

    void setPath(const Coco::Path& path)
    {
        if (path_ == path) return;
        path_ = path;
        updateDerivedProperties_();
        emit pathChanged(path_);
    }

    Save saveAs(const Coco::Path& newPath)
    {
        if (newPath.isEmpty()) return Save::NoOp;

        // Save to new location without changing current file state
        Coco::Path old_path = path_;
        path_ = newPath; // Temporarily set for save() to work

        auto result = save();

        if (result == Save::Success) {
            // Keep new path and update everything
            updateDerivedProperties_();
            emit pathChanged(path_); // Calling set path would not work here due to equality guard
        }
        else {
            // Revert path on failure
            path_ = old_path;
        }

        return result;
    }

signals:
    void displayPropertiesChanged();
    void pathChanged(const Coco::Path& path);

private:
    Coco::Path path_;

    QString title_{};
    QString toolTip_{};

    void updateDerivedProperties_()
    {
        QString old_title = title_;
        QString old_tooltip = toolTip_;

        // Update title: path stem > custom title > "Untitled"
        if (isOnDisk()) title_ = path_.stemQString();
        else if (!temporaryTitle_.isEmpty()) title_ = temporaryTitle_;
        else title_ = "Untitled";

        // Update tooltip: full path if available, otherwise title + status
        if (isOnDisk()) toolTip_ = path_.toQString();
        else toolTip_ = title_ + " [Not on disk]";

        // Emit single signal if anything changed
        if (title_ != old_title || toolTip_ != old_tooltip)
            emit displayPropertiesChanged();
    }

    /// ======================================================================== ///
    /// *** FOR SUBCLASSES ***                                                   ///
    /// ======================================================================== ///

public:
    virtual bool canEdit() const = 0;

    // For editable subclasses to override
    virtual Save save() { return Save::NoOp; }
    virtual QString preferredExt() const { return {}; }
    virtual bool isEdited() const { return false; }
    virtual bool hasUndo() const { return false; }
    virtual bool hasRedo() const { return false; }
    virtual void undo() {}
    virtual void redo() {}

signals:
    void modificationChanged(bool changed);
    void undoAvailable(bool available);
    void redoAvailable(bool available);

protected:
    void setTemporaryTitle(const QString& title)
    {
        if (temporaryTitle_ == title) return;
        temporaryTitle_ = title;
        updateDerivedProperties_();
    }

    void clearTemporaryTitle()
    {
        if (temporaryTitle_.isEmpty()) return;
        temporaryTitle_.clear();
        updateDerivedProperties_();
    }

private:
    QString temporaryTitle_{};
};

template<typename T>
concept IFilePointer = Coco::Concepts::DerivedPointer<IFile, T>;
