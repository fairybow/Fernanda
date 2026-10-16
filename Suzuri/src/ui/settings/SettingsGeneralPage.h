/*
 * Suzuri — A plain-text editor for creative writing
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QAbstractButton>
#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>

#include "core/Publication.h"
#include "core/Translations.h"
#include "core/Updater.h"
#include "core/Vault.h"
#include "core/Version.h"
#include "ui/settings/SettingRow.h"
#include "ui/settings/SettingsPage.h"
#include "ui/widgets/ToggleSwitch.h"

namespace Suzuri::Ui {

// The settings dialog's General page, Obsidian's: the running version, a check
// for a newer one, whether Suzuri checks on its own, and the interface
// language. Suzuri doesn't download or install updates; a found one is linked
// to its release page.
//
// Unlike the other pages, nothing here belongs to the vault. The settings are
// App's (Updater, Translations), so every vault's General page shows the same
// ones, and each follows its owner's signals rather than seeding itself once:
// another vault's dialog can change them, and a check finishes while the page
// is open
class SettingsGeneralPage : public SettingsPage
{
    Q_OBJECT

public:
    SettingsGeneralPage(
        Vault* vault,
        Updater* updater,
        Translations* translations,
        QWidget* parentSettingsDialog)
        : SettingsPage(vault, parentSettingsDialog)
        , updater_(updater)
        , translations_(translations)
    {
        setup_();
    }

    ~SettingsGeneralPage() override { TRACER; }

private:
    Updater* updater_ = nullptr;
    Translations* translations_ = nullptr;

    SettingRow* versionRow_ = nullptr;
    QPushButton* openRelease_ = nullptr;
    QPushButton* check_ = nullptr;
    ToggleSwitch* automatic_ = nullptr;
    QComboBox* language_ = nullptr;

    void setup_()
    {
        addHeading(tr("App"));
        setupVersionRow_();
        setupAutomaticRow_();
        setupLanguageRow_();

        connect(
            updater_,
            &Updater::stateChanged,
            this,
            &SettingsGeneralPage::showState_);

        connect(updater_, &Updater::settingsChanged, this, [this] {
            automatic_->setChecked(updater_->checksAutomatically());
        });

        showState_();
    }

    // The version, with the last check's result under it, and the buttons
    // that check and open a release found
    void setupVersionRow_()
    {
        auto* control = new QWidget(content());
        auto* control_layout = new QHBoxLayout(control);
        control_layout->setContentsMargins(0, 0, 0, 0);

        openRelease_ = new QPushButton(tr("Open release page"), control);
        control_layout->addWidget(openRelease_);

        check_ = new QPushButton(tr("Check for updates"), control);
        control_layout->addWidget(check_);

        // Neither is the dialog's default button, which Enter would press
        openRelease_->setAutoDefault(false);
        check_->setAutoDefault(false);

        connect(openRelease_, &QPushButton::clicked, this, [this] {
            QDesktopServices::openUrl(updater_->release().pageUrl);
        });

        connect(check_, &QPushButton::clicked, updater_, &Updater::check);

        // The description is the check's result, which showState_ fills in
        versionRow_ = addRow(
            tr("Current version: %1 (%2)")
                .arg(VERSION_FULL_QSTRING, PUB_RELEASE_NAME_QSTRING),
            {},
            control);
    }

    void setupAutomaticRow_()
    {
        automatic_ = new ToggleSwitch(content());
        automatic_->setChecked(updater_->checksAutomatically());

        connect(
            automatic_,
            &QAbstractButton::toggled,
            updater_,
            &Updater::setChecksAutomatically);

        addRow(
            tr("Check for updates automatically"),
            tr("Check when Suzuri starts, then once a day. Suzuri only "
               "tells you about an update; it downloads nothing."),
            automatic_);
    }

    // The system's language first, then each there's a translation for, by
    // its own name. A saved language that's no longer available shows as the
    // system's, which is what the app falls back to
    void setupLanguageRow_()
    {
        language_ = new QComboBox(content());
        language_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        language_->addItem(tr("System default"), QString{});

        for (const auto& language : Translations::available()) {
            language_->addItem(Translations::displayName(language), language);
        }

        showLanguage_();

        connect(language_, &QComboBox::activated, this, [this](int index) {
            translations_->setSetting(language_->itemData(index).toString());
        });

        connect(
            translations_,
            &Translations::settingChanged,
            this,
            &SettingsGeneralPage::showLanguage_);

        addRow(
            tr("Language"),
            tr("The language of Suzuri's menus, settings, and messages. "
               "Takes effect the next time Suzuri starts."),
            language_);
    }

    void showLanguage_()
    {
        auto index = language_->findData(translations_->setting());
        language_->setCurrentIndex(qMax(0, index));
    }

    void showState_()
    {
        auto state = updater_->state();

        versionRow_->setDescription(stateText_(state));
        openRelease_->setVisible(state == Updater::State::Available);
        check_->setEnabled(state != Updater::State::Checking);
    }

    [[nodiscard]] QString stateText_(Updater::State state) const
    {
        switch (state) {
        case Updater::State::Checking:
            return tr("Checking for updates...");
        case Updater::State::UpToDate:
            return tr("Suzuri is up to date.");
        case Updater::State::Available:
            return tr("Version %1 is available.")
                .arg(updater_->release().version);
        case Updater::State::Failed:
            return tr("Couldn't check for updates: %1")
                .arg(updater_->failure());
        case Updater::State::Unchecked:
        default:
            return tr("Not checked yet.");
        }
    }
};

} // namespace Suzuri::Ui
