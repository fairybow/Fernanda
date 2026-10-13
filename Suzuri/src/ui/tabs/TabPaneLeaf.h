/*
 * Suzuri — A plain-text editor for creative writing
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

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QPoint>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QString>
#include <QStyle>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"
#include "ui/tabs/NewTabButton.h"
#include "ui/tabs/PageIcon.h"
#include "ui/tabs/PagePin.h"
#include "ui/tabs/TabBar.h"
#include "ui/tabs/TabCloseButton.h"
#include "ui/widgets/Glyph.h"

namespace Suzuri::Ui {

// One leaf of the split tree: a tab bar over a stack of pages. Pages are plain
// QWidget* — a file view, or the empty new-tab page — never AbstractFileView*.
// A new tab has no file, so anything asking "what file is in this tab?" does a
// checked cast and handles null.
//
// The page pointer lives in the tab's data and rides through reorders, so the
// page stack is addressed only by pointer (setCurrentWidget), never by index.
// There is no bar/stack ordering to keep aligned.
//
// Two-level stack: mainStack_ shows the underlay when there are no tabs, and
// the page stack otherwise. The underlay is the no-tabs backdrop, distinct from
// the new-tab page (itself a real page in a real tab).
//
// Closing the last tab leaves the leaf empty; collapse-when-empty is
// TabPaneTree's job, so an empty leaf is a valid transient state here
class TabPaneLeaf : public QWidget
{
    Q_OBJECT

public:
    TabPaneLeaf()
        : QWidget(nullptr)
    {
        setup_();
    }

    // Set the teardown flag BEFORE ~QWidget deletes the pages, so a page dying
    // as part of the leaf's own destruction doesn't try to prune a tab from a
    // half-destroyed bar (see bindPageLifetime_)
    ~TabPaneLeaf() override
    {
        TRACER;
        tearingDown_ = true;
    }

    // --- Queries -----------------------------------------------------------

    [[nodiscard]] int count() const { return bar_->count(); }
    [[nodiscard]] int currentIndex() const { return bar_->currentIndex(); }
    [[nodiscard]] bool isEmpty() const { return bar_->count() == 0; }

    [[nodiscard]] QWidget* pageAt(int index) const
    {
        return qvariant_cast<QWidget*>(bar_->tabData(index));
    }

    [[nodiscard]] QWidget* currentPage() const
    {
        return pageAt(bar_->currentIndex());
    }

    [[nodiscard]] int indexOf(QWidget* page) const
    {
        for (auto i = 0; i < bar_->count(); ++i) {
            if (pageAt(i) == page) {
                return i;
            }
        }

        return -1;
    }

    // Title currently shown for a page's tab, or empty. The drag carries this
    // so the tab can be recreated at the destination
    [[nodiscard]] QString titleOf(QWidget* page) const
    {
        auto index = indexOf(page);
        return index > -1 ? bar_->tabText(index) : QString{};
    }

    // Y-offset where the page area begins — i.e. the height of the tab-bar row.
    // The tree gives this strip drop-precedence over the edge zones
    [[nodiscard]] int headerHeight() const { return mainStack_->y(); }

    // --- Tabs --------------------------------------------------------------

    // Takes ownership of page (parented into the stack) and makes it current
    void addPage(QWidget* page, const QString& title)
    {
        ASSERT(page, "addPage requires a page!");

        pageStack_->addWidget(page);
        bindPage_(page);

        auto index = bar_->addTab(title);
        bar_->setTabData(index, QVariant::fromValue(page));
        applyPageIcon_(index, page);
        bar_->setCurrentIndex(index);

        // addTab on an empty bar emits currentChanged(0) before the data above
        // is set, which onCurrentChanged_ ignores (null page). Sync explicitly
        // now that the tab is fully formed
        onCurrentChanged_(bar_->currentIndex());
        updateEmptyState_();

        // Install the close/unpin button on the new tab. A freshly opened view
        // is unpinned (shows the close X); a page arriving by drag or restore
        // carries its pin on the widget (PagePin), so the button renders the
        // pin here for free — it reads the page's bit live
        installCloseButton_(index, page);

        // Move focus into the page — its editor, a NewTabPage's primary action,
        // a PDF view. setFocus follows the page's own focus proxy, so the leaf
        // stays page-type-ignorant. This is what makes a freshly added or
        // dropped tab immediately typable
        page->setFocus();

        emit pageCountChanged();
    }

    // Swap the page under an existing tab in place — same index, same tab. This
    // is how the new-tab page becomes the editor once a file is chosen: the tab
    // stays put, its content changes. Count is unchanged
    void replacePage(QWidget* oldPage, QWidget* newPage, const QString& title)
    {
        ASSERT(newPage, "replacePage requires a new page!");

        auto index = indexOf(oldPage);
        if (index < 0) {
            return;
        }

        auto was_current = (bar_->currentIndex() == index);

        // The tab persists across a content swap, so its pin does too: a pinned
        // New-tab page turned into a file stays pinned
        auto was_pinned = isPagePinned(oldPage);

        pageStack_->addWidget(newPage);
        bindPage_(newPage);
        bar_->setTabData(index, QVariant::fromValue(newPage));
        bar_->setTabText(index, title);
        applyPageIcon_(index, newPage);
        setPagePinned(newPage, was_pinned);

        // The tab (and its close button) persist across a content swap — only
        // the page under them changes. Repoint the button at the new page so
        // its glyph and the leaf's click branch both read the new page's pin
        // bit. The click lambda captures the button, not the page, so no
        // reconnect is needed
        repointCloseButton_(index, newPage);

        if (oldPage) {
            pageStack_->removeWidget(oldPage);
            oldPage->deleteLater();
        }

        // The bar index didn't change, so currentChanged won't fire — show the
        // replacement explicitly if this tab was the visible one
        if (was_current) {
            onCurrentChanged_(index);
        }
    }

    // Detach the page at index and return it WITHOUT deleting — the tab is
    // removed and the page reparented out so the caller can re-home it (a tab
    // drag to another leaf/window). Emits pageCountChanged, which may collapse
    // this leaf if it empties. Returns null on a bad index
    [[nodiscard]] QWidget* takePage(int index)
    {
        if (index < 0 || index >= bar_->count()) {
            return nullptr;
        }

        auto* page = pageAt(index);
        bar_->removeTab(index);

        if (page) {
            pageStack_->removeWidget(page);
            page->setParent(nullptr);

            // This page is leaving under an explicit removal (its tab is
            // already gone above), so drop its bindings to us — both the
            // lifetime and title connections from bindPage_. A re-homing
            // addPage rebinds at the destination; a self-destruct never reaches
            // takePage, so its binding stays live to prune the orphaned tab
            disconnect(page, nullptr, this, nullptr);
        }

        updateEmptyState_();
        emit pageCountChanged();
        return page;
    }

    // Remove and delete the page at index. Deleting a TextFileView just
    // unregisters its document from the prime (via QObject::destroyed); the
    // buffer is the Vault's and survives. This is the single close path — the
    // close button (when unpinned) and the context menu's Close both land here.
    //
    // Closing a tab is a pure view operation: no dirty-state prompt (no
    // user-facing dirty file), and the prime auto-unregisters a destroyed view
    // doc, so there's no manual cleanup. The buffer is the Vault's and outlives
    // the view.
    //
    // Deliberately no flush at tab close. The buffer persists and is written by
    // autosave and window close; its final per-file flush comes when its last
    // view closes (Vault eviction), not here (a shared file may still be open
    // in another tab or window, so per-tab flushing would be both redundant and
    // premature)
    void closePage(int index)
    {
        if (auto* page = takePage(index)) {
            page->deleteLater();
        }
    }

    void setCurrentPage(QWidget* page)
    {
        if (auto index = indexOf(page); index > -1) {
            bar_->setCurrentIndex(index);
        }
    }

    void setPageTitle(QWidget* page, const QString& title)
    {
        if (auto index = indexOf(page); index > -1) {
            bar_->setTabText(index, title);
        }
    }

signals:
    // The + button was clicked. VaultWindow makes a NewTabPage and adds it —
    // the leaf stays ignorant of page types
    void addPageRequested();

    // A page was added or removed. TabPaneTree collapses an emptied leaf on
    // this
    void pageCountChanged();

    // A tab was dragged clear of the bar. The tree turns this into a QDrag
    // (page-addressed, so the tree never touches the bar). Distinct from
    // closeRequested — a detach *moves* the page, it doesn't destroy it
    void detachRequested(QWidget* page); // TODO: Do we need this?

    // A tab was reordered within this leaf (QTabBar's built-in horizontal move,
    // enabled by TabBar's setMovable). Order changed, count didn't — so it's
    // distinct from pageCountChanged (which drives collapse). The tree persists
    // layout on it
    void tabsReordered();

    // A tab was pinned or unpinned (context menu, or a click on a pinned tab's
    // button). Pin state is persisted, so the tree forwards this to
    // layoutChanged. Order and count are unchanged, so it's neither
    // tabsReordered nor pageCountChanged
    void pinChanged();

    // The page this leaf shows changed: a tab switch, an add, a replace, or
    // the current tab leaving (close, drag-away, a file view destroying
    // itself). nullptr once the leaf has no tabs. Emitted from the one place
    // the shown page is set (onCurrentChanged_), so it can repeat the same
    // page — removing a tab left of the current one shifts the index without
    // changing the page — and consumers compare. The tree turns this into its
    // window's active page (TabPaneTree::activePageChanged)
    void currentPageChanged(QWidget* page);

protected:
    // A mark is rendered once into the tab's icon, in the colors of the moment,
    // so whatever changes those colors renders every tab's mark again: a new
    // palette or style (an OS light/dark switch), which reaches every widget in
    // the window, and the window gaining or losing focus, which moves the
    // palette between its Active and Inactive colors
    void changeEvent(QEvent* event) override
    {
        QWidget::changeEvent(event);

        if (event->type() == QEvent::PaletteChange ||
            event->type() == QEvent::StyleChange ||
            event->type() == QEvent::ActivationChange) {
            for (auto i = 0; i < bar_->count(); ++i) {
                applyPageIcon_(i, pageAt(i));
            }
        }
    }

private:
    TabBar* bar_ = new TabBar(this);
    NewTabButton* addButton_ = new NewTabButton(this);
    QStackedWidget* mainStack_ = new QStackedWidget(this);
    QStackedWidget* pageStack_ = new QStackedWidget(this);
    QWidget* underlay_ = new QWidget(this);

    // See ~TabPaneLeaf and bindPageLifetime_
    bool tearingDown_ = false;

    // Every mark today is the same glyph, so one slot serves every tab
    Glyph::Cache markGlyph_{};

    // Which tab side the close/unpin button occupies — set in setup_ from the
    // style (see there). addPage installs the button here
    QTabBar::ButtonPosition closeSide_ = QTabBar::RightSide;

    void setup_()
    {
        addButton_->setFocusPolicy(Qt::NoFocus);
        connect(addButton_, &QAbstractButton::clicked, this, [this] {
            emit addPageRequested();
        });

        underlay_->setAttribute(Qt::WA_StyledBackground); // QSS target later
        mainStack_->addWidget(underlay_);                 // index 0
        mainStack_->addWidget(pageStack_);                // index 1

        // Tabs, then the + button hugging their right edge, then a stretch that
        // eats the slack. The bar takes its sizeHint width (sum of preferred
        // tab widths), so the button slides right as tabs are added; when the
        // tabs would overflow, TabBar::minimumSizeHint (width 0) lets the bar
        // collapse under the button, pinning it to the right edge while the bar
        // scrolls. addStretch is a layout item, so no spacer widget is needed
        auto* top = new QHBoxLayout;
        top->setContentsMargins(TAB_ROW_LEFT_MARGIN, 0, 0, 0);
        top->setSpacing(0);
        top->addWidget(bar_, 0);
        top->addLayout(addButtonColumn_(), 0);
        top->addStretch(1);

        // Hold the row one tab tall regardless of the bar's visibility.
        // updateEmptyState_ hides the bar while there are no tabs; a hidden
        // widget contributes nothing to the layout, so without this the row
        // would collapse to the + button's height and lift the button out of
        // the vertical centering it has against the tabs. A zero-width Fixed
        // spacer reserves the bar's height (TAB_HEIGHT, plus the gap the style
        // leaves above every tab) and nothing else — no horizontal effect, so
        // the button still pins to the right edge. It's a layout item, not a
        // widget (same reasoning as the stretch above). TAB_HEIGHT < 0 means
        // Qt-default tab sizing, where we don't know the height, so skip it
        if (TAB_HEIGHT >= 0) {
            top->addItem(new QSpacerItem(
                0,
                TAB_HEIGHT + STYLE_TAB_TOP_GAP,
                QSizePolicy::Fixed,
                QSizePolicy::Fixed));
        }

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addLayout(top, 0);
        layout->addWidget(mainStack_, 1);

        connect(
            bar_,
            &QTabBar::currentChanged,
            this,
            &TabPaneLeaf::onCurrentChanged_);

        // A tab pressed focuses its page, whether it was already current (the
        // lone tab of an inactive leaf) or just became current. Only a press
        // does: a tab made current any other way (a restore, a tab closing)
        // leaves focus where it is
        connect(bar_, &TabBar::tabPressed, this, [this](int index) {
            if (auto* page = pageAt(index)) {
                page->setFocus();
            }
        });

        // A vertical drag-off becomes a page-level detach. The tree owns what
        // happens next (QDrag, drop routing, pop-out); the leaf just names the
        // page that left
        connect(bar_, &TabBar::detachDragged, this, [this](int index) {
            if (auto* page = pageAt(index)) {
                emit detachRequested(page);
            }
        });

        // A horizontal reorder shuffles tab order without changing the count.
        // Surface it so the tree can persist the new order. The page pointers
        // ride in tabData, so pageAt already reflects the move — this carries
        // no payload, just the fact that something moved
        connect(bar_, &QTabBar::tabMoved, this, [this](int, int) {
            emit tabsReordered();
        });

        // Which side the close/unpin button occupies — right on Windows/Linux,
        // left on macOS, per the style. addPage installs the button there; the
        // pin lives on this same button (it swaps glyph), so there is no
        // separate indicator to place. Computed once from the style
        closeSide_ =
            static_cast<QTabBar::ButtonPosition>(bar_->style()->styleHint(
                QStyle::SH_TabBar_CloseButtonPosition,
                nullptr,
                bar_));

        // Right-click a tab: Close, then Pin/Unpin. Handled here in the leaf,
        // which has the page and the pin helpers; TabBar keeps its "no chrome"
        // promise and gains nothing
        bar_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(
            bar_,
            &QWidget::customContextMenuRequested,
            this,
            &TabPaneLeaf::onTabContextMenu_);

        updateEmptyState_();
    }

    // Bind a page's lifetime AND title to its tab. Lifetime: every ordinary
    // exit (closePage/takePage/replacePage) re-points or removes the tab before
    // the page dies, so the destroyed handler no-ops there — it exists for the
    // one path that skips them, a file view self-destructing because its file
    // was deleted on disk, dropping the orphaned tab and letting the tree
    // collapse. Title: pages carry their display name in windowTitle, so a
    // rename (fanned out from the shared model) reaches the tab here without
    // the leaf learning it holds a file view. takePage drops both bindings when
    // a page leaves under an explicit move; addPage rebinds at the destination.
    // Guarded against teardown so a leaf dying with tabs leaves the bar alone
    // (~TabPaneLeaf set the flag first)
    void bindPage_(QWidget* page)
    {
        connect(page, &QObject::destroyed, this, [this, page] {
            if (tearingDown_) {
                return;
            }

            if (auto index = indexOf(page); index > -1) {
                bar_->removeTab(index);
                updateEmptyState_();
                emit pageCountChanged();
            }
        });

        connect(
            page,
            &QWidget::windowTitleChanged,
            this,
            [this, page](const QString& title) { setPageTitle(page, title); });
    }

    // Underlay when empty, pages otherwise. Also hides the bar when empty — an
    // empty tab bar is just a stray line. The + button stays visible so the
    // first tab can be created
    void updateEmptyState_()
    {
        auto has_tabs = bar_->count() > 0;
        bar_->setVisible(has_tabs);
        mainStack_->setCurrentWidget(has_tabs ? pageStack_ : underlay_);
    }

    // Right-click on a tab: Close, a separator, then Pin or Unpin depending on
    // the page's current state. tabAt returns -1 on empty bar space, where
    // there is nothing to act on
    void onTabContextMenu_(const QPoint& pos)
    {
        auto index = bar_->tabAt(pos);
        if (index < 0) {
            return;
        }

        QMenu menu(this);

        connect(
            menu.addAction(tr("Close")),
            &QAction::triggered,
            this,
            [this, index] { closePage(index); });

        menu.addSeparator();

        auto pinned = isPagePinned(pageAt(index));
        connect(
            menu.addAction(pinned ? tr("Unpin") : tr("Pin")),
            &QAction::triggered,
            this,
            [this, index] { togglePin_(index); });

        menu.exec(bar_->mapToGlobal(pos));
    }

    // Flip the page's pin flag, refresh its button, and announce it so the tree
    // can persist (pinChanged -> layoutChanged). The flag lives on the page
    // (PagePin), so it rides drags and restores with the widget
    void togglePin_(int index)
    {
        auto* page = pageAt(index);
        if (!page) {
            return;
        }

        setPagePinned(page, !isPagePinned(page));
        refreshCloseButton_(index);
        emit pinChanged();
    }

    // Install the close/unpin button on a tab, bound to its page. The lambda
    // captures the BUTTON (not the page) so a later repoint on replacePage is
    // enough — no reconnect. Qt owns tab buttons and deleteLater()s this one
    // when the tab is removed, so its lifetime tracks the tab and the lambda
    // can never fire on a dangling page
    void installCloseButton_(int index, QWidget* page)
    {
        if (index < 0 || index >= bar_->count()) {
            return;
        }

        auto* button = new TabCloseButton(page, bar_);
        connect(button, &QAbstractButton::clicked, this, [this, button] {
            onCloseButtonClicked_(button);
        });
        bar_->setTabButton(index, closeSide_, button);
    }

    // Draw the page's tab mark, if it has one (PageIcon). Called wherever a tab
    // gains a page (addPage for a new or dropped one, replacePage for the
    // new-tab -> editor swap) and for every tab when the colors change
    // (changeEvent). A page with no mark clears the tab's icon, so a marked
    // page replaced by an unmarked one leaves nothing behind. The leaf never
    // asks what the mark means.
    //
    // The color is the leaf's, which the bar shares (neither sets a palette of
    // its own). It's read from the leaf, not the bar, because a palette change
    // reaches a parent before its children: changeEvent runs this while the
    // bar still holds the old palette
    void applyPageIcon_(int index, QWidget* page)
    {
        if (index < 0 || index >= bar_->count()) {
            return;
        }

        auto path = pageIconPath(page);

        if (path.isEmpty()) {
            bar_->setTabIcon(index, QIcon{});
            return;
        }

        bar_->setTabIcon(
            index,
            markGlyph_.icon(
                path,
                TAB_ICON_EXTENT,
                palette().color(TAB_MARK_ICON_ROLE),
                devicePixelRatioF()));
    }

    // Repoint an existing tab's button at a new page (replacePage: the tab
    // persists, the page under it swaps). setPage refreshes the glyph
    void repointCloseButton_(int index, QWidget* page)
    {
        if (auto* button = closeButtonAt_(index)) {
            button->setPage(page);
        }
    }

    // Repaint a tab's button after its page's pin bit changed elsewhere (the
    // context menu). The button reads the bit live; it just needs to be told to
    // repaint
    void refreshCloseButton_(int index)
    {
        if (auto* button = closeButtonAt_(index)) {
            button->update();
        }
    }

    // The + button, centered on the tabs as they're drawn rather than on the
    // whole row: the style leaves a gap above every tab (STYLE_TAB_TOP_GAP), so
    // the button's column has the same gap above it. NEW_TAB_BUTTON_LEFT_GAP
    // keeps its hover wash off the last tab, and NEW_TAB_BUTTON_RIGHT_GAP keeps
    // it off the pane's right edge once the tabs fill the row: the column
    // holds that space, so the bar gives it up instead
    [[nodiscard]] QVBoxLayout* addButtonColumn_()
    {
        auto* column = new QVBoxLayout;
        column->setContentsMargins(
            NEW_TAB_BUTTON_LEFT_GAP,
            STYLE_TAB_TOP_GAP,
            NEW_TAB_BUTTON_RIGHT_GAP,
            0);
        column->setSpacing(0);
        column->addWidget(addButton_, 0, Qt::AlignVCenter);
        return column;
    }

    [[nodiscard]] TabCloseButton* closeButtonAt_(int index) const
    {
        if (index < 0 || index >= bar_->count()) {
            return nullptr;
        }

        return qobject_cast<TabCloseButton*>(
            bar_->tabButton(index, closeSide_));
    }

    // A click on a tab's close/unpin button. Pinned → unpin in place (the tab
    // stays; the button's glyph flips back to the close X on repaint) and
    // announce it so the tree can persist. Unpinned → close the tab. The branch
    // lives here, not on the button, mirroring how QTabBar keeps close handling
    // in the bar rather than the button. The page is resolved from the button,
    // and indexOf scans tabData by pointer, so a prior reorder can't stale it.
    //
    // Deleting a tab (the close branch) goes through removeTab, which
    // deleteLater()s this button — safe to trigger from inside its own
    // clicked()
    void onCloseButtonClicked_(TabCloseButton* button)
    {
        auto* page = button->page();
        if (!page) {
            return;
        }

        if (isPagePinned(page)) {
            setPagePinned(page, false);
            button->update();
            emit pinChanged();
        } else {
            closePage(indexOf(page));
        }
    }

    void onCurrentChanged_(int index)
    {
        if (index < 0) {
            setFocusProxy(nullptr);
            emit currentPageChanged(nullptr);
            return;
        }

        auto* page = pageAt(index);
        if (!page) {
            return;
        }

        pageStack_->setCurrentWidget(page);
        setFocusProxy(page);
        emit currentPageChanged(page);
    }
};

} // namespace Suzuri::Ui
