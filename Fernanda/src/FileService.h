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

// Coordinates file I/O, model lifecycle, and save operations across the
// Workspace. Creates and manages file models, handles all save variants
// (save/save-as/save-all), and ensures models persist until their last view is
// closed
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

protected:
    virtual void registerBusCommands() override
    {
        bus->addCommandHandler(
            Commands::OPEN_FILE_AT_PATH,
            [&](const Command& cmd) {
                auto path = cmd.pathParam();
                if (path.isEmpty() || !path.exists()) return;

                // Check if model already exists and re-ready
                if (auto existing_model = pathToFileModel_[path]) {
                    emit bus->fileModelReadied(cmd.context, existing_model);
                    return;
                }

                newDiskFileModel_(cmd.context, path);
            });
    }

    virtual void connectBusEvents() override
    {
        //...
    }

private:
    QHash<Coco::Path, IFileModel*> pathToFileModel_{};

    void setup_()
    {
        //...
    }

    void newDiskFileModel_(Window* window, const Coco::Path& path)
    {
        if (!window) return;
        if (path.isEmpty() || !path.exists()) return;

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
            WARN("Failed to open new file model from disk for {}!", path);
            return;
        }

        pathToFileModel_[path] = model;
        connectNewModel_(model);
        emit bus->fileModelReadied(window, model);
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

    ///==================================================================

    /*void createNewTextFile_(Window* window)
    {
        if (!window) return;
        auto model = new TextFileModel({}, this);
        connectNewModel_(model);
        emit bus->fileModelReadied(window, model);
    }*/

private slots:
    void onViewClosed_(IFileView* view)
    {
        if (!view) return;
        auto model = view->model();
        if (!model) return;
        auto meta = model->meta();
        if (!meta) return;

        /*auto view_count = bus->call<int>(
            Commands::MODEL_VIEWS_COUNT,
            { { "model", toQVariant(model) } });

        if (view_count < 1) {
            auto path = meta->path();
            pathToFileModel_.remove(path);
            INFO(
                "Removed model for path: {}",
                path.isEmpty() ? "N/A" : path.toString());
            delete model;
        }*/
    }
};

} // namespace Fernanda
