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

#include <QComboBox>
#include <QFontDatabase>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/BundledFonts.h"
#include "core/VaultConfig.h"
#include "ui/UiConstants.h"

namespace Fernanda::Ui {

// A font family picker. The list runs, separated into groups:
//
//   [missing]  the current family, when this machine doesn't have it
//   default    the default text font (VaultConfig), bundled
//   bundled    the other fonts Fernanda ships (BundledFonts)
//   system     every other family the font database knows
//
// Bundled families are registered with the application's font database, so
// they're in its family list too; they're dropped from the system group so
// none appears twice. Private families (macOS's dot-prefixed system fonts,
// which aren't meant to be picked) are dropped as well.
//
// Families aren't filtered by name: dropping those that end in Bold, Light,
// Black, and the like would also drop real families ("Arial Black"). And a
// family that isn't installed here is shown rather than hidden. A vault can
// arrive from another machine naming a font this one lacks: Qt falls back to
// another font to draw it, but the setting still names the original, and a box
// showing the default instead would misstate it. The missing entry is labeled
// as such and stays in the list, so the user can switch away and back.
//
// Each item's data is the family name, and the label may differ (the missing
// entry's does). The owner reads family() — never currentText() — on
// currentIndexChanged. Items are drawn in the UI font; a preview of each
// family in its own face is a possible later addition
class FontFamilyBox : public QComboBox
{
    Q_OBJECT

public:
    FontFamilyBox(const QString& currentFamily, QWidget* parent)
        : QComboBox(parent)
    {
        setup_(currentFamily);
    }

    ~FontFamilyBox() override { TRACER; }

    // The selected family, from the item's data (not its label)
    [[nodiscard]] QString family() const { return currentData().toString(); }

private:
    void setup_(const QString& currentFamily)
    {
        auto default_family =
            QString::fromLatin1(VaultConfig::DEFAULT_TEXT_FONT_FAMILY);

        auto bundled = BundledFonts::families();
        bundled.removeAll(default_family);

        QSet<QString> listed_bundled(
            BundledFonts::families().cbegin(),
            BundledFonts::families().cend());
        listed_bundled << default_family;

        QStringList system{};
        for (const auto& family : QFontDatabase::families()) {
            if (listed_bundled.contains(family) ||
                QFontDatabase::isPrivateFamily(family)) {
                continue;
            }

            system << family;
        }

        auto known = listed_bundled.contains(currentFamily) ||
                     system.contains(currentFamily);

        if (!known) {
            addItem(tr("%1 (not installed)").arg(currentFamily), currentFamily);
            insertSeparator(count());
        }

        addFamily_(default_family);
        insertSeparator(count());

        for (const auto& family : bundled) {
            addFamily_(family);
        }

        insertSeparator(count());

        for (const auto& family : system) {
            addFamily_(family);
        }

        setCurrentIndex(findData(currentFamily));

        // Sized to a fixed number of characters rather than the longest family
        // installed, which can be very long and would push the row wide
        setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        setMinimumContentsLength(FONT_FAMILY_BOX_MIN_CHARS);
    }

    void addFamily_(const QString& family) { addItem(family, family); }
};

} // namespace Fernanda::Ui
