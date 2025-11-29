/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTextDocument>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"

#include "Bus.h"
#include "Commands.h"
#include "Constants.h"
#include "Debug.h"
#include "FileMeta.h"
#include "FileTypes.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "IService.h"
#include "NoOpFileModel.h"
#include "TextFileModel.h"
#include "TextIo.h"
#include "Tr.h"
#include "Window.h"

namespace Fernanda {

// Creates and manages file models
// TODO: Rename?
class FileService : public IService
{
    Q_OBJECT

public:
    FileService(Bus* bus, QObject* parent = nullptr)
        : IService(bus, parent)
    {
        setup_();
    }

    virtual ~FileService() override { TRACER; }

    // TODO: Could use a handle (would that be too overly complex) instead of
    // passing models around?

    /// TODO NBM
    bool anyModified() const
    {
        for (auto& model : fileModels_)
            if (model && model->isModified()) return true;

        return false;
    }

    void deleteModel(IFileModel* fileModel)
    {
        if (!fileModel) return;

        auto path = fileModel->meta()->path();
        fileModels_.remove(fileModel);
        pathToFileModel_.remove(path);
        delete fileModel;
    }

    void deleteModels(const QList<IFileModel*>& fileModels)
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

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(
            Commands::OPEN_FILE_AT_PATH,
            [&](const Command& cmd) {
                if (!cmd.context) return;
                auto path = cmd.param<Coco::Path>("path", {});
                if (path.isEmpty() || !path.exists()) return;

                // Check if model already exists and re-ready
                if (auto existing_model = pathToFileModel_[path]) {
                    emit bus->fileModelReadied(cmd.context, existing_model);
                    return;
                }

                auto title = cmd.param<QString>("title", {});
                if (auto model = newDiskFileModel_(path, title))
                    emit bus->fileModelReadied(cmd.context, model);
            });
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    QSet<IFileModel*> fileModels_{};
    QHash<Coco::Path, IFileModel*> pathToFileModel_{};

    void setup_()
    {
        //...
    }

    // TODO: Do this for similar setups in other Services
    void registerModel_(IFileModel* fileModel, const Coco::Path& path = {})
    {
        if (!path.isEmpty()) pathToFileModel_[path] = fileModel;
        fileModels_ << fileModel;
    }

    IFileModel*
    newDiskFileModel_(const Coco::Path& path, const QString& title = {})
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        IFileModel* model = nullptr;

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

    IFileModel* newDiskTextFileModel_(const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto model = new TextFileModel(path, this);
        auto text = TextIo::read(path);

        // TODO: Is there a reason we don't have the model set its own text?
        auto document = model->document();
        document->setPlainText(text);
        document->setModified(false); // Pretty important!

        // TODO: Handle document is nullptr?

        return model;
    }

    // TODO: Will need a newOffDiskFileModel_ function if we ever want new,
    // blank files that aren't plaintext (think via context menu click on add
    // tab button). Will be a template function and we can just pass the right
    // type, since setup will probably be the same for all.
    IFileModel* newOffDiskTextFileModel_()
    {
        auto model = new TextFileModel({}, this);
        registerModel_(model);
        connectNewModel_(model);
        return model;
    }

    void connectNewModel_(IFileModel* fileModel)
    {
        connect(
            fileModel,
            &IFileModel::modificationChanged,
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
};

} // namespace Fernanda
