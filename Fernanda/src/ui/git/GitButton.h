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

#include <QPalette>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/git/GitRepository.h"
#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The status bar's Git button, at the bar's left. Checked while the window's
// Git view is open, which fills it with the accent (GlyphButton). Its
// tooltip names the branch, from the repository's last status.
//
// What it does, whether it shows, and whether it's checked are the
// VaultWindow's: it shows the button while the vault is in a repository, and
// a click triggers the window's repository.toggleView action, so the button,
// the shortcut, and the View menu are one path. The window sets the checked
// state from the view, so the button mirrors the view however it was opened
class GitButton : public GlyphButton
{
    Q_OBJECT

public:
    GitButton(GitRepository* repository, QWidget* parentVaultWindow)
        : GlyphButton(
              GIT_BUTTON_EXTENT,
              GIT_BUTTON_ICON_EXTENT,
              parentVaultWindow)
        , repository_(repository)
    {
        setup_();
    }

    ~GitButton() override { TRACER; }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        return u":/lucide/GitBranch.svg"_s;
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return GIT_BUTTON_ICON_ROLE;
    }

private:
    GitRepository* repository_;

    void setup_()
    {
        setCheckable(true);

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitButton::updateToolTip_);

        updateToolTip_();
    }

    // Before the first status, and with a detached head, there is no branch
    // to name
    void updateToolTip_()
    {
        const auto& branch = repository_->status().branch;

        if (branch.detached) {
            setToolTip(tr("Git (detached HEAD)"));
        } else if (branch.name.isEmpty()) {
            setToolTip(tr("Git"));
        } else {
            setToolTip(tr("Git (%1)").arg(branch.name));
        }
    }
};

} // namespace Fernanda::Ui
