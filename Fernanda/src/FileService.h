#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QTextDocument>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Bool.h"
#include "Coco/Debug.h"
#include "Coco/Path.h"
#include "Coco/PathUtil.h"
#include "Coco/TextIo.h"

#include "Commander.h"
#include "EventBus.h"
#include "FileMeta.h"
#include "FileServiceSaveHelper.h"
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
    explicit FileService(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
    {
        initialize_();
    }

    virtual ~FileService() override { COCO_TRACER; }

private:
    QHash<Coco::Path, IFileModel*> pathToFileModel_{};
    FileServiceSaveHelper* saveHelper_ = nullptr;

    void initialize_()
    {
        saveHelper_ = new FileServiceSaveHelper(
            commander,
            eventBus,
            pathToFileModel_,
            this);

        commander->addCommandHandler(Commands::NewTab, [&](const Command& cmd) {
            createNewTextFile_(cmd.context);
        });

        commander->addCommandHandler(
            Commands::OpenFile,
            [&](const Command& cmd) {
                auto path = Coco::Path(to<QString>(cmd.params, "path"));
                if (path.isEmpty() || !path.exists()) return;

                // Check if model already exists for this path
                if (auto existing_model = pathToFileModel_[path]) {
                    emit eventBus->fileReadied(existing_model, cmd.context);
                    return;
                }

                createExistingFile_(path, cmd.context);
            });

        commander->addCallHandler(Calls::Save, [&](const Call& call) {
            auto result = saveHelper_->saveAt(
                call.context,
                to<int>(call.params, "index", -1));
            emit eventBus->saveExecuted(call.context, result);
            return toQVariant(result);
        });

        commander->addCallHandler(Calls::SaveAs, [&](const Call& call) {
            auto result = saveHelper_->saveAsAt(
                call.context,
                to<int>(call.params, "index", -1));
            emit eventBus->saveExecuted(call.context, result);
            return toQVariant(result);
        });

        commander->addCallHandler(
            Calls::SaveIndexesInWindow,
            [&](const Call& call) {
                auto result = saveHelper_->saveIndexesInWindow(
                    call.context,
                    to<QList<int>>(call.params, "indexes"));
                emit eventBus->saveExecuted(
                    call.context,
                    result); // Right now, only planning to use these with
                             // ColorBar, but may want to have a more specific
                             // notification
                return toQVariant(result);
            });

        commander->addCallHandler(Calls::SaveWindow, [&](const Call& call) {
            auto result = saveHelper_->saveWindow(call.context);
            emit eventBus->saveWindowExecuted(
                call.context,
                result); // Re: ColorBar
            return toQVariant(result);
        });

        commander->addCallHandler(Calls::SaveAll, [&] {
            auto result = saveHelper_->saveAll();
            emit eventBus->saveAllExecuted(result); // Re: ColorBar
            return toQVariant(result);
        });

        connect(
            eventBus,
            &EventBus::viewClosed,
            this,
            &FileService::onViewClosed_);
    }

    void connectNewModel_(IFileModel* model)
    {
        connect(
            model,
            &IFileModel::modificationChanged,
            this,
            [&, model](bool modified) {
                emit eventBus->fileModificationChanged(model, modified);
            });

        auto meta = model->meta();

        if (meta) {
            connect(meta, &FileMeta::changed, this, [&, model] {
                emit eventBus->fileMetaChanged(model);
            });
        }

        // Emit initial states (needed?)
        emit eventBus->fileModificationChanged(model, model->isModified());
        emit eventBus->fileMetaChanged(model);
    }

    void createNewTextFile_(Window* window)
    {
        if (!window) return;
        auto model = new TextFileModel({}, this);
        connectNewModel_(model);
        emit eventBus->fileReadied(model, window);
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
        emit eventBus->fileReadied(model, window);
    }

    IFileModel* createTextFileFromDisk_(const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto model = new TextFileModel(path, this);
        auto text = Coco::TextIo::read(path);
        auto document = model->document();
        document->setPlainText(text);
        document->setModified(false); // Pretty important!

        // Todo: Handle document is nullptr?

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

        auto view_count = commander->query<int>(
            Queries::ViewCountForModel,
            { { "model", toQVariant(model) } });

        if (view_count < 1) {
            auto path = meta->path();
            pathToFileModel_.remove(path);
            COCO_LOG_THIS(QString("Removed model for path: %0")
                              .arg(path.isEmpty() ? "N/A" : path.toQString()));
            delete model;
        }
    }
};

} // namespace Fernanda
