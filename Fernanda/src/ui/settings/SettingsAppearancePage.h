/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
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

#include <QAbstractButton>
#include <QComboBox>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "ui/settings/FontFamilyBox.h"
#include "ui/settings/SettingsPage.h"
#include "ui/widgets/DisplaySlider.h"
#include "ui/widgets/ToggleSwitch.h"

namespace Fernanda::Ui {

// The settings dialog's Appearance page, Obsidian's Appearance tab: where the
// text font lives. The column, and how a page talks to its Vault, are
// SettingsPage's.
//
// The Font section, in Obsidian's order where it has the setting: the text
// font's family, Bold and Italic, then its size. Obsidian has no bold or italic
// for its text font; Fernanda keeps them because it's plain-text first —
// there's no Markdown styling to carry emphasis instead. Obsidian's size slider
// has a reset button beside it; this one doesn't
class SettingsAppearancePage : public SettingsPage
{
    Q_OBJECT

public:
    SettingsAppearancePage(Vault* vault, QWidget* parentSettingsDialog)
        : SettingsPage(vault, parentSettingsDialog)
    {
        setup_();
    }

    ~SettingsAppearancePage() override { TRACER; }

private:
    void setup_() { setupFontSection_(); }

    void setupFontSection_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Font"));

        // Read through family(), the item's data: a missing family's label
        // carries "(not installed)", and its data is the bare name
        auto* family = new FontFamilyBox(config.textFontFamily(), content());
        connect(family, &QComboBox::currentIndexChanged, this, [this, family] {
            vault()->setConfig(
                &VaultConfig::setTextFontFamily,
                family->family());
        });
        addRow(
            tr("Text font"),
            tr("The font used for text in the editor."),
            family);

        auto* bold = new ToggleSwitch(content());
        bold->setChecked(config.textFontBold());
        connect(bold, &QAbstractButton::toggled, this, [this](bool checked) {
            vault()->setConfig(&VaultConfig::setTextFontBold, checked);
        });
        addRow(tr("Bold"), tr("Show the text font in bold."), bold);

        auto* italic = new ToggleSwitch(content());
        italic->setChecked(config.textFontItalic());
        connect(italic, &QAbstractButton::toggled, this, [this](bool checked) {
            vault()->setConfig(&VaultConfig::setTextFontItalic, checked);
        });
        addRow(tr("Italic"), tr("Show the text font in italics."), italic);

        // Applies live while dragging; the Vault writes once it settles
        auto* size = new DisplaySlider(
            VaultConfig::MIN_TEXT_FONT_SIZE,
            VaultConfig::MAX_TEXT_FONT_SIZE,
            config.textFontSize(),
            content());
        connect(size, &DisplaySlider::valueChanged, this, [this](int value) {
            vault()->setConfig(&VaultConfig::setTextFontSize, value);
        });
        addRow(tr("Font size"), tr("The text font's size, in points."), size);
    }
};

} // namespace Fernanda::Ui
