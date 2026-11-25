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

#include <optional>

#include <QJsonObject>
#include <QList>
#include <QVariant>
#include <QWidget>
#include <QtTypes>

#include "core/files/PathKey.h"
#include "core/git/GitStatus.h"

namespace Fernanda::Ui {

// A tab's back and forward history: the pages it showed before the one it
// shows now, and the ones it went back from. Each tab has its own, as in
// Obsidian. It lives on the page widget, as the pin (PagePin) and the mark
// (PageIcon) do, so it rides every reorder, drag, and pop-out with the page,
// and VaultWindow hands it to whatever page replaces it.
//
// An entry records a page as data, not a widget, so a page left behind is
// destroyed like any other. A file view is its file and the view's own state
// (AbstractFileView::writeViewState: cursor, scroll, zoom), and a diff is the
// change it showed. Going back builds the page again from its entry. A text
// file's buffer is kept a while after its last view closes (BufferStore), so
// a rebuilt view finds its undo history.
//
// The history lasts the session. It isn't saved with the workspace
class TabHistory
{
public:
    // How many entries each list keeps. Past it, the farthest one is dropped,
    // so a tab used all day can't grow without limit
    static constexpr qsizetype MAX_ENTRIES = 50;

    // Which way to go: to the newest back entry, or the next forward one
    enum class Step
    {
        Back,
        Forward
    };

    struct Entry
    {
        enum class Kind
        {
            File,
            Diff
        };

        Kind kind = Kind::File;

        // A file view's file and its state. Empty for a diff
        PathKey file{};
        QJsonObject viewState{};

        // A diff's change. Unused for a file
        GitStatus::Entry change{};
    };

    // Whether there's an entry to go to that way. An entry may turn out not to
    // build (its file gone), so true means a step can be tried, not that it
    // will land
    [[nodiscard]] bool canGoBack() const noexcept { return !back_.isEmpty(); }

    [[nodiscard]] bool canGoForward() const noexcept
    {
        return !forward_.isEmpty();
    }

    // A new page took the tab over from the one recorded: that page is the
    // step back, and the forward list is cleared, as a browser's is
    void record(const Entry& left)
    {
        pushBack(left);
        forward_.clear();
    }

    // Add to the back list, leaving the forward list as it is, as going
    // forward does with the page it leaves
    void pushBack(const Entry& entry) { push_(back_, entry); }

    // Add to the forward list, as going back does with the page it leaves
    void pushForward(const Entry& entry) { push_(forward_, entry); }

    // The newest entry on the back list, removed from it. None when it's empty
    [[nodiscard]] std::optional<Entry> takeBack() { return take_(back_); }

    // The next entry on the forward list, removed from it. None when it's
    // empty
    [[nodiscard]] std::optional<Entry> takeForward() { return take_(forward_); }

    // A file was renamed or moved, or a folder it's in was: every file entry
    // at or under oldEntry, on either list, takes its new key. Diffs are left
    // as they are, since a diff's change is named by Git, which reports a
    // moved file as a new change
    void rebase(const PathKey& oldEntry, const PathKey& newEntry)
    {
        rebase_(back_, oldEntry, newEntry);
        rebase_(forward_, oldEntry, newEntry);
    }

private:
    // Each oldest first, so the next to go to is last
    QList<Entry> back_{};
    QList<Entry> forward_{};

    static void push_(QList<Entry>& list, const Entry& entry)
    {
        list << entry;

        while (list.size() > MAX_ENTRIES) {
            list.removeFirst();
        }
    }

    [[nodiscard]] static std::optional<Entry> take_(QList<Entry>& list)
    {
        if (list.isEmpty()) {
            return std::nullopt;
        }

        return list.takeLast();
    }

    static void rebase_(
        QList<Entry>& list,
        const PathKey& oldEntry,
        const PathKey& newEntry)
    {
        for (auto& entry : list) {
            if (entry.kind == Entry::Kind::File &&
                entry.file.isAtOrUnder(oldEntry)) {
                entry.file = entry.file.rebased(oldEntry, newEntry);
            }
        }
    }
};

inline constexpr auto PAGE_HISTORY_PROPERTY = "fernanda_pageHistory";

// Empty for a page that has none, and for no page
[[nodiscard]] inline TabHistory pageHistory(QWidget* page)
{
    return page ? page->property(PAGE_HISTORY_PROPERTY).value<TabHistory>()
                : TabHistory{};
}

// Set by VaultWindow when a page replaces another, and when the files in a
// history are renamed or moved
inline void setPageHistory(QWidget* page, const TabHistory& history)
{
    if (page) {
        page->setProperty(PAGE_HISTORY_PROPERTY, QVariant::fromValue(history));
    }
}

} // namespace Fernanda::Ui
