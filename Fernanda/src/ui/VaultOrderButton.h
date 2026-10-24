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

#include "core/VaultOrder.h"
#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The button over Manage Vaults' list that shows the list's order and moves
// it on: recent, then name A to Z, then Z to A, then recent again. It shows
// the order it's given and nothing more: ManageVaults decides what a click
// does, sets the next order here, and sorts the list
class VaultOrderButton : public GlyphButton
{
    Q_OBJECT

public:
    VaultOrderButton(VaultOrder order, QWidget* parentManageVaults)
        : GlyphButton(
              VAULT_ORDER_BUTTON_EXTENT,
              VAULT_ORDER_BUTTON_ICON_EXTENT,
              parentManageVaults)
    {
        setAccessibleName(tr("Vault order"));
        setOrder(order);
    }

    ~VaultOrderButton() override { TRACER; }

    void setOrder(VaultOrder order)
    {
        order_ = order;

        switch (order_) {
        case VaultOrder::Recent:
            setToolTip(tr("Recently opened first"));
            break;
        case VaultOrder::NameAscending:
            setToolTip(tr("Name (A to Z)"));
            break;
        case VaultOrder::NameDescending:
            setToolTip(tr("Name (Z to A)"));
            break;
        }

        update();
    }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        switch (order_) {
        case VaultOrder::NameAscending:
            return u":/lucide/ArrowDownAZ.svg"_s;
        case VaultOrder::NameDescending:
            return u":/lucide/ArrowDownZA.svg"_s;
        case VaultOrder::Recent:
            break;
        }

        return u":/lucide/ClockArrowDown.svg"_s;
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return VAULT_ORDER_BUTTON_ICON_ROLE;
    }

private:
    VaultOrder order_ = VaultOrder::Recent;
};

} // namespace Fernanda::Ui
