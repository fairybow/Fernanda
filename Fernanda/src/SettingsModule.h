#pragma once

#include <QFont>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Commander.h"
#include "EventBus.h"
#include "IService.h"
#include "Ini.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "Utility.h"

namespace Fernanda {

//...
class SettingsModule : public IService
{
    Q_OBJECT

public:
    SettingsModule(
        const Coco::Path& configPath,
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : IService(commander, eventBus, parent)
        , baseConfigPath_(configPath)
    {
        initialize_();
    }

    virtual ~SettingsModule() override { COCO_TRACER; }

    void setOverrideConfigPath(const Coco::Path& configPath)
    {
        if (!settings_) return;
        settings_->setOverride(configPath);
    }

private:
    Coco::Path baseConfigPath_;
    Settings* settings_ = nullptr;
    QPointer<SettingsDialog> dialog_ = nullptr;

    void initialize_()
    {
        settings_ = new Settings(baseConfigPath_, this);

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

        connect(eventBus, &EventBus::lastWindowClosed, this, [&] {
            if (dialog_) dialog_->close();
        });
    }

    void openDialog_()
    {
        if (dialog_) {
            dialog_->raise();
            dialog_->activateWindow();
            return;
        }

        auto initial_font = settings_->value<QFont>(
            Ini::Editor::FONT_KEY,
            Ini::Editor::defaultFont());
        // Other initials later...

        dialog_ = new SettingsDialog(initial_font);

        dialog_->setFontChangeHandler([&](const QFont& font) {
            emit eventBus->settingChanged(Ini::Editor::FONT_KEY, toQVariant(font));
        });
        //...

        connect(
            dialog_,
            &SettingsDialog::fontSaveRequested,
            this,
            [&](const QFont& font) {
                if (settings_->isWritable())
                    settings_->setValue(
                        Ini::Editor::FONT_KEY,
                        toQVariant(font));
            });
        //...

        connect(dialog_, &SettingsDialog::finished, this, [&](int result) {
            (void)result;
            if (dialog_) dialog_->deleteLater();
        });

        dialog_->open();
    }
};

} // namespace Fernanda
