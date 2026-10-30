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

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QLatin1StringView>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QPalette>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/VaultTreeModel.h"
#include "ui/UiConstants.h"
#include "ui/widgets/Glyph.h"
#include "ui/widgets/RowBadges.h"

namespace Fernanda::Ui {

// Paints each row of a vault tree with a Lucide glyph left of the name (File,
// or Folder / FolderOpen by expansion) and each file row with an extension
// badge: the extension, uppercased, in muted letters against the view's right
// edge (ui/widgets/RowBadges.h, without a border), the way Obsidian marks its
// non-Markdown files. The row's text is only the file's stem
// (VaultTreeModel::data), so the badge is the one place the extension shows.
// Every file but a .txt gets one; folders never do. As the view narrows, the
// name elides; the badge never does.
//
// The glyphs are a deliberate divergence: core Obsidian's explorer shows none.
// They live here, not in the model, because the model is QtCore-only and a
// QIcon is QtGui. TREE_ICONS_ENABLED turns them off entirely.
//
// The row itself is still painted by the style. The glyph goes in through
// initStyleOption as an ordinary decoration, so the style lays it out, sizes
// the row for it, and starts the text after it, exactly as it would for a
// model's DecorationRole. The name is only pre-shortened to the room left of
// the badge before the style draws the item, so selection, hover, focus, and
// text colors are exactly what an undecorated row gets.
//
// Glyph and badge both come from the model's typed nameOf and isDir, not a role
class VaultTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    VaultTreeItemDelegate(
        VaultTreeModel* treeModel,
        QObject* parentVaultTreeView)
        : QStyledItemDelegate(parentVaultTreeView)
        , treeModel_(treeModel)
    {
    }

    ~VaultTreeItemDelegate() override { TRACER; }

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        auto badge = badgeOf_(index);
        if (badge.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        auto style = opt.widget ? opt.widget->style() : QApplication::style();
        auto badge_font = RowBadges::font(opt.font);
        auto badge_rect = RowBadges::rects(opt.rect, { badge }, badge_font)[0];

        opt.text = RowBadges::nameElidedBefore(opt, style, badge_rect.left());
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        RowBadges::paint(painter, opt, badge, badge_font, badge_rect, false);
    }

protected:
    // Both paint paths come through here (the badgeless one inside
    // QStyledItemDelegate::paint, the badged one explicitly), and so does
    // sizeHint, which is what makes the row's layout account for the glyph
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index)
        const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        if constexpr (TREE_ICONS_ENABLED) {
            decorate_(option, index);
        }
    }

private:
    VaultTreeModel* treeModel_;

    // The one badgeless file type. A plain-text file is the ordinary case,
    // so it goes unmarked. At most one type can go without a badge: with the
    // extension off the row's text, "draft.md" and "draft.txt" would
    // otherwise both read as a bare "draft"
    static constexpr auto BADGELESS_EXTENSION_ = ".txt";

    static constexpr auto FILE_ICON_PATH_ = ":/lucide/File.svg";
    static constexpr auto FOLDER_ICON_PATH_ = ":/lucide/Folder.svg";
    static constexpr auto FOLDER_OPEN_ICON_PATH_ = ":/lucide/FolderOpen.svg";

    // One cache per glyph, each rendered on the first row that needs it (a tree
    // with no expanded folder never renders FolderOpen) and again whenever the
    // device pixel ratio (a window moved to another screen) or its tint (a
    // palette change, or its role resolving differently) moves. With
    // TREE_ICONS_ENABLED false none is ever asked, so each holds only null
    // members and Glyph::render is never called
    Glyph::Cache fileIcon_{};
    Glyph::Cache folderIcon_{};
    Glyph::Cache folderOpenIcon_{};

    // The extension without its dot, uppercased ("PDF", "FOUNTAIN"). Folders
    // and .txt files get none. Every listed file has an extension, since
    // typeOf calls a file with none Unsupported and the listing leaves it out
    [[nodiscard]] QString badgeOf_(const QModelIndex& index) const
    {
        if (treeModel_->isDir(index)) {
            return {};
        }

        auto ext = treeModel_->nameOf(index).extQString();
        if (ext.compare(
                QLatin1StringView(BADGELESS_EXTENSION_),
                Qt::CaseInsensitive) == 0) {
            return {};
        }

        return ext.mid(1).toUpper();
    }

    // The style draws opt->icon at decorationSize ahead of the text. The glyph
    // is chosen per row: File for files, Folder or FolderOpen by the view's
    // own expansion state (expansion lives in each view, not the model)
    //
    // Tints resolve against the item's current group (see the tint note in
    // UiConstants.h). The caches key on the resolved colors, so a focus change
    // on a palette whose groups agree re-renders nothing. Selected rows still
    // get a highlight wash, since the style asks the QIcon for its Selected
    // mode and generates one from the cached pixmap
    void decorate_(QStyleOptionViewItem* option, const QModelIndex& index) const
    {
        auto is_dir = treeModel_->isDir(index);
        auto tree_view = qobject_cast<const QTreeView*>(option->widget);
        auto is_expanded = is_dir && tree_view && tree_view->isExpanded(index);

        const auto& cache = !is_dir       ? fileIcon_
                            : is_expanded ? folderOpenIcon_
                                          : folderIcon_;
        auto path = !is_dir       ? FILE_ICON_PATH_
                    : is_expanded ? FOLDER_OPEN_ICON_PATH_
                                  : FOLDER_ICON_PATH_;
        auto role = is_dir ? TREE_FOLDER_ICON_ROLE : TREE_FILE_ICON_ROLE;

        auto dpr = option->widget ? option->widget->devicePixelRatioF()
                                  : qApp->devicePixelRatio();
        auto color =
            option->palette.color(RowBadges::colorGroupOf(option->state), role);

        option->features |= QStyleOptionViewItem::HasDecoration;
        option->icon =
            cache.icon(QString::fromLatin1(path), TREE_ICON_EXTENT, color, dpr);
        option->decorationSize = QSize(TREE_ICON_EXTENT, TREE_ICON_EXTENT);
    }
};

} // namespace Fernanda::Ui
