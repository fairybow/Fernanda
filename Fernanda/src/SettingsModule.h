/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Debug.h"
#include "Coco/Path.h"

#include "Bus.h"
#include "Constants.h"
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
        Bus* bus,
        QObject* parent = nullptr)
        : IService(bus, parent)
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

        bus->addCommandHandler(
            WorkspaceCmd::SETTINGS_GET,
            [&](const Command& cmd) {
                return settings_->value(
                    cmd.param<QString>("key"),
                    cmd.param("default"));
            });

        bus->addCommandHandler(
            WorkspaceCmd::SETTINGS_SET,
            [&](const Command& cmd) {
                if (!settings_ || !settings_->isWritable()) return;
                settings_->setValue(
                    cmd.param<QString>("key"),
                    cmd.param("value"));
            });

        bus->addCommandHandler(WorkspaceCmd::SETTINGS_DIALOG, [&] {
            openDialog_();
        });

        connect(bus, &Bus::lastWindowClosed, this, [&] {
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
            emit bus->settingChanged(Ini::Editor::FONT_KEY, toQVariant(font));
        });
        // Connect other setting handlers

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
        // Listen to other setting signals

        connect(dialog_, &SettingsDialog::finished, this, [&](int result) {
            (void)result;
            if (dialog_) dialog_->deleteLater();
        });

        dialog_->open();
    }
};

} // namespace Fernanda
