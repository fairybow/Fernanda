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

#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/UiConstants.h"
#include "ui/tabs/TabHistory.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

namespace Internal {

// One of the header's two arrows: a fixed glyph in a fixed tint. Disabled, it
// draws in the palette's Disabled colors, as every glyph button does
class TabHeaderButton_ : public GlyphButton
{
public:
    TabHeaderButton_(
        const Coco::Path& glyphPath,
        QPalette::ColorRole glyphRole,
        QWidget* parentHeader)
        : GlyphButton(
              TAB_HEADER_BUTTON_EXTENT,
              TAB_HEADER_BUTTON_ICON_EXTENT,
              parentHeader)
        , glyphPath_(glyphPath)
        , glyphRole_(glyphRole)
    {
    }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override { return glyphPath_; }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return glyphRole_;
    }

private:
    Coco::Path glyphPath_;
    QPalette::ColorRole glyphRole_;
};

} // namespace Internal

// The strip over a pane's pages, as Obsidian's view header is: back and
// forward arrows for the tab's history (TabHistory), then the title of the
// page the pane shows. One per pane (TabPaneLeaf), showing whichever page is
// current, so like the leaf it learns nothing of page types: it shows the
// title on the page's tab (a new tab's "New tab" included) and the history the
// page widget carries.
//
// An arrow is enabled while the history has an entry that way, and a click
// asks the leaf to step (stepRequested). The arrows never take the keyboard
// focus, as the tab row's buttons don't.
//
// It sits on the page, in the page's color (Base), so the current tab, the
// header, and the page read as one surface, as the find bar does
class TabHeader : public QWidget
{
    Q_OBJECT

public:
    explicit TabHeader(QWidget* parentLeaf)
        : QWidget(parentLeaf)
    {
        setup_();
    }

    ~TabHeader() override { TRACER; }

    // Show page's history's directions, and the title its tab shows, or
    // nothing for no page. Called again whenever either may have changed
    void showPage(QWidget* page, const QString& title)
    {
        auto history = pageHistory(page);

        back_->setEnabled(history.canGoBack());
        forward_->setEnabled(history.canGoForward());
        title_->setText(title);
    }

signals:
    void stepRequested(TabHistory::Step step);

private:
    Internal::TabHeaderButton_* back_ = new Internal::TabHeaderButton_(
        u":/lucide/ChevronLeft.svg"_s,
        TAB_HEADER_BACK_ICON_ROLE,
        this);
    Internal::TabHeaderButton_* forward_ = new Internal::TabHeaderButton_(
        u":/lucide/ChevronRight.svg"_s,
        TAB_HEADER_FORWARD_ICON_ROLE,
        this);
    QLabel* title_ = new QLabel(this);

    void setup_()
    {
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
        setFixedHeight(TAB_HEADER_HEIGHT);

        back_->setToolTip(tr("Navigate back"));
        forward_->setToolTip(tr("Navigate forward"));

        // The title takes what width is left and asks for none, so a long
        // title never widens the pane: it's cut off at the header's edge
        title_->setForegroundRole(TAB_HEADER_TITLE_ROLE);
        title_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(TAB_HEADER_LEFT_MARGIN, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(back_);
        layout->addSpacing(TAB_HEADER_ARROW_SPACING);
        layout->addWidget(forward_);
        layout->addSpacing(TAB_HEADER_TITLE_GAP);
        layout->addWidget(title_, 1);

        connect(back_, &QAbstractButton::clicked, this, [this] {
            emit stepRequested(TabHistory::Step::Back);
        });

        connect(forward_, &QAbstractButton::clicked, this, [this] {
            emit stepRequested(TabHistory::Step::Forward);
        });
    }
};

} // namespace Fernanda::Ui
