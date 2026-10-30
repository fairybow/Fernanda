/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The gear right of the vault switcher, where Obsidian puts its settings gear
// in the vault profile row. A GlyphButton with a fixed glyph, the same shape as
// NewTabButton: constants from glyphPath and glyphRole and nothing else. What a
// click means is wired by its owner (the Sidebar), which triggers the window's
// vault.openSettings action — so the button, Ctrl+, and the File menu item are
// one path
class SettingsButton : public GlyphButton
{
    Q_OBJECT

public:
    explicit SettingsButton(QWidget* parentSidebar)
        : GlyphButton(
              SETTINGS_BUTTON_EXTENT,
              SETTINGS_BUTTON_ICON_EXTENT,
              parentSidebar)
    {
        setToolTip(tr("Settings"));
    }

    ~SettingsButton() override { TRACER; }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        return u":/lucide/Settings.svg"_s;
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return SETTINGS_BUTTON_ICON_ROLE;
    }
};

} // namespace Fernanda::Ui
