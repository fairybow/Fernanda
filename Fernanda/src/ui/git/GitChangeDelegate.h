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
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QString>
#include <QStringList>
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

// Paints each row of the Git page's list of changed files: the file's name,
// its folder after it as a muted label (as the file tree labels an
// extension), and its change's mark against the row's right edge, in the
// change's color (GIT_ADDED_COLOR and the rest). The mark is a square with a
// plus (added, or new and untracked), a dot (modified), a minus (deleted), or
// an arrow (renamed), and a warning triangle for a file left conflicted by a
// merge. As the list narrows the name elides first, then the folder, from
// its left, so its nearest folder stays; the mark never does.
//
// The row itself is painted by the style, as the file tree's are: the name is
// only shortened to the room left of the folder before the style draws it, so
// the checkbox, selection, hover, and text colors are what any row gets. A
// row's change is read from CHANGE_ROLE and its folder, relative to the
// repository, from FOLDER_ROLE (empty for a file at the repository's top)
class GitChangeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    // The role a row's GitStatus::Change is stored in, as an int
    static constexpr int CHANGE_ROLE = Qt::UserRole;

    // The role a row's folder is stored in, as a QString
    static constexpr int FOLDER_ROLE = Qt::UserRole + 1;

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

        // The folder's label is placed as if the row ended at the mark
        auto label_row = opt.rect;
        label_row.setRight(icon_rect.left() - ROW_BADGE_SPACING);

        auto badge_font = RowBadges::font(opt.font);
        auto folder = elidedFolder_(
            index.data(FOLDER_ROLE).toString(),
            badge_font,
            opt.rect.width() * GIT_CHANGE_FOLDER_MAX_PERCENT / 100);

        auto name_end = icon_rect.left();
        QRect folder_rect{};

        if (!folder.isEmpty()) {
            folder_rect =
                RowBadges::rects(label_row, QStringList{ folder }, badge_font)
                    .first();
            name_end = folder_rect.left();
        }

        opt.text = RowBadges::nameElidedBefore(opt, style, name_end);
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        if (!folder.isEmpty()) {
            RowBadges::paint(
                painter,
                opt,
                folder,
                badge_font,
                folder_rect,
                false);
        }

        auto change = GitStatus::Change(index.data(CHANGE_ROLE).toInt());
        auto dpr = opt.widget ? opt.widget->devicePixelRatioF()
                              : qApp->devicePixelRatio();

        auto pixmap = cacheOf_(change).pixmap(
            QString::fromLatin1(pathOf_(change)),
            GIT_CHANGE_ICON_EXTENT,
            colorOf_(change),
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
    // again when the device pixel ratio moves. An untracked file shares the
    // added glyph and its cache
    Glyph::Cache addedIcon_{};
    Glyph::Cache modifiedIcon_{};
    Glyph::Cache deletedIcon_{};
    Glyph::Cache renamedIcon_{};
    Glyph::Cache conflictedIcon_{};

    // The mark's square, vertically centered, its right edge where a row
    // label's would be
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

    // The folder, elided from its left to fit maxWidth with the label's
    // padding. Empty when it's empty, or when not even an ellipsis fits
    [[nodiscard]] static QString
    elidedFolder_(const QString& folder, const QFont& font, int maxWidth)
    {
        auto room = maxWidth - 2 * ROW_BADGE_H_PADDING;

        if (folder.isEmpty() || room <= 0) {
            return {};
        }

        return QFontMetrics(font).elidedText(folder, Qt::ElideLeft, room);
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

    [[nodiscard]] static QColor colorOf_(GitStatus::Change change)
    {
        switch (change) {
        case GitStatus::Change::Added:
        case GitStatus::Change::Untracked:
            return GIT_ADDED_COLOR;
        case GitStatus::Change::Modified:
            return GIT_MODIFIED_COLOR;
        case GitStatus::Change::Deleted:
            return GIT_DELETED_COLOR;
        case GitStatus::Change::Renamed:
            return GIT_RENAMED_COLOR;
        case GitStatus::Change::Conflicted:
            return GIT_CONFLICTED_COLOR;
        }

        return GIT_MODIFIED_COLOR;
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
