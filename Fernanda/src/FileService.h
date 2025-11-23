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
#include <QSet>
#include <QObject>
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
    void deleteModel(IFileModel* model)
    {
        if (!model) return;

        auto path = model->meta()->path();
        models_.remove(model);
        pathToFileModel_.remove(path);
        delete model;
    }

    void deleteModels(const QList<IFileModel*>& models)
    {
        for (auto& model : models)
            deleteModel(model);
    }

    void deleteAll()
    {
        for (auto& model : models_)
            delete model;
        models_.clear();
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
    QSet<IFileModel*> models_{};
    QHash<Coco::Path, IFileModel*> pathToFileModel_{};

    void setup_()
    {
        //...
    }

    // TODO: Do this for similar setups in other Services
    void registerModel_(IFileModel* model, const Coco::Path& path = {})
    {
        if (!path.isEmpty()) pathToFileModel_[path] = model;
        models_ << model;
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

    void connectNewModel_(IFileModel* model)
    {
        connect(
            model,
            &IFileModel::modificationChanged,
            this,
            [&, model](bool modified) {
                emit bus->fileModelModificationChanged(model, modified);
            });

        connect(model->meta(), &FileMeta::changed, this, [&, model] {
            emit bus->fileModelMetaChanged(model);
        });

        // TODO: Emit initial states (needed?)
        emit bus->fileModelModificationChanged(model, model->isModified());
        emit bus->fileModelMetaChanged(model);
    }
};

} // namespace Fernanda
