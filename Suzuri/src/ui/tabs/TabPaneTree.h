/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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

#include <climits>
#include <functional>

#include <QApplication>
#include <QCursor>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMimeData>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/WorkspaceKeys.h"
#include "ui/UiConstants.h"
#include "ui/UiUtility.h"
#include "ui/sidebar/VaultEntryDragMime.h"
#include "ui/tabs/TabPaneLeaf.h"

namespace Suzuri::Ui {

// The split tree. A QWidget owning a *replaceable* root QSplitter — NOT a
// QSplitter itself. Splitting the root against its orientation wraps it in a
// new splitter; owning the root internally lets it be swapped without churning
// the window's central widget or invalidating the window's pointer to the tree.
//
// Structure: nested QSplitters are branches, TabPaneLeafs are leaves. A leaf
// holds N tabbed pages; the tree holds N leaves. Both window types host one.
//
// Invariant: the tree always contains at least one leaf. Collapsing the last
// occupied leaf leaves an empty leaf as the floor (its underlay shows), never
// an empty tree.
//
// Drag & drop is QDrag-lite: QDrag gives us cross-window drop routing for free,
// but the MIME carries only a marker — the moved page and its origin are read
// from the source tree via QDropEvent::source(), never serialized. The page
// moves only on a committed drop, so the source never sits empty mid-drag and
// the collapse rule needs no suppression. An invalid drop (no family target)
// pops the page out into a new window in the same vault family.
class TabPaneTree : public QWidget
{
    Q_OBJECT

public:
    explicit TabPaneTree(QWidget* parentWindow)
        : QWidget(parentWindow)
    {
        setup_();
    }

    ~TabPaneTree() override { TRACER; }

    // --- Queries -----------------------------------------------------------

    [[nodiscard]] TabPaneLeaf* activeLeaf() const noexcept
    {
        return activeLeaf_;
    }

    // Give keyboard focus to the active leaf's current page, if it has one.
    // In a window that isn't active, the page takes focus when the window
    // next activates
    void focusActivePage()
    {
        if (!activeLeaf_) {
            return;
        }

        if (auto* page = activeLeaf_->currentPage()) {
            page->setFocus();
        }
    }

    // Opaque identity of the window family (a VaultWindow and its pop-outs).
    // Set by VaultWindow on its own tree and every pop-out tree it makes; the
    // tree compares it by pointer identity and never dereferences it, so no
    // type coupling to VaultWindow
    [[nodiscard]] QObject* familyId() const noexcept { return familyId_; }
    void setFamilyId(QObject* id) noexcept { familyId_ = id; }

    // Whether the center zone adds the page as a tab to the hovered leaf (true)
    // or is treated as invalid and pops out (false, default). A toggle for
    // feeling out which behaviour is right
    void setCenterAddsTab(bool enabled) noexcept { centerAddsTab_ = enabled; }

    // Which dropped files this tree will open. The window sets it — only IT
    // knows which vaults it hosts — and the tree asks once per drag, so a file
    // from a foreign vault gets the "No" cursor instead of a valid-looking
    // highlight and a silent nothing on release. Unset accepts nothing: a tree
    // nobody wired can't resolve a path to a model anyway
    void setEntryOpenFilter(std::function<bool(const Coco::Path&)> filter)
    {
        entryOpenFilter_ = std::move(filter);
    }

    // True when any leaf holds a page — false at the empty floor. WorkspaceFile
    // uses it on restore to discard a pop-out whose files all vanished
    [[nodiscard]] bool hasPages() const { return !isEmpty_(); }

    // Whether any leaf in this tree currently holds page. VaultWindow uses it
    // to find which of its trees hosts a NewTabPage at event time, so the
    // page's buttons act on wherever it lives now — not a tree captured when it
    // was made
    [[nodiscard]] bool containsPage(QWidget* page) const
    {
        return leafFor_(page) != nullptr;
    }

    // --- Pages (forwarded to the right leaf) -------------------------------

    // New pages land in the active leaf — a sidebar-activated view, or a
    // "+"-spawned NewTabPage the window just made
    void addToActiveLeaf(QWidget* page, const QString& title)
    {
        ASSERT(activeLeaf_, "The tree always has an active leaf!");
        activeLeaf_->addPage(page, title);
    }

    // Page-addressed, so the window never has to know which leaf holds a page.
    // This is the new-tab -> editor swap: same tab, page replaced
    void replacePage(QWidget* oldPage, QWidget* newPage, const QString& title)
    {
        if (auto* leaf = leafFor_(oldPage)) {
            leaf->replacePage(oldPage, newPage, title);
            emit layoutChanged();
        }
    }

    // Page-addressed close. Emptying a leaf triggers collapse via
    // pageCountChanged — unless it's the last leaf, which is the floor
    void closePage(QWidget* page)
    {
        if (auto* leaf = leafFor_(page)) {
            leaf->closePage(leaf->indexOf(page));
        }
    }

    // --- Splits ------------------------------------------------------------

    // Split the active leaf, adding a fresh empty leaf beside (Horizontal) or
    // below (Vertical) it. The newcomer becomes active, ready to be filled
    void splitActiveLeaf(Qt::Orientation orientation)
    {
        if (activeLeaf_) {
            setActiveLeaf_(splitFor_(activeLeaf_, orientation, false));
        }
    }

    // --- Layout persistence -------------------------------------------

    // A page rebuilt from a persisted tab. VaultWindow's buildPage supplies
    // both halves — the widget and the title its tab should carry — so the tree
    // can re-home it without knowing what a page IS. A null page means "drop
    // this tab": the file no longer exists on disk
    struct RestoredPage
    {
        QWidget* page = nullptr;
        QString title{};
    };

    // Serialize the whole tree to a JSON node. Recursive: splits carry
    // orientation, sizes, and children; leaves carry their tab list and active
    // index. describePage turns each page into a tab entry — or returns a null
    // value to omit it (an unknown page type). Page-type-ignorant: the tree
    // never learns what a page is
    [[nodiscard]] QJsonObject
    serialize(const std::function<QJsonValue(QWidget*)>& describePage) const
    {
        return serializeSplitter_(root_, describePage);
    }

    void restore(
        const QJsonObject& node,
        const std::function<RestoredPage(const QJsonValue&)>& buildPage)
    {
        // See restoring_: keep the pageCountChanged handler off the tree until
        // the swap below makes root_ current
        restoring_ = true;

        QSplitter* new_root = nullptr;

        if (node.value(WorkspaceKeys::TYPE).toString() ==
            WorkspaceKeys::TYPE_LEAF) {
            new_root = makeSplitter_(Qt::Horizontal);
            auto* leaf = makeLeaf_();
            new_root->addWidget(leaf);
            populateLeaf_(leaf, node, buildPage);
        } else {
            new_root = buildSplitter_(node, buildPage);
        }

        QList<TabPaneLeaf*> restored{};
        gatherLeaves_(new_root, restored);
        if (restored.isEmpty()) {
            new_root->deleteLater();
            restoring_ = false;
            return; // keep the pristine default tree
        }

        layout_->removeWidget(root_);
        root_->hide();
        root_->deleteLater();

        layout_->addWidget(new_root);
        root_ = new_root;
        activeLeaf_ = restored.value(0);

        restoring_ = false;

        // The restored leaves filled while another leaf was still the active
        // one, so none of their page changes were announced (makeLeaf_)
        emit activePageChanged(activeLeaf_->currentPage());
    }

signals:
    // A leaf's "+" was clicked; the tree has already made that leaf active. The
    // window responds by adding a NewTabPage to the active leaf. Named for the
    // intent, not for the button
    void newTabRequested();

    // A tab was dropped somewhere invalid (no family target). The window
    // creates a PopoutWindow at globalPos and hosts the page there. The page is
    // already detached from its origin leaf when this fires
    void popOutRequested(
        QWidget* page,
        const QString& title,
        const QPoint& globalPos);

    // A file was dropped on an editor pane to open it. This tree has already
    // made the dropped-on leaf active, so the window opens into that leaf. The
    // window resolves which vault owns absolute and opens it replace-active
    // (Obsidian: takes the active tab unless pinned, else a new tab). Files
    // only — a folder drag is a move the file tree handles, and never reaches
    // an editor (acceptsEntryOpen_ rejects directories)
    void fileDropped(const Coco::Path& absolute);

    // The page being worked in changed: the active leaf's current page, which
    // moves when the active leaf changes (focus, a split, a collapse, restore)
    // or when that leaf shows a different page (TabPaneLeaf::
    // currentPageChanged). nullptr when the active leaf is empty. Can repeat
    // the same page, as the leaf's signal can; consumers compare. The window's
    // status bar follows this (BaseWindow). Page-type-ignorant like the rest
    // of the tree — the window casts
    void activePageChanged(QWidget* page);

    // The tree collapsed to its empty floor — its last page anywhere just
    // closed. A VaultWindow lets this pass (an empty floor is a valid resting
    // state); a PopoutWindow connects it to close itself
    void emptied();

    // Structure, tab membership, or a page swap changed — WorkspaceFile
    // debounces a workspace.json write on this. Deliberately coarse: "something
    // persist-worthy moved", not what. Active-tab and cursor/scroll changes
    // don't fire it — they're captured at the guaranteed close/exit save —
    // which keeps this to structural session edits
    void layoutChanged();

protected:
    // --- Drop routing (target side) ----------------------------------------

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        // A file dragged from a vault tree, to open here. A distinct MIME from
        // a tab drag, and its source is a VaultTreeView, not a TabPaneTree — so
        // it's handled entirely apart from the family-scoped tab path below
        entryDragged_ = entryOpenOf_(event);
        if (hasEntryOpen_()) {
            window()->raise();
            window()->activateWindow();
            entryDragOver_(event);
            return;
        }

        if (!accepts_(event)) {
            event->ignore();
            return;
        }

        // A drag over us brings our window forward. Kept local — we only ever
        // hover our own family
        window()->raise();
        window()->activateWindow();

        event->acceptProposedAction();
        updateOverlay_(event->position().toPoint());
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        if (hasEntryOpen_()) {
            entryDragOver_(event);
            return;
        }

        auto zone = resolve_(event);

        // Center-in-popout-mode is not a valid in-tree drop: ignore it so the
        // release falls through to the source's pop-out path
        if (!accepts_(event) || zone == Zone_::Invalid) {
            event->ignore();
            hideOverlay_();
            return;
        }

        event->acceptProposedAction();
        showOverlay_(leafAt_(event->position().toPoint()), zone);
    }

    void dragLeaveEvent(QDragLeaveEvent* event) override
    {
        hideOverlay_();
        entryDragged_ = {};
        QWidget::dragLeaveEvent(event);
    }

    void dropEvent(QDropEvent* event) override
    {
        hideOverlay_();

        // A file dropped to open here: open it in the leaf it landed on. Make
        // that leaf active first (the hovered leaf, not whatever was active),
        // then hand the path up — the window opens it replace-active
        if (hasEntryOpen_()) {
            auto absolute = entryDragged_;
            entryDragged_ = {};

            auto* leaf = leafAt_(event->position().toPoint());
            if (!leaf) {
                event->ignore();
                return;
            }

            setActiveLeaf_(leaf);
            event->acceptProposedAction();
            emit fileDropped(absolute);
            return;
        }

        auto zone = resolve_(event);
        if (!accepts_(event) || zone == Zone_::Invalid) {
            event->ignore();
            return;
        }

        auto* target = leafAt_(event->position().toPoint());
        auto* src = qobject_cast<TabPaneTree*>(event->source());
        if (!target || !src || !src->drag_.page || !src->drag_.leaf) {
            event->ignore();
            return;
        }

        auto* page = src->drag_.page;
        auto title = src->drag_.title;
        auto* src_leaf = src->drag_.leaf;

        // A leaf's only tab dropped back on itself: nothing to do
        if (src_leaf == target && src_leaf->count() == 1) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        if (zone == Zone_::TabBar || zone == Zone_::Center) {
            // Add as a tab to the hovered leaf. At the end for now —
            // TODO: insert at the drop position instead
            src->takeFromOrigin_();
            target->addPage(page, title);
            setActiveLeaf_(target);
            activateWindow_();
        } else {
            auto orientation =
                (zone == Zone_::EdgeLeft || zone == Zone_::EdgeRight)
                    ? Qt::Horizontal
                    : Qt::Vertical;
            auto before = (zone == Zone_::EdgeLeft || zone == Zone_::EdgeTop);

            // Split first (reads target), then move the page in — so a self
            // edge-split with other tabs present keeps the origin non-empty
            auto* fresh = splitFor_(target, orientation, before);
            src->takeFromOrigin_();
            fresh->addPage(page, title);
            setActiveLeaf_(fresh);
            activateWindow_();
        }

        event->setDropAction(Qt::MoveAction);
        event->accept();
    }

private:
    enum class Zone_
    {
        TabBar,
        EdgeLeft,
        EdgeRight,
        EdgeTop,
        EdgeBottom,
        Center,
        Invalid // center in pop-out mode — not a valid in-tree drop
    };

    struct DragState_
    {
        TabPaneLeaf* leaf = nullptr;
        QWidget* page = nullptr;
        QString title{};
    };

    static constexpr auto MIME_TYPE_ = "application/x-suzuri-tab";
    static constexpr qreal EDGE_FRACTION_ = 0.30;

    QVBoxLayout* layout_ = nullptr;
    QSplitter* root_ = nullptr;
    TabPaneLeaf* activeLeaf_ = nullptr;

    QObject* familyId_ = nullptr;
    QWidget* overlay_ = nullptr;
    DragState_ drag_;
    bool centerAddsTab_ = false;

    // The file an entry drag is carrying, resolved once on drag-enter: our
    // MIME, exactly one entry, a regular file (a folder drag is a move the file
    // tree handles, never an open), and openable per entryOpenFilter_. Cleared
    // when the drag ends
    std::function<bool(const Coco::Path&)> entryOpenFilter_;
    Coco::Path entryDragged_{};

    // True only while restore() builds a detached subtree and swaps it in. The
    // pageCountChanged handler must not act on the half-installed tree — root_
    // still points at the old subtree until the swap, so isEmpty_() reads stale
    bool restoring_ = false;

    // --- Construction / wiring ---------------------------------------------

    void setup_()
    {
        setAcceptDrops(true);

        layout_ = new QVBoxLayout(this);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->setSpacing(0);

        // Root begins as a one-leaf splitter. Orientation is arbitrary with a
        // single child; the first split against it picks the real one
        root_ = makeSplitter_(Qt::Horizontal);
        layout_->addWidget(root_);

        auto* first = makeLeaf_();
        root_->addWidget(first);
        activeLeaf_ = first;

        // Active-leaf tracking is focus-driven: whenever focus lands anywhere
        // inside one of our leaves, that leaf becomes active. focusChanged is
        // app-global (fires for every window, including sibling trees in
        // pop-outs), so isAncestorOf filters it down to our own subtree
        connect(
            qApp,
            &QApplication::focusChanged,
            this,
            &TabPaneTree::onFocusChanged_);
    }

    TabPaneLeaf* makeLeaf_()
    {
        auto* leaf = new TabPaneLeaf; // parented on insertion into a splitter

        // "+" -> make this leaf active, then re-emit as intent
        connect(leaf, &TabPaneLeaf::addPageRequested, this, [this, leaf] {
            setActiveLeaf_(leaf);
            emit newTabRequested();
        });

        // A tab dragged off its bar becomes a QDrag originating here
        connect(
            leaf,
            &TabPaneLeaf::detachRequested,
            this,
            [this, leaf](QWidget* page) { startDrag_(leaf, page); });

        // The active leaf showing a different page is the window's active page
        // changing. Any other leaf's page change isn't; it's announced if and
        // when that leaf becomes active (setActiveLeaf_)
        connect(
            leaf,
            &TabPaneLeaf::currentPageChanged,
            this,
            [this, leaf](QWidget* page) {
                if (leaf == activeLeaf_) {
                    emit activePageChanged(page);
                }
            });

        connect(leaf, &TabPaneLeaf::pageCountChanged, this, [this, leaf] {
            // During restore the tree is built detached and root_ is swapped
            // only at the end, so isEmpty_() would read the OLD root here and
            // spuriously fire emptied() — which a pop-out has wired to close
            // itself, killing restored pop-outs mid-build. Restore installs
            // exactly the serialized shape, so there's nothing to collapse or
            // signal until it's live
            if (restoring_) {
                return;
            }

            if (leaf->isEmpty()) {
                collapse_(leaf);
            }

            if (isEmpty_()) {
                emit emptied();
            }

            emit layoutChanged();
        });

        // A reorder within the leaf changes persisted tab order but not
        // structure — forward it straight to layoutChanged
        connect(
            leaf,
            &TabPaneLeaf::tabsReordered,
            this,
            &TabPaneTree::layoutChanged);

        // Pin/unpin is persisted, so a toggle schedules a save too
        connect(
            leaf,
            &TabPaneLeaf::pinChanged,
            this,
            &TabPaneTree::layoutChanged);

        return leaf;
    }

    QSplitter* makeSplitter_(Qt::Orientation orientation)
    {
        auto* splitter = new QSplitter(orientation);

        // A pane dragged to zero width via the handle would vanish with no way
        // back. Collapse is an explicit operation here, not a drag artifact
        splitter->setChildrenCollapsible(false);

        return splitter;
    }

    // A tab dropped here from another window leaves that window the active
    // one, and a widget takes keyboard focus only in the active window. The
    // dropped page asked for focus as it was added (TabPaneLeaf::addPage), so
    // activating this window gives it to the page
    void activateWindow_()
    {
        window()->raise();
        window()->activateWindow();
    }

    // --- Active leaf -------------------------------------------------------

    void setActiveLeaf_(TabPaneLeaf* leaf)
    {
        if (!leaf || leaf == activeLeaf_) {
            return;
        }

        activeLeaf_ = leaf;
        emit activePageChanged(leaf->currentPage());
    }

    void onFocusChanged_(QWidget* /*old*/, QWidget* now)
    {
        if (!now || !isAncestorOf(now)) {
            return;
        }

        if (auto* leaf = leafContaining_(now)) {
            setActiveLeaf_(leaf);
        }
    }

    // Walk up from w to the TabPaneLeaf that contains it, or null
    TabPaneLeaf* leafContaining_(QWidget* w) const
    {
        while (w && w != this) {
            if (auto* leaf = qobject_cast<TabPaneLeaf*>(w)) {
                return leaf;
            }
            w = w->parentWidget();
        }

        return nullptr;
    }

    // The leaf whose tab set contains page, or null
    TabPaneLeaf* leafFor_(QWidget* page) const
    {
        for (auto* leaf : leaves_()) {
            if (leaf->indexOf(page) > -1) {
                return leaf;
            }
        }

        return nullptr;
    }

    // The leaf under a tree-local point, or null
    TabPaneLeaf* leafAt_(QPoint treePos) const
    {
        for (auto* leaf : leaves_()) {
            if (leaf->rect().contains(leaf->mapFrom(this, treePos))) {
                return leaf;
            }
        }

        return nullptr;
    }

    // Every leaf in the tree, in visual order, by walking the splitter
    // structure
    QList<TabPaneLeaf*> leaves_() const
    {
        QList<TabPaneLeaf*> out{};
        gatherLeaves_(root_, out);
        return out;
    }

    // True when no leaf holds a page — the tree has collapsed to its empty
    // floor (a single childless leaf showing its underlay)
    [[nodiscard]] bool isEmpty_() const
    {
        for (auto* leaf : leaves_()) {
            if (!leaf->isEmpty()) {
                return false;
            }
        }

        return true;
    }

    void gatherLeaves_(QSplitter* splitter, QList<TabPaneLeaf*>& out) const
    {
        for (auto i = 0; i < splitter->count(); ++i) {
            auto* child = splitter->widget(i);

            if (auto* leaf = qobject_cast<TabPaneLeaf*>(child)) {
                out.append(leaf);
            } else if (auto* sub = qobject_cast<QSplitter*>(child)) {
                gatherLeaves_(sub, out);
            }
        }
    }

    // --- Split -------------------------------------------------------------

    // Add a fresh empty leaf beside target on the requested orientation, before
    // or after it. Returns the fresh leaf. Same-orientation splits extend the
    // existing splitter, halving only the target's slot (neighbours keep their
    // sizes, keyed by widget so the before/after choice can't scramble
    // indices); a cross-orientation split wraps target in a new branch
    TabPaneLeaf*
    splitFor_(TabPaneLeaf* target, Qt::Orientation orientation, bool before)
    {
        auto* parent = qobject_cast<QSplitter*>(target->parentWidget());
        ASSERT(parent, "A leaf's parent is always a QSplitter!");

        auto* fresh = makeLeaf_();
        auto index = parent->indexOf(target);

        if (parent->count() == 1) {
            // Single-child splitter (a fresh root): adopt the orientation and
            // add. Two children, split evenly
            parent->setOrientation(orientation);
            before ? parent->insertWidget(0, fresh) : parent->addWidget(fresh);
            equalize_(parent);
        } else if (parent->orientation() == orientation) {
            // Same direction: extend in place. Capture the pre-split
            // distribution keyed by widget, insert, then hand target + fresh
            // each half of target's old slot while every neighbour keeps its
            // own
            QHash<QWidget*, int> pre{};
            auto pre_sizes = parent->sizes();
            for (auto i = 0; i < parent->count(); ++i) {
                pre.insert(parent->widget(i), pre_sizes[i]);
            }

            parent->insertWidget(before ? index : index + 1, fresh);

            auto half = qMax(1, pre.value(target) / 2);
            QList<int> sizes{};
            for (auto i = 0; i < parent->count(); ++i) {
                auto* w = parent->widget(i);
                sizes << ((w == target || w == fresh) ? half : pre.value(w));
            }
            parent->setSizes(sizes);
        } else {
            // Cross direction: wrap target in a new branch, holding target +
            // fresh evenly. The branch inherits target's slot in the parent, so
            // neighbours are undisturbed
            auto sizes = parent->sizes();
            auto* branch = makeSplitter_(orientation);
            parent->insertWidget(index, branch); // target pushed to index+1

            if (before) {
                branch->addWidget(fresh);
                branch->addWidget(target); // reparents target out of parent
            } else {
                branch->addWidget(target); // reparents target out of parent
                branch->addWidget(fresh);
            }

            parent->setSizes(sizes); // count restored to n; distribution held
            equalize_(branch);
        }

        emit layoutChanged();
        return fresh;
    }

    // --- Layout (de)serialization -------------------------------------

    QJsonObject serializeSplitter_(
        QSplitter* splitter,
        const std::function<QJsonValue(QWidget*)>& describePage) const
    {
        QJsonArray children{};
        QJsonArray sizes{};
        const auto splitter_sizes = splitter->sizes();

        for (auto i = 0; i < splitter->count(); ++i) {
            auto* w = splitter->widget(i);

            if (auto* leaf = qobject_cast<TabPaneLeaf*>(w)) {
                children.append(serializeLeaf_(leaf, describePage));
            } else if (auto* sub = qobject_cast<QSplitter*>(w)) {
                children.append(serializeSplitter_(sub, describePage));
            } else {
                continue;
            }

            sizes.append(splitter_sizes.value(i));
        }

        QJsonObject obj{};
        obj[WorkspaceKeys::TYPE] = WorkspaceKeys::TYPE_SPLIT;
        obj[WorkspaceKeys::ORIENTATION] =
            (splitter->orientation() == Qt::Vertical)
                ? WorkspaceKeys::ORIENTATION_V
                : WorkspaceKeys::ORIENTATION_H;
        obj[WorkspaceKeys::SIZES] = sizes;
        obj[WorkspaceKeys::CHILDREN] = children;
        return obj;
    }

    QJsonObject serializeLeaf_(
        TabPaneLeaf* leaf,
        const std::function<QJsonValue(QWidget*)>& describePage) const
    {
        QJsonArray tabs{};
        auto* current = leaf->currentPage();
        auto active = -1;

        for (auto i = 0; i < leaf->count(); ++i) {
            auto* page = leaf->pageAt(i);
            auto value = describePage(page);
            if (value.isNull()) {
                continue; // an unknown page type — dropped
            }

            if (page == current) {
                active = tabs.size(); // index into the FILTERED list
            }
            tabs.append(value);
        }

        QJsonObject obj{};
        obj[WorkspaceKeys::TYPE] = WorkspaceKeys::TYPE_LEAF;
        obj[WorkspaceKeys::ACTIVE] =
            active; // -1 when empty or the current page was dropped
        obj[WorkspaceKeys::TABS] = tabs;
        return obj;
    }

    QSplitter* buildSplitter_(
        const QJsonObject& node,
        const std::function<RestoredPage(const QJsonValue&)>& buildPage)
    {
        auto orientation = (node.value(WorkspaceKeys::ORIENTATION).toString() ==
                            WorkspaceKeys::ORIENTATION_V)
                               ? Qt::Vertical
                               : Qt::Horizontal;

        auto* splitter = makeSplitter_(orientation);

        const auto children = node.value(WorkspaceKeys::CHILDREN).toArray();
        for (const auto& child : children) {
            auto obj = child.toObject();
            auto type = obj.value(WorkspaceKeys::TYPE).toString();

            if (type == WorkspaceKeys::TYPE_LEAF) {
                auto* leaf = makeLeaf_();
                splitter->addWidget(leaf);
                populateLeaf_(leaf, obj, buildPage);
            } else if (type == WorkspaceKeys::TYPE_SPLIT) {
                splitter->addWidget(buildSplitter_(obj, buildPage));
            }
        }

        // Never leave a childless splitter — at least one leaf, always
        if (splitter->count() == 0) {
            splitter->addWidget(makeLeaf_());
        }

        // Sizes apply pre-show (QSplitter caches them). Trust only a matching
        // count; otherwise fall through to Qt's default distribution
        const auto sizes_array = node.value(WorkspaceKeys::SIZES).toArray();
        if (sizes_array.size() == splitter->count()) {
            QList<int> sizes{};
            for (const auto& value : sizes_array) {
                sizes << value.toInt();
            }
            splitter->setSizes(sizes);
        }

        return splitter;
    }

    void populateLeaf_(
        TabPaneLeaf* leaf,
        const QJsonObject& node,
        const std::function<RestoredPage(const QJsonValue&)>& buildPage)
    {
        const auto tabs = node.value(WorkspaceKeys::TABS).toArray();
        auto active_index = node.value(WorkspaceKeys::ACTIVE).toInt(-1);

        QWidget* active_page = nullptr;

        for (auto i = 0; i < tabs.size(); ++i) {
            auto built = buildPage(tabs.at(i));
            if (!built.page) {
                continue; // missing file — dropped silently
            }

            leaf->addPage(built.page, built.title);
            if (i == active_index) {
                active_page = built.page;
            }
        }

        // Restore the active tab if it survived; otherwise the leaf keeps
        // whatever addPage made current last — the persisted active file was
        // deleted between sessions, a benign edge
        if (active_page) {
            leaf->setCurrentPage(active_page);
        }
    }

    // --- Collapse ----------------------------------------------------------

    void collapse_(TabPaneLeaf* leaf)
    {
        auto* parent = qobject_cast<QSplitter*>(leaf->parentWidget());
        if (!parent) {
            return;
        }

        // The floor: never remove the tree's sole leaf. An empty leaf showing
        // its underlay is a valid resting state
        if (parent == root_ && parent->count() == 1) {
            return;
        }

        if (activeLeaf_ == leaf) {
            activeLeaf_ = nullptr;
        }

        // Detach before deleteLater so the splitter forgets it immediately — no
        // empty frame lingers until the event loop runs the deletion
        leaf->setParent(nullptr);
        leaf->deleteLater();

        normalize_(parent);

        if (!activeLeaf_) {
            setActiveLeaf_(leaves_().value(0));
        }
    }

    // After a removal, a splitter may be left with a single child. A redundant
    // one-child branch must be collapsed into its parent, or invisible
    // splitters accumulate and corrupt saved workspaces
    void normalize_(QSplitter* splitter)
    {
        if (splitter->count() != 1) {
            return;
        }

        auto* only = splitter->widget(0);

        if (splitter == root_) {
            // Root with one leaf is the valid base state — leave it. Root
            // wrapping a single *branch* is redundant: promote the branch to be
            // the new root. This is exactly why the root is owned, not
            // inherited
            if (auto* branch = qobject_cast<QSplitter*>(only)) {
                branch->setParent(nullptr);
                layout_->removeWidget(root_);
                root_->hide();
                root_->deleteLater();

                layout_->addWidget(branch); // reparents branch into this
                root_ = branch;
            }

            return;
        }

        // Non-root: replace the splitter with its only child in the
        // grandparent, preserving the grandparent's distribution (its count is
        // unchanged)
        auto* grandparent = qobject_cast<QSplitter*>(splitter->parentWidget());
        ASSERT(
            grandparent,
            "A non-root branch's parent is always a QSplitter!");

        auto index = grandparent->indexOf(splitter);
        auto sizes = grandparent->sizes();

        grandparent->insertWidget(index, only); // reparents only up
        splitter->setParent(nullptr);
        splitter->deleteLater();

        grandparent->setSizes(sizes);
    }

    // --- Sizing ------------------------------------------------------------

    // Give every child of a splitter an equal share. Before first show the
    // extent is 0 and the equal fallback still distributes evenly once shown
    void equalize_(QSplitter* splitter)
    {
        auto n = splitter->count();
        if (n < 1) {
            return;
        }

        auto extent = (splitter->orientation() == Qt::Horizontal)
                          ? splitter->width()
                          : splitter->height();

        auto share = extent > 0 ? extent / n : 1;
        splitter->setSizes(QList<int>(n, share));
    }

    // --- Drag (source side) ------------------------------------------------

    void startDrag_(TabPaneLeaf* origin, QWidget* page)
    {
        if (!page || origin->indexOf(page) < 0) {
            return;
        }

        drag_ = { origin, page, origin->titleOf(page) };

        // Source = this tree, so a family target reads us via event->source().
        // The MIME is a bare marker — the real data is drag_, read directly
        // (same class, same process, one active drag)
        auto* drag = new QDrag(this);
        auto* mime = new QMimeData;
        mime->setData(QString::fromLatin1(MIME_TYPE_), QByteArray{});
        drag->setMimeData(mime);

        auto pixmap = dragPixmap(drag_.title, font());
        drag->setPixmap(pixmap);
        drag->setHotSpot({ pixmap.width() / 2, pixmap.height() / 2 });

        // Modal loop. Any family target's dropEvent runs (and takes the page)
        // before this returns. We do NOT remove the tab up front, so the origin
        // stays non-empty until a committed drop
        auto result = drag->exec(Qt::MoveAction);

        if (result != Qt::MoveAction) {
            // Nothing in the family accepted — pop out. Detach here (source
            // side), since no target did
            takeFromOrigin_();
            emit popOutRequested(drag_.page, drag_.title, QCursor::pos());
        }

        drag_ = {};
    }

    // Detach the dragged page from its origin leaf without deleting it. Safe to
    // call twice — indexOf returns -1 once it's gone
    void takeFromOrigin_()
    {
        if (!drag_.leaf || !drag_.page) {
            return;
        }

        if (auto index = drag_.leaf->indexOf(drag_.page); index > -1) {
            (void)drag_.leaf->takePage(index);
        }
    }

    // --- Drop resolution (target side) -------------------------------------

    bool accepts_(QDropEvent* event) const
    {
        if (!event->mimeData()->hasFormat(QString::fromLatin1(MIME_TYPE_))) {
            return false;
        }

        auto* src = qobject_cast<TabPaneTree*>(event->source());
        return src && src->familyId_ == familyId_; // same vault family only
    }

    [[nodiscard]] Coco::Path entryOpenOf_(const QDropEvent* event) const
    {
        auto paths = fromVaultEntryDragMime(event->mimeData());
        if (paths.size() != 1) {
            return {};
        }

        auto absolute = paths.first();
        if (!absolute.isFile()) {
            return {};
        }

        if (!entryOpenFilter_ || !entryOpenFilter_(absolute)) {
            return {};
        }

        return absolute;
    }

    [[nodiscard]] bool hasEntryOpen_() const
    {
        return !entryDragged_.isEmpty();
    }

    // A file open highlights the whole hovered leaf — it opens there, with no
    // edge-split or tab-position nuance a tab drag has. Center's highlight rect
    // is the whole leaf, so it's reused as-is
    void entryDragOver_(QDropEvent* event)
    {
        auto* leaf = leafAt_(event->position().toPoint());
        if (leaf) {
            event->acceptProposedAction();
            showOverlay_(leaf, Zone_::Center);
        } else {
            event->ignore();
            hideOverlay_();
        }
    }

    // The zone under an event's position. Invalid means "not a valid in-tree
    // drop" (center in pop-out mode) — the caller ignores it so it pops out
    Zone_ resolve_(QDropEvent* event) const
    {
        auto* leaf = leafAt_(event->position().toPoint());
        if (!leaf) {
            return Zone_::Invalid;
        }

        auto zone =
            zoneFor_(leaf, leaf->mapFrom(this, event->position().toPoint()));
        if (zone == Zone_::Center && !centerAddsTab_) {
            return Zone_::Invalid;
        }

        return zone;
    }

    Zone_ zoneFor_(TabPaneLeaf* leaf, QPoint p) const
    {
        // Tab-bar strip takes precedence over the edge bands
        if (p.y() < leaf->headerHeight()) {
            return Zone_::TabBar;
        }

        QRect content(
            0,
            leaf->headerHeight(),
            leaf->width(),
            leaf->height() - leaf->headerHeight());

        auto left = p.x() - content.left();
        auto right = content.right() - p.x();
        auto top = p.y() - content.top();
        auto bottom = content.bottom() - p.y();
        auto band_x = int(content.width() * EDGE_FRACTION_);
        auto band_y = int(content.height() * EDGE_FRACTION_);

        struct Cand
        {
            Zone_ zone{};
            int distance = 0;
            bool in_band = false;
        };

        // Nearest in-band edge wins, which resolves corners deterministically
        Cand cands[] = {
            { Zone_::EdgeLeft,   left,   left < band_x   },
            { Zone_::EdgeRight,  right,  right < band_x  },
            { Zone_::EdgeTop,    top,    top < band_y    },
            { Zone_::EdgeBottom, bottom, bottom < band_y },
        };

        auto best = Zone_::Center;
        auto best_distance = INT_MAX;
        for (const auto& c : cands) {
            if (c.in_band && c.distance < best_distance) {
                best_distance = c.distance;
                best = c.zone;
            }
        }

        return best;
    }

    // --- Drop indicator ----------------------------------------------------

    void ensureOverlay_()
    {
        if (overlay_) {
            return;
        }

        overlay_ = new QWidget(this);
        overlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
        overlay_->setStyleSheet(DROP_OVERLAY_QSS);
        overlay_->hide();
    }

    void showOverlay_(TabPaneLeaf* leaf, Zone_ zone)
    {
        if (!leaf) {
            hideOverlay_();
            return;
        }

        ensureOverlay_();
        overlay_->setGeometry(highlightRect_(leaf, zone));
        overlay_->raise();
        overlay_->show();
    }

    void hideOverlay_()
    {
        if (overlay_) {
            overlay_->hide();
        }
    }

    void updateOverlay_(QPoint treePos)
    {
        auto* leaf = leafAt_(treePos);
        if (!leaf) {
            hideOverlay_();
            return;
        }

        auto zone = zoneFor_(leaf, leaf->mapFrom(this, treePos));
        if (zone == Zone_::Center && !centerAddsTab_) {
            hideOverlay_();
            return;
        }

        showOverlay_(leaf, zone);
    }

    // The region the drop would fill, in tree coords: the whole leaf for a tab
    // add, the relevant half for an edge split
    QRect highlightRect_(TabPaneLeaf* leaf, Zone_ zone)
    {
        QRect r(leaf->mapTo(this, QPoint(0, 0)), leaf->size());

        switch (zone) {
        case Zone_::EdgeLeft:
            return { r.left(), r.top(), r.width() / 2, r.height() };
        case Zone_::EdgeRight:
            return { r.center().x(),
                     r.top(),
                     r.width() - r.width() / 2,
                     r.height() };
        case Zone_::EdgeTop:
            return { r.left(), r.top(), r.width(), r.height() / 2 };
        case Zone_::EdgeBottom:
            return { r.left(),
                     r.center().y(),
                     r.width(),
                     r.height() - r.height() / 2 };
        default:
            return r; // TabBar or Center(add): the whole leaf
        }
    }
};

} // namespace Suzuri::Ui
