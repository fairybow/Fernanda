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

#include <QApplication>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QString>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/git/GitStatus.h"
#include "ui/UiConstants.h"
#include "ui/widgets/Glyph.h"
#include "ui/widgets/RowBadges.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// Paints each row of the Git view's list of changed files with its change's
// glyph against the list's right edge, where GitHub Desktop puts it: a square
// with a plus (added, or new and untracked), a dot (modified), a minus
// (deleted), or an arrow (renamed), and a warning triangle for a file left
// conflicted by a merge. As the list narrows, the path elides; the glyph never
// does.
//
// The row itself is painted by the style, as the file tree's are
// (VaultTreeItemDelegate): the path is only shortened to the room left of the
// glyph before the style draws it, so selection, hover, and text colors are
// what any row gets. Each row's change is read from CHANGE_ROLE
class GitChangeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    // The role a row's GitStatus::Change is stored in, as an int
    static constexpr int CHANGE_ROLE = Qt::UserRole;

    explicit GitChangeDelegate(QObject* parentList)
        : QStyledItemDelegate(parentList)
    {
    }

    ~GitChangeDelegate() override { TRACER; }

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        auto style = opt.widget ? opt.widget->style() : QApplication::style();
        auto icon_rect = iconRect_(opt.rect);

        opt.text = RowBadges::nameElidedBefore(opt, style, icon_rect.left());
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        auto change = GitStatus::Change(index.data(CHANGE_ROLE).toInt());
        auto color = opt.palette.color(
            RowBadges::colorGroupOf(opt.state),
            roleOf_(change));
        auto dpr = opt.widget ? opt.widget->devicePixelRatioF()
                              : qApp->devicePixelRatio();

        auto pixmap = cacheOf_(change).pixmap(
            QString::fromLatin1(pathOf_(change)),
            GIT_CHANGE_ICON_EXTENT,
            color,
            dpr);

        painter->drawPixmap(icon_rect.topLeft(), pixmap);
    }

private:
    static constexpr auto ADDED_ICON_PATH_ = ":/lucide/SquarePlus.svg";
    static constexpr auto MODIFIED_ICON_PATH_ = ":/lucide/SquareDot.svg";
    static constexpr auto DELETED_ICON_PATH_ = ":/lucide/SquareMinus.svg";
    static constexpr auto RENAMED_ICON_PATH_ = ":/lucide/SquareArrowRight.svg";
    static constexpr auto CONFLICTED_ICON_PATH_ = ":/lucide/TriangleAlert.svg";

    // One cache per glyph, each rendered on the first row that needs it and
    // again when the device pixel ratio or its tint moves. An untracked file
    // shares the added glyph and its cache
    Glyph::Cache addedIcon_{};
    Glyph::Cache modifiedIcon_{};
    Glyph::Cache deletedIcon_{};
    Glyph::Cache renamedIcon_{};
    Glyph::Cache conflictedIcon_{};

    // The glyph's square, vertically centered, its right edge where a row
    // badge's would be
    [[nodiscard]] static QRect iconRect_(const QRect& rowRect)
    {
        auto right = rowRect.right() - ROW_BADGE_RIGHT_MARGIN;
        auto top =
            rowRect.top() + (rowRect.height() - GIT_CHANGE_ICON_EXTENT) / 2;

        return QRect(
            right - GIT_CHANGE_ICON_EXTENT + 1,
            top,
            GIT_CHANGE_ICON_EXTENT,
            GIT_CHANGE_ICON_EXTENT);
    }

    [[nodiscard]] static const char* pathOf_(GitStatus::Change change)
    {
        switch (change) {
        case GitStatus::Change::Added:
        case GitStatus::Change::Untracked:
            return ADDED_ICON_PATH_;
        case GitStatus::Change::Modified:
            return MODIFIED_ICON_PATH_;
        case GitStatus::Change::Deleted:
            return DELETED_ICON_PATH_;
        case GitStatus::Change::Renamed:
            return RENAMED_ICON_PATH_;
        case GitStatus::Change::Conflicted:
            return CONFLICTED_ICON_PATH_;
        }

        return MODIFIED_ICON_PATH_;
    }

    [[nodiscard]] static QPalette::ColorRole roleOf_(GitStatus::Change change)
    {
        switch (change) {
        case GitStatus::Change::Added:
        case GitStatus::Change::Untracked:
            return GIT_ADDED_ICON_ROLE;
        case GitStatus::Change::Modified:
            return GIT_MODIFIED_ICON_ROLE;
        case GitStatus::Change::Deleted:
            return GIT_DELETED_ICON_ROLE;
        case GitStatus::Change::Renamed:
            return GIT_RENAMED_ICON_ROLE;
        case GitStatus::Change::Conflicted:
            return GIT_CONFLICTED_ICON_ROLE;
        }

        return GIT_MODIFIED_ICON_ROLE;
    }

    [[nodiscard]] const Glyph::Cache& cacheOf_(GitStatus::Change change) const
    {
        switch (change) {
        case GitStatus::Change::Added:
        case GitStatus::Change::Untracked:
            return addedIcon_;
        case GitStatus::Change::Modified:
            return modifiedIcon_;
        case GitStatus::Change::Deleted:
            return deletedIcon_;
        case GitStatus::Change::Renamed:
            return renamedIcon_;
        case GitStatus::Change::Conflicted:
            return conflictedIcon_;
        }

        return modifiedIcon_;
    }
};

} // namespace Fernanda::Ui
