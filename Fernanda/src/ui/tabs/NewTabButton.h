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

#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The new-tab + button, laid immediately after the bar by TabPaneLeaf (it hugs
// the tabs' right edge and pins to the window edge as the bar collapses under
// overflow). A GlyphButton with a fixed glyph and no page identity: it never
// has a pin state or a second SVG, so it only returns constants from
// glyphPath and glyphRole and sets its tooltip. Its click means "new tab" —
// the leaf wires clicked() to addPageRequested()
class NewTabButton : public GlyphButton
{
    Q_OBJECT

public:
    explicit NewTabButton(QWidget* parent)
        : GlyphButton(NEW_TAB_BUTTON_EXTENT, NEW_TAB_BUTTON_ICON_EXTENT, parent)
    {
    }

    ~NewTabButton() override { TRACER; }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        return u":/lucide/Plus.svg"_s;
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return NEW_TAB_ICON_ROLE;
    }
};

} // namespace Fernanda::Ui
