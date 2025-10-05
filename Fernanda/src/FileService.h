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
#include "Coco/TextIo.h"

#include "Bus.h"
#include "Constants.h"
#include "Debug.h"
#include "FileMeta.h"
// #include "FileServiceSaveHelper.h"
#include "FileTypes.h"
#include "IFileModel.h"
#include "IFileView.h"
#include "IService.h"
#include "NoOpFileModel.h"
#include "TextFileModel.h"
#include "Tr.h"
#include "Utility.h"
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
        initialize_();
    }

    virtual ~FileService() override { TRACER; }

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
    QHash<Coco::Path, IFileModel*> pathToFileModel_{};
    // FileServiceSaveHelper* saveHelper_ = nullptr;

    void initialize_()
    {
        // saveHelper_ = new FileServiceSaveHelper(bus, pathToFileModel_, this);
    }

    void connectNewModel_(IFileModel* model)
    {
        connect(
            model,
            &IFileModel::modificationChanged,
            this,
            [&, model](bool modified) {
                emit bus->fileModificationChanged(model, modified);
            });

        auto meta = model->meta();

        if (meta) {
            connect(meta, &FileMeta::changed, this, [&, model] {
                emit bus->fileMetaChanged(model);
            });
        }

        // Emit initial states (needed?)
        emit bus->fileModificationChanged(model, model->isModified());
        emit bus->fileMetaChanged(model);
    }

    void createNewTextFile_(Window* window)
    {
        if (!window) return;
        auto model = new TextFileModel({}, this);
        connectNewModel_(model);
        emit bus->fileReadied(model, window);
    }

    void createExistingFile_(const Coco::Path& path, Window* window)
    {
        if (!window) return;
        if (path.isEmpty() || !path.exists()) return;

        IFileModel* model = nullptr;

        switch (FileTypes::type(path)) {
        case FileTypes::PlainText:
            model = createTextFileFromDisk_(path);
            break;
        default:
            model = new NoOpFileModel(path, this);
            break;
        }

        if (!model) return;

        pathToFileModel_[path] = model;
        connectNewModel_(model);
        emit bus->fileReadied(model, window);
    }

    IFileModel* createTextFileFromDisk_(const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto model = new TextFileModel(path, this);
        auto text = Coco::TextIo::read(path);
        auto document = model->document();
        document->setPlainText(text);
        document->setModified(false); // Pretty important!

        // TODO: Handle document is nullptr?

        return model;
    }

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
