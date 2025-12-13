/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTextDocument>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "AbstractService.h"
#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "FileMeta.h"
#include "FileTypes.h"
#include "Io.h"
#include "NoOpFileModel.h"
#include "TextFileModel.h"
#include "Tr.h"
#include "Window.h"

namespace Fernanda {

// Creates and manages file models
// TODO: Rename?
// TODO: When saving files, we should move originals to a back-up location
// (Notebook's archive save will do the same)
class FileService : public AbstractService
{
    Q_OBJECT

public:
    enum SaveResult
    {
        NoOp,
        Success,
        Failure
    };

    FileService(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~FileService() override { TRACER; }

    // TODO: Could use a handle (would that be too overly complex) instead of
    // passing models around?

    void deleteModel(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;

        auto path = fileModel->meta()->path();
        fileModels_.remove(fileModel);
        pathToFileModel_.remove(path);
        delete fileModel;
    }

    void setPathTitleOverride(const Coco::Path& path, const QString& title)
    {
        if (path.isEmpty() || title.isEmpty()) return;

        if (auto model = pathToFileModel_.value(path))
            if (auto meta = model->meta()) meta->setTitleOverride(title);
    }

    void openOffDiskTxtIn(Window* window)
    {
        if (!window) return;

        if (auto model = newOffDiskTextFileModel_())
            emit bus->fileModelReadied(window, model);
    }

    /// TODO SAVES

    QSet<AbstractFileModel*> fileModels() const noexcept { return fileModels_; }

    [[nodiscard]] SaveResult save(AbstractFileModel* fileModel)
    {
        if (!fileModel || !fileModel->supportsModification()) return NoOp;
        auto path = fileModel->meta()->path();
        if (path.isEmpty()) return NoOp;

        return writeModelToDisk_(fileModel, path);
    }

    [[nodiscard]] SaveResult
    saveAs(AbstractFileModel* fileModel, const Coco::Path& newPath)
    {
        if (!fileModel || !fileModel->supportsModification()) return NoOp;
        if (newPath.isEmpty()) return NoOp;

        // Prevent overwriting a different model's file
        if (auto existing = pathToFileModel_.value(newPath))
            if (existing != fileModel) return Failure;

        auto result = writeModelToDisk_(fileModel, newPath);

        if (result == Success) {
            // Signal handles hash update!
            fileModel->meta()->setPath(newPath);
        }

        return result;
    }

    void openFilePathIn(
        Window* window,
        const Coco::Path& path,
        const QString& title = {})
    {
        if (!window) return;
        if (path.isEmpty() || !path.isFile() || !path.exists()) return;

        // Check if model already exists and re-ready
        if (auto existing_model = pathToFileModel_[path]) {
            emit bus->fileModelReadied(window, existing_model);
            return;
        }

        // Else, make a new one and ready it
        if (auto model = newDiskFileModel_(path, title))
            emit bus->fileModelReadied(window, model);
    }

    /// TODO SAVES (END)

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    QSet<AbstractFileModel*> fileModels_{};
    QHash<Coco::Path, AbstractFileModel*> pathToFileModel_{};

    void setup_()
    {
        //...
    }

    // TODO: Do this for similar setups in other Services
    void
    registerModel_(AbstractFileModel* fileModel, const Coco::Path& path = {})
    {
        if (!path.isEmpty()) pathToFileModel_[path] = fileModel;
        fileModels_ << fileModel;
        connect(
            fileModel->meta(),
            &FileMeta::pathChanged,
            this,
            [&, fileModel](const Coco::Path& old, const Coco::Path& now) {
                if (!old.isEmpty()) pathToFileModel_.remove(old);
                if (!now.isEmpty()) pathToFileModel_[now] = fileModel;
            });
    }

    AbstractFileModel*
    newDiskFileModel_(const Coco::Path& path, const QString& title = {})
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        AbstractFileModel* model = nullptr;

        switch (FileTypes::type(path)) {
        case FileTypes::PlainText:
            model = newDiskTextFileModel_(path);
            break;
        default:
            model = new NoOpFileModel(path, this);
            break;
        }

        if (!model) {
            // TODO: UI feedback?
            WARN("Failed to open new file model from disk for {}!", path);
            return nullptr;
        }

        // Set title if provided (for Notebook files)
        if (!title.isEmpty())
            if (auto meta = model->meta()) meta->setTitleOverride(title);

        registerModel_(model, path);
        connectNewModel_(model);
        return model;
    }

    AbstractFileModel* newDiskTextFileModel_(const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto model = new TextFileModel(path, this);
        model->setData(Io::read(path));
        model->setModified(false); // Pretty important!

        // TODO: Handle document is nullptr?

        return model;
    }

    // TODO: Will need a newOffDiskFileModel_ function if we ever want new,
    // blank files that aren't plaintext (think via context menu click on add
    // tab button). Will be a template function and we can just pass the right
    // type, since setup will probably be the same for all.
    AbstractFileModel* newOffDiskTextFileModel_()
    {
        auto model = new TextFileModel({}, this);
        registerModel_(model);
        connectNewModel_(model);

        return model;
    }

    void connectNewModel_(AbstractFileModel* fileModel)
    {
        connect(
            fileModel,
            &AbstractFileModel::modificationChanged,
            this,
            [&, fileModel](bool modified) {
                emit bus->fileModelModificationChanged(fileModel, modified);
            });

        connect(fileModel->meta(), &FileMeta::changed, this, [&, fileModel] {
            emit bus->fileModelMetaChanged(fileModel);
        });

        // TODO: Emit initial states (needed?)
        emit bus->fileModelModificationChanged(
            fileModel,
            fileModel->isModified());
        emit bus->fileModelMetaChanged(fileModel);
    }

    SaveResult
    writeModelToDisk_(AbstractFileModel* model, const Coco::Path& path)
    {
        auto data = model->data();
        auto success = Io::write(data, path);
        if (success) model->setModified(false);

        return success ? Success : Failure;
    }
};

inline QString toQString(FileService::SaveResult saveResult) noexcept
{
    switch (saveResult) {
    default:
    case FileService::NoOp:
        return "FileService::NoOp";
    case FileService::Success:
        return "FileService::Success";
    case FileService::Failure:
        return "FileService::Failure";
    }
}

} // namespace Fernanda

/*
TODO SAVES

void deleteModels(const QList<AbstractFileModel*>& fileModels)
{
    for (auto& model : fileModels)
        deleteModel(model);
}

void deleteAll()
{
    for (auto& model : fileModels_)
        delete model;
    fileModels_.clear();
    pathToFileModel_.clear();
}
*/
