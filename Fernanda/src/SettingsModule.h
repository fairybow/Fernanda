#pragma once

#include <QDialog>
#include <QFont>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
#include "Ini.h"
#include "SettingsDialog.h"
#include "Utility.h"
// #include "TieredSettings.h"

namespace Fernanda {

/// For now, let's use a simple QSettings and get to Notebook's two-tier
/// settings later
class SettingsModule : public IService
{
    Q_OBJECT

public:
    SettingsModule(
        const Coco::Path& configPath,
        // const Coco::Path& fallbackConfigPath,
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
        , settings_(new QSettings(
              configPath.toQString(),
              QSettings::IniFormat,
              this)) //, settings_(new TieredSettings(configPath,
                     // fallbackConfigPath, this))
    {
        initialize_();
    }

    virtual ~SettingsModule() override { COCO_TRACER; }

private:
    QSettings* settings_;
    QPointer<SettingsDialog> dialog_ = nullptr;

    void initialize_()
    {
        commander->addCommandHandler(Commands::SettingsDialog, [&] {
            openDialog_();
        });

        commander->addCommandHandler(
            Commands::SetSetting,
            [&](const Command& cmd) {
                if (!settings_ || !settings_->isWritable()) return;
                settings_->setValue(
                    to<QString>(cmd.params, "key"),
                    cmd.params.value("value"));
            });

        commander->addQueryHandler(
            Queries::Setting,
            [&](const QVariantMap& params) {
                return settings_->value(
                    to<QString>(params, "key"),
                    params.value("default"));
            });
    }

    void openDialog_()
    {
        if (dialog_) {
            dialog_->raise();
            dialog_->activateWindow();
            return;
        }

        dialog_ = new SettingsDialog(Ini::EditorFont::load(commander));
        dialog_->setFontChangeHandler([&](const QFont& font) {
            emit eventBus->settingEditorFontChanged(font);
        });

        connect(
            dialog_,
            &SettingsDialog::fontPersistenceRequested,
            this,
            [&](const QFont& font) { Ini::EditorFont::save(font, commander); });

        connect(dialog_, &SettingsDialog::finished, this, [&](int result) {
            (void)result;
            if (dialog_) dialog_->deleteLater();
        });

        dialog_->open();
    }
};

} // namespace Fernanda
