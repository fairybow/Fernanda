/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/FileTypes.h"
#include "core/Io.h"
#include "core/MagicBytes.h"
#include "core/Tr.h"
#include "models/AbstractFileModel.h"
#include "models/FileMeta.h"
#include "models/ImageFileModel.h"
#include "models/PdfFileModel.h"
#include "models/TextFileModel.h"
#include "services/AbstractService.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "workspaces/Bus.h"

namespace Fernanda {

// Creates and manages file models
// TODO: When saving files, we should move originals to a back-up location
// (Notebook's archive save will do the same)
// TODO: Rename FileModelService? (and ViewService -> FileViewService?)
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
            signalFileModelReadied_(window, model);
    }

    QSet<AbstractFileModel*> fileModels() const noexcept { return fileModels_; }

    bool anyModified() const
    {
        for (auto& model : fileModels_) {
            if (model && model->isModified()) return true;
        }

        return false;
    }

    [[nodiscard]] SaveResult save(AbstractFileModel* fileModel)
    {
        if (!fileModel || !fileModel->supportsModification()) return NoOp;
        auto meta = fileModel->meta();
        if (!meta || meta->isStale()) return NoOp;
        auto path = meta->path();
        if (path.isEmpty()) return NoOp;

        return writeModelToDisk_(fileModel, path);
    }

    [[nodiscard]] SaveResult
    saveAs(AbstractFileModel* fileModel, const Coco::Path& newPath)
    {
        if (!fileModel) return NoOp;
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
            signalFileModelReadied_(window, existing_model);
            return;
        }

        // Else, make a new one and ready it
        if (auto model = newDiskFileModel_(path, title))
            signalFileModelReadied_(window, model);
    }

    AbstractFileModel* modelFor(const Coco::Path& path) const
    {
        return pathToFileModel_.value(path, nullptr);
    }

    QSet<AbstractFileModel*> modelsFor(const QSet<Coco::Path>& paths) const
    {
        QSet<AbstractFileModel*> models{};

        for (auto& path : paths)
            if (auto model = pathToFileModel_.value(path, nullptr))
                models << model;

        return models;
    }

    void deleteModel(AbstractFileModel* fileModel)
    {
        if (!fileModel) return;

        fileModels_.remove(fileModel);

        auto path = fileModel->meta()->path();
        if (!path.isEmpty()) {
            pathToFileModel_.remove(path);
            watcher_->removePath(path.toQString());
        }

        INFO("File model destroyed [{}]", fileModel);
        delete fileModel;
    }

    void deleteModels(const QSet<AbstractFileModel*>& fileModels)
    {
        if (fileModels.isEmpty()) return;

        for (auto& model : fileModels)
            deleteModel(model);
    }

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::fileModelReloadRequested,
            this,
            [this](AbstractFileModel* fileModel) {
                if (!fileModel) return;
                auto meta = fileModel->meta();
                if (!meta) return;

                auto path = meta->path();
                if (path.isEmpty() || !path.exists()) return;

                fileModel->setData(Io::read(path));
                fileModel->setModified(false);
                INFO("File model [{}] reloaded from disk", fileModel);
            });
    }

private:
    QSet<AbstractFileModel*> fileModels_{};
    QHash<Coco::Path, AbstractFileModel*> pathToFileModel_{};
    QFileSystemWatcher* watcher_ = new QFileSystemWatcher(this);
    QSet<QString> recentlyWritten_{};

    void setup_()
    {
        // Re: Notepad TreeView rename/move vs watcher: When a file is renamed
        // or moved via TreeView, both a model signal (fileRenamed / fileMoved)
        // and a QFileSystemWatcher::fileChanged signal will fire for the old
        // path. The model signal is handled synchronously on the main thread
        // (Notepad calls meta->setPath(), which triggers pathChanged, which
        // swaps the watched path here in registerModel_'s lambda). The
        // watcher's fileChanged crosses a thread boundary and is delivered via
        // queued connection, so it cannot arrive until the synchronous chain
        // completes and we return to the event loop. By that point, the old
        // path is no longer in pathToFileModel_ and the signal is harmlessly
        // ignored. This doesn't apply to Notebook (whose file moves are XML
        // only)

        connect(
            watcher_,
            &QFileSystemWatcher::fileChanged,
            this,
            &FileService::onWatcherFileChanged_);
    }

    // TODO: Do this for similar setups in other Services
    void
    registerModel_(AbstractFileModel* fileModel, const Coco::Path& path = {})
    {
        if (!path.isEmpty()) {
            pathToFileModel_[path] = fileModel;
            watcher_->addPath(path.toQString());
        }

        fileModels_ << fileModel;

        connect(
            fileModel->meta(),
            &FileMeta::pathChanged,
            this,
            [this, fileModel](const Coco::Path& old, const Coco::Path& now) {
                if (!old.isEmpty()) {
                    pathToFileModel_.remove(old);
                    watcher_->removePath(old.toQString());
                }
                if (!now.isEmpty()) {
                    pathToFileModel_[now] = fileModel;
                    watcher_->addPath(now.toQString());
                }
            });
    }

    /// TODO FT
    AbstractFileModel*
    newDiskFileModel_(const Coco::Path& path, const QString& title = {})
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        AbstractFileModel* model = nullptr;

        /// TODO FT: May want NoOp for the very large files that are also
        /// unsupported? Like jpeg, etc.
        /// OR just use it for stuff that will eventually be supported but
        /// isn't? (This is a later-problem, not a right-now problem)

        // Tier 1: Magic bytes for binary formats
        auto type = MagicBytes::type(path);
        switch (type) {

        case MagicBytes::Pdf:
            model = newDiskPdfFileModel_(path);
            break;

        case MagicBytes::Png:
        case MagicBytes::Tiff:
        case MagicBytes::Gif:
        case MagicBytes::Jpeg:
        case MagicBytes::Bmp:
        case MagicBytes::WebP:
            model = newDiskImageFileModel_(type, path);
            break;

        default:
        case MagicBytes::NoKnownSignature:

            // Tier 2: Extension for special plaintext types
            // switch (FileTypes::fromPath(path)) {
            // case FileTypes::Markdown:
            // case FileTypes::Fountain:
            // case FileTypes::FernandaCorkboard:
            // case FileTypes::FernandaWindowTheme:
            // case FileTypes::FernandaEditorTheme:
            // default:
            model = newDiskTextFileModel_(path);
            // break;
            //}

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

    AbstractFileModel* newDiskPdfFileModel_(const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto model = new PdfFileModel(path, this);
        model->setData(Io::read(path));
        model->setModified(false); // Probably not needed yet (PDFs may be
                                   // editable later, though)

        return model;
    }

    AbstractFileModel*
    newDiskImageFileModel_(MagicBytes::Type fileType, const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto type = FileTypes::fromMagicBytes(fileType);
        auto model = new ImageFileModel(type, path, this);
        model->setData(Io::read(path));
        model->setModified(false); // Probably not needed yet (images may be
                                   // editable later, though)

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
            [this, fileModel](bool modified) {
                signalFileModelModificationChanged_(fileModel, modified);
            });

        connect(fileModel->meta(), &FileMeta::changed, this, [this, fileModel] {
            signalFileModelMetaChanged_(fileModel);
        });

        // TODO: Emit initial states (needed?)
        signalFileModelModificationChanged_(fileModel, fileModel->isModified());
        signalFileModelMetaChanged_(fileModel);
    }

    SaveResult
    writeModelToDisk_(AbstractFileModel* model, const Coco::Path& path)
    {
        recentlyWritten_ << path.toQString();

        auto data = model->data();
        auto success = Io::write(data, path);
        if (success) model->setModified(false);

        return success ? Success : Failure;
    }

    void signalFileModelReadied_(Window* window, AbstractFileModel* fileModel)
    {
        INFO("File model readied in [{}]: [{}]", window, fileModel);
        emit bus->fileModelReadied(window, fileModel);
    }

    void signalFileModelModificationChanged_(
        AbstractFileModel* fileModel,
        bool modified)
    {
        INFO("File model [{}] modification changed to {}", fileModel, modified);
        emit bus->fileModelModificationChanged(fileModel, modified);
    }

    void signalFileModelMetaChanged_(AbstractFileModel* fileModel)
    {
        INFO("File model [{}] metadata changed", fileModel);
        emit bus->fileModelMetaChanged(fileModel);
    }

private slots:
    void onWatcherFileChanged_(const QString& path)
    {
        // Ignore watcher signals caused by our own writes
        if (recentlyWritten_.remove(path)) {
            // Some platforms/operations drop the watch after modification, so
            // we'll re-add in case:
            watcher_->addPath(path);
            return;
        }

        auto coco_path = Coco::Path(path);
        auto model = pathToFileModel_.value(coco_path);

        // Already swapped away (For Notepad: TreeView rename/move
        // (Doesn't apply to Notebook))
        if (!model) return;

        if (coco_path.exists()) {
            // Content changed externally (or atomic write replacement)
            watcher_->addPath(path);
            emit bus->fileModelExternallyModified(model);
        } else {
            // File gone (deleted, moved externally, unmounted)
            model->meta()->markStale();
            emit bus->fileModelPathInvalidated(model);
        }
    }
};

inline QString toQString(FileService::SaveResult saveResult) noexcept
{
    switch (saveResult) {
    default:
    case FileService::NoOp:
        return QStringLiteral("FileService::NoOp");
    case FileService::Success:
        return QStringLiteral("FileService::Success");
    case FileService::Failure:
        return QStringLiteral("FileService::Failure");
    }
}

} // namespace Fernanda
