/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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
#include <QList>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "ui/settings/SettingRow.h"
#include "ui/settings/SettingsPage.h"
#include "ui/widgets/ToggleSwitch.h"

namespace Fernanda::Ui {

// The settings dialog's Status bar page: what the window status bar shows. One
// section per item (ui/WordCounter.h, ui/CursorPosition.h), each led by the
// item's own switch and followed by a switch for each thing it can show.
// Obsidian has no such page — its word count is a core plugin with no options,
// and it has no cursor position — so the layout is Suzuri's.
//
// While an item is off, the rows for its parts are disabled rather than
// hidden: they stay readable, and keep their values for when the item comes
// back on (VaultConfig keeps them too).
//
// Every setting here is on/off, so SettingsPage::addSwitch builds them all;
// each call names the Vault setter it drives
class SettingsStatusBarPage : public SettingsPage
{
    Q_OBJECT

public:
    SettingsStatusBarPage(Vault* vault, QWidget* parentSettingsDialog)
        : SettingsPage(vault, parentSettingsDialog)
    {
        setup_();
    }

    ~SettingsStatusBarPage() override { TRACER; }

private:
    void setup_()
    {
        setupWordCounterSection_();
        setupCursorPositionSection_();
    }

    void setupWordCounterSection_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Word counter"));

        auto item = addSwitch(
            tr("Show word counter"),
            tr("Count the active text file in the status bar."),
            config.wordCounterEnabled(),
            &VaultConfig::setWordCounterEnabled);

        QList<SettingRow*> parts{};

        parts << addSwitch(
                     tr("Words"),
                     tr("Show the number of words."),
                     config.wordCounterWords(),
                     &VaultConfig::setWordCounterWords)
                     .row;

        parts << addSwitch(
                     tr("Characters"),
                     tr("Show the number of characters, spaces included."),
                     config.wordCounterCharacters(),
                     &VaultConfig::setWordCounterCharacters)
                     .row;

        parts << addSwitch(
                     tr("Lines"),
                     tr("Show the number of lines."),
                     config.wordCounterLines(),
                     &VaultConfig::setWordCounterLines)
                     .row;

        parts << addSwitch(
                     tr("Selection counts"),
                     tr("While text is selected, show its counts against "
                        "the file's, as \"12 of 1,234 words\"."),
                     config.wordCounterSelection(),
                     &VaultConfig::setWordCounterSelection)
                     .row;

        enableWith_(item.toggle, parts);
    }

    void setupCursorPositionSection_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Cursor position"));

        auto item = addSwitch(
            tr("Show cursor position"),
            tr("Show where the text cursor is in the status bar."),
            config.cursorPositionEnabled(),
            &VaultConfig::setCursorPositionEnabled);

        QList<SettingRow*> parts{};

        parts << addSwitch(
                     tr("Line"),
                     tr("Show the line number."),
                     config.cursorPositionLine(),
                     &VaultConfig::setCursorPositionLine)
                     .row;

        parts << addSwitch(
                     tr("Column"),
                     tr("Show the column: the cursor's character within "
                        "its line."),
                     config.cursorPositionColumn(),
                     &VaultConfig::setCursorPositionColumn)
                     .row;

        enableWith_(item.toggle, parts);
    }

    // Keep rows enabled only while toggle is on: now, and on every flip. A
    // disabled row greys its text and its switch and takes no input
    void enableWith_(ToggleSwitch* toggle, const QList<SettingRow*>& rows)
    {
        auto apply = [rows](bool enabled) {
            for (auto* row : rows) {
                row->setEnabled(enabled);
            }
        };

        apply(toggle->isChecked());
        connect(toggle, &QAbstractButton::toggled, this, apply);
    }
};

} // namespace Fernanda::Ui
