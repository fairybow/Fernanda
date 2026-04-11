/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariant>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Files.h"
#include "core/Io.h"
#include "core/MagicBytes.h"
#include "core/Tr.h"
#include "models/AbstractFileModel.h"
#include "models/FileMeta.h"
#include "models/PdfFileModel.h"
#include "models/RawFileModel.h"
#include "models/TextFileModel.h"
#include "services/AbstractService.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "workspaces/Bus.h"

namespace Fernanda {

/// TODO BA
COCO_BOOL(ClearModified)

// Creates and manages file models. Note, this is not named ModelService (or
// FileModelService). It would be a nice symmetry with ViewService, but this
// class does more than just model work right now (like writing to disk)
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

    /// TODO BA
    DECLARE_HOOK(
        std::function<void(const Coco::Path&)>,
        beforeWriteHook,
        setBeforeWriteHook)

    /// TODO BA
    DECLARE_HOOK(
        std::function<void(AbstractFileModel*)>,
        afterModelCreatedHook,
        setAfterModelCreatedHook)

    // TODO: Could use a handle (would that be too overly complex) instead of
    // passing models around?

    void setPathTitleOverride(const Coco::Path& path, const QString& title)
    {
        if (path.isEmpty() || title.isEmpty()) return;

        if (auto model = pathToFileModel_.value(path))
            if (auto meta = model->meta()) meta->setTitleOverride(title);
    }

    void openOffDiskPlainTextFileIn(
        Window* window,
        Files::Type plainTextFileType,
        const QString& initialTitle = {},
        const QString& initialContent = {})
    {
        if (!window) return;

        if (auto model = newOffDiskTextFileModel_(plainTextFileType)) {
            if (!initialContent.isEmpty()) {
                if (auto text_model = qobject_cast<TextFileModel*>(model)) {
                    text_model->insertContent(initialContent);
                }
            }

            if (!initialTitle.isEmpty()) {
                model->meta()->setTitleOverride(initialTitle);
            }

            signalFileModelReadied_(window, model);
        }
    }

    QSet<AbstractFileModel*> fileModels() const noexcept { return fileModels_; }

    bool anyModified() const
    {
        for (auto& model : fileModels_) {
            if (model && model->isModified()) return true;
        }

        return false;
    }

    [[nodiscard]] SaveResult save(
        AbstractFileModel* fileModel,
        ClearModified clearModified = ClearModified::Yes)
    {
        if (!fileModel || !fileModel->isUserEditable()) return NoOp;
        auto meta = fileModel->meta();
        if (!meta || meta->isStale()) return NoOp;
        auto path = meta->path();
        if (path.isEmpty()) return NoOp;

        return writeModelToDisk_(fileModel, path, clearModified);
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
        if (auto model = newDiskFileModel_(path, title)) {
            signalFileModelReadied_(window, model);
        }
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
            [](AbstractFileModel* fileModel) {
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

    // For ignoring our own watcher signals
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
        /// unsupported? Like mp3, etc. Just detect size and go, Oops IDK about
        /// that one...

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
            model = newDiskRawFileModel_(Files::fromMagicBytes(type), path);
            break;

        default:
        case MagicBytes::NoKnownSignature: {
            switch (Files::fromPath(path)) {

            case Files::Html:
                model = newDiskRawFileModel_(Files::Html, path);
                break;

            default:
                model = newDiskTextFileModel_(path);
                break;
            }

            break;
        }
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
        /// TODO BA
        if (afterModelCreatedHook_) afterModelCreatedHook_(model);
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

    AbstractFileModel* newOffDiskTextFileModel_(Files::Type plainTextFileType)
    {
        auto model = new TextFileModel(plainTextFileType, this);
        registerModel_(model);
        /// TODO BA
        if (afterModelCreatedHook_) afterModelCreatedHook_(model);
        connectNewModel_(model);

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
    newDiskRawFileModel_(Files::Type fileType, const Coco::Path& path)
    {
        if (path.isEmpty() || !path.exists()) return nullptr;

        auto model = new RawFileModel(fileType, path, this);
        model->setData(Io::read(path));
        model->setModified(false); // TODO: Probably not needed? Investigate
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

    SaveResult writeModelToDisk_(
        AbstractFileModel* model,
        const Coco::Path& path,
        ClearModified clearModified = ClearModified::Yes)
    {
        /// TODO BA
        if (beforeWriteHook_) beforeWriteHook_(path);

        auto q_path = path.toQString();

        // Temporarily remove from watcher. On Windows, QSaveFile::commit()
        // atomically replaces via MoveFileEx, which can fail with "Access is
        // denied" if the watcher engine holds a transient handle on the file
        // during a stat check
        watcher_->removePath(q_path);

        auto data = model->data();
        auto success = Io::write(data, path);

        // Re-add to watcher. recentlyWritten_ guards against a spurious
        // fileChanged signal that some platforms emit on re-add
        recentlyWritten_ << q_path;
        watcher_->addPath(q_path);

        if (success && clearModified) model->setModified(false);

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
            model->setModified(true);
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
