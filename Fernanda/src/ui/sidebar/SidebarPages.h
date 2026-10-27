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
#include <QHBoxLayout>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/Style.h"
#include "ui/UiConstants.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

namespace Internal {

// One page's button in SidebarPages' row. Not checkable: the row marks the
// active page's button (setActive), which is drawn over the shade of a
// selected row with its glyph in the text's own color, where the others' are
// muted. Hover and press washes are any glyph button's
class SidebarPageButton_ : public GlyphButton
{
public:
    SidebarPageButton_(const Coco::Path& glyphPath, QWidget* parentRow)
        : GlyphButton(
              SIDEBAR_PAGE_BUTTON_EXTENT,
              SIDEBAR_PAGE_BUTTON_ICON_EXTENT,
              parentRow)
        , glyphPath_(glyphPath)
    {
    }

    void setActive(bool active)
    {
        if (active != active_) {
            active_ = active;
            update();
        }
    }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override { return glyphPath_; }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return active_ ? SIDEBAR_PAGE_BUTTON_ACTIVE_ICON_ROLE
                       : SIDEBAR_PAGE_BUTTON_ICON_ROLE;
    }

    // The active shade, in the shape the style gives a tool button's panel,
    // under what GlyphButton paints
    void paintEvent(QPaintEvent* event) override
    {
        if (active_) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Style::selectedWash(palette()));
            painter.drawRoundedRect(
                QRectF(rect()),
                STYLE_TOOL_BUTTON_RADIUS,
                STYLE_TOOL_BUTTON_RADIUS);
        }

        GlyphButton::paintEvent(event);
    }

private:
    Coco::Path glyphPath_;
    bool active_ = false;
};

} // namespace Internal

// The sidebar's pages, one showing at a time, under a row of buttons, one per
// page, as Obsidian's sidebar has: what QTabWidget does, with flat glyph
// buttons for tabs. The row shows only while there's more than one page, so a
// sidebar with one looks as if this weren't here.
//
// Clicking a button shows its page and gives it the focus. The active page's
// button is marked (Internal::SidebarPageButton_). A page removed is taken
// out, not deleted, so its owner can add it again; removing the page that
// shows shows the first
class SidebarPages : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarPages(QWidget* parentSidebar)
        : QWidget(parentSidebar)
    {
        setup_();
    }

    ~SidebarPages() override { TRACER; }

    // Add page at the end, with a button showing glyphPath and toolTip
    void
    addPage(QWidget* page, const Coco::Path& glyphPath, const QString& toolTip)
    {
        auto* button = new Internal::SidebarPageButton_(glyphPath, row_);
        button->setToolTip(toolTip);
        rowLayout_->insertWidget(buttons_.size(), button);

        buttons_ << button;
        stack_->addWidget(page);

        connect(button, &QAbstractButton::clicked, this, [this, page] {
            setCurrentPage(page);
            page->setFocus();
        });

        update_();
    }

    void removePage(QWidget* page)
    {
        auto index = stack_->indexOf(page);

        if (index < 0) {
            return;
        }

        auto was_current = index == stack_->currentIndex();

        // Out of the stack, and hidden, but still this widget's child, so
        // it's deleted with the sidebar unless its owner adds it again
        stack_->removeWidget(page);
        page->setParent(this);
        page->hide();

        delete buttons_.takeAt(index);

        if (was_current && stack_->count() > 0) {
            stack_->setCurrentIndex(0);
        }

        update_();
    }

    [[nodiscard]] bool hasPage(QWidget* page) const
    {
        return stack_->indexOf(page) >= 0;
    }

    [[nodiscard]] QWidget* currentPage() const
    {
        return stack_->currentWidget();
    }

    void setCurrentPage(QWidget* page)
    {
        if (hasPage(page)) {
            stack_->setCurrentWidget(page);
            update_();
        }
    }

private:
    QWidget* row_ = nullptr;
    QHBoxLayout* rowLayout_ = nullptr;
    QStackedWidget* stack_ = nullptr;

    // One per page, in the stack's order
    QList<Internal::SidebarPageButton_*> buttons_{};

    void setup_()
    {
        row_ = new QWidget(this);
        rowLayout_ = new QHBoxLayout(row_);
        rowLayout_->setContentsMargins(
            SIDEBAR_PAGES_LEFT_MARGIN,
            SIDEBAR_PAGES_TOP_MARGIN,
            SIDEBAR_PAGES_RIGHT_MARGIN,
            SIDEBAR_PAGES_BOTTOM_MARGIN);
        rowLayout_->setSpacing(SIDEBAR_PAGE_BUTTON_SPACING);
        rowLayout_->addStretch(1);

        stack_ = new QStackedWidget(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(row_);
        layout->addWidget(stack_, 1);

        update_();
    }

    // The row while there's a choice, and the active page's button marked
    void update_()
    {
        row_->setVisible(stack_->count() > 1);

        for (qsizetype i = 0; i < buttons_.size(); ++i) {
            buttons_[i]->setActive(i == stack_->currentIndex());
        }
    }
};

} // namespace Fernanda::Ui
