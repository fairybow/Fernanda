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

#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QCursor>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFrame>
#include <QHoverEvent>
#include <QList>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QString>
#include <QTreeView>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/VaultTreeModel.h"
#include "ui/Style.h"
#include "ui/UiConstants.h"
#include "ui/UiUtility.h"
#include "ui/sidebar/VaultEntryDragMime.h"
#include "ui/sidebar/VaultTreeItemDelegate.h"
#include "ui/widgets/Glyph.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The QTreeView inside a VaultFileTree. A plain tree, plus one feature:
// dragging an entry to move it.
//
// It does NOT use the item view's built-in model DnD. A model-driven drop would
// move the file on disk behind the BufferStore's re-keying, watcher, and
// write-fingerprint moves, so the model carries no DnD flags and stays out of
// it: DropOnly disables the base's model-driven drag start (we start our own
// QDrag, TabBar-style), while leaving the view a drop target so our own
// drag-event overrides run. The drop never touches disk here — it resolves the
// destination and emits moveRequested; the Sidebar maps that to the owning
// Vault's move.
//
// Scope is the vault root: a drag is accepted only if its source path lives
// under the model's root, which forbids cross-vault drops (the common tree's
// root differs) and rejects any foreign/OS drag for free. The move itself is
// same-tree; a drop onto an editor pane (opening the file) is a separate accept
// site (TabPaneTree).
//
// And it paints its own expand / collapse chevrons: Lucide ChevronRight /
// ChevronDown in one muted tint, collapsed or expanded, in place of the style's
// twisty, which tints the two states differently. See drawBranches.
class VaultTreeView : public QTreeView
{
    Q_OBJECT

public:
    // Sits on its vault's shared VaultTreeModel and reads paths and kinds back
    // from it directly, the root included: the model's root scopes the drag,
    // so an entry outside it is a foreign or cross-vault source. The root is
    // asked for each time, since the model can move it (moveRoot) and
    // every path it hands out follows
    VaultTreeView(VaultTreeModel* treeModel, QWidget* parentVaultFileTree)
        : QTreeView(parentVaultFileTree)
        , treeModel_(treeModel)
    {
        setup_();
    }

    ~VaultTreeView() override { TRACER; }

    // Hover the row under the cursor, or none when the cursor isn't over the
    // rows. Qt updates hover only when the mouse moves, so after the view or
    // its rows move under a still cursor, the hovered row is wherever the
    // cursor was before. A HoverMove is what a real move sends, and
    // QAbstractItemView answers it by hovering the row at its position
    void syncHover()
    {
        auto global = QCursor::pos();

        if (!viewport()->isVisible() ||
            QApplication::widgetAt(global) != viewport()) {
            clearHover_();
            return;
        }

        auto local = QPointF(viewport()->mapFromGlobal(global));
        QHoverEvent move(QEvent::HoverMove, local, QPointF(global), local);
        QCoreApplication::sendEvent(viewport(), &move);
    }

    // The row at a point, but only within the row's wash (Style::itemWashRect):
    // beside it, left of a nested row or in the margin at either side, is no
    // row. The wash is what shows a row's extent, so a point outside it is
    // empty space everywhere the view asks: a press there clears the
    // selection, hover there shows no row, a right-click there gets the vault
    // root's menu, and a drop there lands in the vault root. Only the wash's
    // sides count; the pixel it leaves between two rows still belongs to them
    [[nodiscard]] QModelIndex indexAt(const QPoint& point) const override
    {
        auto index = QTreeView::indexAt(point);
        if (!index.isValid()) {
            return {};
        }

        auto wash = Style::itemWashRect(visualRect(index), this);
        if (point.x() < wash.left() || point.x() >= wash.right()) {
            return {};
        }

        return index;
    }

signals:
    // A committed move-drop: move absoluteSource into absoluteDestDir (an
    // existing folder in this vault, or the vault root). Only fired for a
    // genuine move — same-vault-scoped, not a no-op, not a folder into
    // itself/descendant. The pane relays it; the Sidebar maps it to the owning
    // Vault, which re-checks authoritatively and does the disk work
    void moveRequested(
        const Coco::Path& absoluteSource,
        const Coco::Path& absoluteDestDir);

protected:
    // --- Branch column -------------------------------------------

    // Replaces the style's branch indicators outright. drawRow has already
    // filled the gutter's background (selection, hover, alternation) before
    // calling this, so only the indicator is ours. One glyph per row, in the
    // row's own cell — the innermost indentation slice at the gutter's right
    // edge. Ancestor cells get nothing, so no style's guide lines survive
    // either (Windows 11 draws none). Expansion hit-testing is Qt's and is
    // cell-based, so it's untouched by what's painted here.
    //
    // Children as QTreeView itself caches them: an unlisted folder claims
    // children so it shows a chevron; a listed empty one doesn't. Left-to-right
    // only — QTreeView mirrors the gutter under RTL, which Fernanda doesn't
    // support
    void drawBranches(
        QPainter* painter,
        const QRect& rect,
        const QModelIndex& index) const override
    {
        if (!model()->hasChildren(index)) {
            return;
        }

        auto indent = indentation();
        auto extent = qMin(TREE_CHEVRON_EXTENT, qMin(indent, rect.height()));
        if (extent <= 0) {
            return;
        }

        auto cell_left = rect.right() + 1 - indent;
        auto x = cell_left + (indent - extent) / 2;
        auto y = rect.top() + (rect.height() - extent) / 2;

        // The tint resolves against the view's current color group
        // (QWidget::palette() picks it), so an inactive window re-renders only
        // if its palette gives the role a different Inactive color
        auto expanded = isExpanded(index);
        const auto& cache = expanded ? chevronDown_ : chevronRight_;
        auto chevron = cache.pixmap(
            QString::fromLatin1(
                expanded ? CHEVRON_DOWN_ICON_PATH : CHEVRON_RIGHT_ICON_PATH),
            extent,
            palette().color(TREE_CHEVRON_ICON_ROLE),
            devicePixelRatioF());

        painter->drawPixmap(x, y, chevron);
    }

    // --- Drag source (TabBar's pattern) ------------------------------------

    void mousePressEvent(QMouseEvent* event) override
    {
        // A press off every row (indexAt), with either button, clears the
        // selection and the current row, as the system's file explorers do,
        // and does nothing else. Obsidian keeps its selection until another
        // entry is clicked. QTreeView isn't given the press: it finds a
        // folder's chevron by its whole indentation step, and the wash's inset
        // leaves the left edge of that step off the row, where a press would
        // still open or close the folder
        if (!indexAt(event->pos()).isValid()) {
            clearSelection();
            selectionModel()->clearCurrentIndex();
            return;
        }

        if (event->button() == Qt::LeftButton) {
            dragStartPos_ = event->pos();

            // Never from empty space — and never from the expand/collapse
            // arrow. indexAt answers for the row's wash, which takes in the
            // chevron's step left of the item rect, so without the x check a
            // small movement off the chevron would move the folder instead of
            // toggling it (Explorer and Obsidian both toggle)
            auto index = indexAt(event->pos());
            dragArmed_ =
                index.isValid() && event->pos().x() >= visualRect(index).left();
        }

        QTreeView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (dragArmed_ && (event->buttons() & Qt::LeftButton)) {
            auto delta = event->pos() - dragStartPos_;
            if (delta.manhattanLength() >= QApplication::startDragDistance()) {
                dragArmed_ = false; // one shot per press
                startEntryDrag_(dragStartPos_);
                return; // the drag consumed this gesture
            }
        }

        QTreeView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        dragArmed_ = false;
        QTreeView::mouseReleaseEvent(event);
    }

    // A double-click beside a row's wash is on no row (indexAt), so it does
    // nothing, as a press there does nothing but clear the selection.
    // QTreeView finds the row by height alone here, and would open or close a
    // folder
    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (!indexAt(event->pos()).isValid()) {
            return;
        }

        QTreeView::mouseDoubleClickEvent(event);
    }

    // --- Drop target -------------------------------------------------------

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        dragged_ = draggedEntryOf_(event);

        // Accept the ENTER for any in-vault entry drag, whatever the cursor is
        // over right now. This is load-bearing: a widget that IGNORES a
        // drag-enter gets no drag-move events after it, and a drag from this
        // very tree enters on the source's own row — a no-op spot — so gating
        // the enter on position would kill every subsequent move (the "No"
        // cursor). The real per-position accept/ignore is in dragMoveEvent
        if (!dragged_.valid) {
            event->ignore();
            return;
        }

        // Opt back into the base item view's drag machinery. Its auto-expand
        // timer only fires while state() is DraggingState, and nothing else
        // sets it here: the base sets it in the startDrag we never call
        setState(QAbstractItemView::DraggingState);

        event->acceptProposedAction();
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        // The base runs FIRST, for its side effects only: QTreeView restarts
        // its auto-expand timer here, and QAbstractItemView starts autoscroll
        // when the cursor nears an edge. Both ignore() the event by default
        // (our MIME is not in the model's mimeTypes), which is exactly why our
        // own accept/ignore has to come after — the last word wins
        QTreeView::dragMoveEvent(event);

        // The base also sets its hover row from indexAt on every move, which
        // paints the ordinary mouse-over band on whatever row the cursor
        // happens to be over. During a drag that band is a lie: the drop lands
        // in the destination FOLDER the overlay marks, not on the row being
        // pointed at, and two highlights disagreeing is worse than one
        clearHover_();

        // The base repaints only inside its can-decode branch, which our
        // private MIME never enters — so without this the hover paint goes
        // stale instead of disappearing with the cursor
        viewport()->update();

        // Resolve the hovered row ONCE: entryAt_ stats the path, and both the
        // accept decision and the highlight need the same answer. Two lookups
        // per move would be two stats and two chances to disagree
        auto index = indexAt(event->position().toPoint());
        auto entry = entryAt_(index);

        if (wouldMove_(destDirOf_(entry))) {
            event->acceptProposedAction();
            showOverlay_(index, entry);
        } else {
            event->ignore();
            hideOverlay_();
        }
    }

    void dragLeaveEvent(QDragLeaveEvent* event) override
    {
        endDrag_();
        QTreeView::dragLeaveEvent(event);
    }

    void dropEvent(QDropEvent* event) override
    {
        auto src = dragged_.absolute;
        auto dest_dir =
            destDirOf_(entryAt_(indexAt(event->position().toPoint())));
        auto moves = wouldMove_(dest_dir);

        endDrag_(); // clears dragged_, so read what we need first

        if (!moves) {
            event->ignore();
            return;
        }

        event->setDropAction(Qt::MoveAction);
        event->accept();
        emit moveRequested(src, dest_dir);
    }

private:
    VaultTreeModel* treeModel_;

    QWidget* overlay_ = nullptr; // the target highlight
    QPoint dragStartPos_{};
    bool dragArmed_ = false;

    // The branch chevrons. Cached because drawBranches runs once per visible
    // row per paint, and each render parses the SVG. Two caches, not one, since
    // collapsed and expanded rows alternate within a single paint. Drawn as
    // plain pixmaps, not QIcons, so selected rows get no generated wash: the
    // chevron stays muted on the selection band, like the extension badge
    Glyph::Cache chevronDown_{};
    Glyph::Cache chevronRight_{};

    // The entry under a viewport point, or the entry a drag is carrying: its
    // absolute path and whether it's a directory
    struct Entry_
    {
        Coco::Path absolute;
        bool isDir = false;
        bool valid = false;
    };

    // The dragged entry, resolved ONCE on drag-enter rather than re-parsed out
    // of the MIME and re-stat'd on every drag-move. isDir is a filesystem call,
    // and a network or cloud-synced path can block the UI thread — not
    // something to do per mouse move. Cleared on leave and on drop, so a stale
    // source can never survive into the next drag
    Entry_ dragged_{};

    void setup_()
    {
        setModel(treeModel_);

        // Part of the sidebar's surface: no frame of its own, and the window's
        // color behind the rows rather than the editor's (Base), so the sidebar
        // reads as one panel beside the text
        setFrameShape(QFrame::NoFrame);
        viewport()->setBackgroundRole(QPalette::Window);

        // Each file row's right-anchored extension badge
        setItemDelegate(new VaultTreeItemDelegate(treeModel_, this));

        // Drop target, but NOT a model-DnD drag source — see class note. We
        // initiate drags by hand in mouseMoveEvent
        setDragDropMode(QAbstractItemView::DropOnly);

        // We paint our own destination highlight, so the base's insertion-line
        // indicator would be a second, contradicting answer
        setDropIndicatorShown(false);

        // Hover a collapsed folder during a drag and it opens. QTreeView's own
        // timer does this; it just needs a non-negative delay and the
        // DraggingState we set in dragEnterEvent
        setAutoExpandDelay(TREE_AUTO_EXPAND_MS);
    }

    [[nodiscard]] Entry_ entryAt_(const QPoint& viewportPos) const
    {
        return entryAt_(indexAt(viewportPos));
    }

    // The entry at an index. The model answers both halves from its listing, so
    // a drag-move over rows costs no filesystem call
    [[nodiscard]] Entry_ entryAt_(const QModelIndex& index) const
    {
        if (!index.isValid()) {
            return {};
        }

        return { treeModel_->pathOf(index), treeModel_->isDir(index), true };
    }

    // Where a drop on this entry would move something INTO: a folder is its own
    // target; a file targets its parent folder; empty space (an invalid entry)
    // targets the vault root. Takes the resolved entry rather than a point, so
    // the caller stats once and the highlight reads the same answer this does
    [[nodiscard]] Coco::Path destDirOf_(const Entry_& entry) const
    {
        if (!entry.valid) {
            return treeModel_->root();
        }

        return entry.isDir ? entry.absolute : entry.absolute.parent();
    }

    // --- Drag initiation ---------------------------------------------------

    void startEntryDrag_(const QPoint& viewportPos)
    {
        auto entry = entryAt_(viewportPos);
        if (!entry.valid) {
            return;
        }

        auto* drag = new QDrag(this);
        drag->setMimeData(toVaultEntryDragMime({ entry.absolute }));

        auto pixmap = dragPixmap(entry.absolute.nameQString(), font());
        drag->setPixmap(pixmap);
        drag->setHotSpot({ pixmap.width() / 2, pixmap.height() / 2 });

        // Modal. A move-drop (this tree) or an open-drop (an editor pane)
        // consumes it; anything else — Explorer, a stray release — is a no-op.
        // Unlike a tab drag there is no pop-out fallback. One action: nothing
        // reads exec's result, so Move is just the honest label, no Copy needed
        drag->exec(Qt::MoveAction);

        // The view never sees the mouse release that ended this gesture — the
        // modal drag consumed it. Qt calls its own startDrag from inside the
        // base's mouseMoveEvent precisely so the base can reset afterwards;
        // ours is called from ours, so the press state QTreeView::
        // mousePressEvent set would otherwise linger into the next gesture
        setState(QAbstractItemView::NoState);
    }

    // --- Drop resolution ---------------------------------------------------

    // The entry a drag is carrying, if this tree could ever accept it: our MIME
    // (a foreign or OS drag decodes to nothing), exactly one entry (multi-
    // select is deferred), and a source living in THIS vault. Scope is what
    // refuses a cross-vault drag — the common tree's root differs.
    // Position-independent, so it's the right gate for opting in on drag-enter;
    // wouldMove_ then judges each position
    [[nodiscard]] Entry_ draggedEntryOf_(const QDropEvent* event) const
    {
        auto paths = fromVaultEntryDragMime(event->mimeData());
        if (paths.size() != 1) {
            return {};
        }

        auto absolute = paths.first();
        auto root = treeModel_->root();

        if (absolute.isEmpty() || absolute == root ||
            !absolute.isAtOrUnder(root)) {
            return {};
        }

        return { absolute, absolute.isDir(), true };
    }

    // Would a drop here actually move the dragged entry? The Vault re-checks
    // the second one authoritatively; these keep the accept state and the
    // highlight honest. The no-op guard lives here, not in the Vault — it's a
    // pure path comparison, and letting it reach the Vault would turn a
    // harmless nothing into a spurious failure warning
    [[nodiscard]] bool wouldMove_(const Coco::Path& destDir) const
    {
        if (!dragged_.valid) {
            return false;
        }

        // Already in destDir — a no-op (dropping a root-level entry on empty
        // space lands it back at root). Obsidian's own bug is failing this
        if (destDir == dragged_.absolute.parent()) {
            return false;
        }

        // A folder can't move into itself or one of its descendants
        if (dragged_.isDir && destDir.isAtOrUnder(dragged_.absolute)) {
            return false;
        }

        return true;
    }

    // One exit for every way a drag ends here — a leave, a drop, a refused
    // drop. Leaves nothing behind: no highlight, no cached source, and none of
    // the base's drag state (autoscroll runs on a timer that outlives the drag
    // if nobody stops it)
    void endDrag_()
    {
        hideOverlay_();
        dragged_ = {};
        stopAutoScroll();
        setState(QAbstractItemView::NoState);
    }

    // Tell the view the cursor is over no row. QAbstractItemView::viewportEvent
    // answers a HoverLeave by clearing its (private) hover index and repainting
    // the rows that changed — the same path a real cursor exit takes, so no
    // internals are being reached into. The positions are unused by that
    // handler; -1,-1 says "nowhere" honestly
    void clearHover_()
    {
        QHoverEvent leave(
            QEvent::HoverLeave,
            QPointF(-1, -1),
            QPointF(-1, -1),
            QPointF(-1, -1));

        QCoreApplication::sendEvent(viewport(), &leave);
    }

    // --- Highlight overlay ---------------------------------------------------

    void ensureOverlay_()
    {
        if (overlay_) {
            return;
        }

        // Parented to the viewport so its geometry is in viewport coords, the
        // same space visualRect and indexAt speak. Same look as the tab drop
        // overlay (TabPaneTree): one constant, two widgets
        overlay_ = new QWidget(viewport());
        overlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
        overlay_->setStyleSheet(DROP_OVERLAY_QSS);
        overlay_->hide();
    }

    void showOverlay_(const QModelIndex& index, const Entry_& entry)
    {
        ensureOverlay_();
        overlay_->setGeometry(highlightRect_(index, entry));
        overlay_->raise();
        overlay_->show();
    }

    void hideOverlay_()
    {
        if (overlay_) {
            overlay_->hide();
        }
    }

    // The destination folder's subtree — its own row plus every visible row
    // beneath it, Obsidian-style — or the whole tree when the destination is
    // the vault root (empty space, or a root-level entry's parent). Highlights
    // the true target folder, not the row under the cursor, so a drop onto a
    // file shows the parent it will land in. Takes the same resolved entry
    // destDirOf_ read, so the wash and the move can't disagree
    [[nodiscard]] QRect
    highlightRect_(const QModelIndex& index, const Entry_& entry) const
    {
        if (!entry.valid) {
            return viewport()->rect(); // empty space -> root
        }

        auto target = entry.isDir ? index : index.parent();
        if (!target.isValid() || target == rootIndex()) {
            return viewport()->rect(); // targets the vault root
        }

        return subtreeRect_(target);
    }

    // A folder's own row plus every visible row beneath it — Obsidian washes
    // the whole subtree, not just the header row. Visible is the operative
    // word: rows under a collapsed descendant aren't laid out, so walking the
    // last expanded child finds the block's true bottom
    [[nodiscard]] QRect subtreeRect_(const QModelIndex& folder) const
    {
        auto last = folder;
        while (isExpanded(last)) {
            auto count = model()->rowCount(last);
            if (count < 1) {
                break;
            }

            last = model()->index(count - 1, 0, last);
        }

        auto top = visualRect(folder);
        auto bottom = visualRect(last);

        return QRect(
                   0,
                   top.top(),
                   viewport()->width(),
                   bottom.bottom() - top.top() + 1)
            .intersected(viewport()->rect());
    }
};

} // namespace Fernanda::Ui
