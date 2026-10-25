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

#include <QWidget>

#include <Coco/Debug.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "ui/settings/SettingsPage.h"

namespace Fernanda::Ui {

// The settings dialog's Git page: whether this vault's windows look for the
// Git repository the vault is in and offer the Git view
// (core/git/GitRepository.h). Obsidian has no Git of its own, so the page is
// Fernanda's; Obsidian's Git plugin keeps its settings per vault, as this
// does. Off, Fernanda never runs git for the vault
class SettingsGitPage : public SettingsPage
{
    Q_OBJECT

public:
    SettingsGitPage(Vault* vault, QWidget* parentSettingsDialog)
        : SettingsPage(vault, parentSettingsDialog)
    {
        setup_();
    }

    ~SettingsGitPage() override { TRACER; }

private:
    void setup_()
    {
        addHeading(tr("Git"));

        addSwitch(
            tr("Show Git"),
            tr("When this vault is in a Git repository, show a button in the "
               "status bar that opens the repository's changes. Uses the Git "
               "installed on your computer."),
            vault()->config().gitEnabled(),
            &VaultConfig::setGitEnabled);
    }
};

} // namespace Fernanda::Ui
