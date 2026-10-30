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

#include <QPalette>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/Updater.h"
#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

// The status bar's update notice: a glyph in the accent color, shown while
// the Updater has an update the user hasn't dismissed (Updater::hasNotice),
// in every window at once. Its tooltip names the version.
//
// What a click does is the VaultWindow's, which wires the notice in its own
// status bar and in each of its pop-outs': dismiss the notice, and open the
// vault's settings on the General page
class UpdateNotice : public GlyphButton
{
    Q_OBJECT

public:
    UpdateNotice(Updater* updater, QWidget* parentWindow)
        : GlyphButton(
              UPDATE_NOTICE_EXTENT,
              UPDATE_NOTICE_ICON_EXTENT,
              parentWindow)
        , updater_(updater)
    {
        setup_();
    }

    ~UpdateNotice() override { TRACER; }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        return QString::fromLatin1(UPDATE_NOTICE_ICON_PATH);
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return UPDATE_NOTICE_ICON_ROLE;
    }

private:
    Updater* updater_ = nullptr;

    void setup_()
    {
        connect(
            updater_,
            &Updater::stateChanged,
            this,
            &UpdateNotice::refresh_);

        refresh_();
    }

    void refresh_()
    {
        setToolTip(
            tr("Fernanda %1 is available").arg(updater_->release().version));
        setVisible(updater_->hasNotice());
    }
};

} // namespace Fernanda::Ui
