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

#include <QtTypes>

namespace Fernanda {

inline constexpr auto WORKSPACE_FILE_NAME = "workspace.json";

// Whether a stable build is offered prereleases when it checks for updates. A
// prerelease build always is (Updater, which passes the answer to
// UpdateCheck::availableUpdate)
inline constexpr bool STABLE_BUILDS_SEE_PRERELEASES = true;

// Where Updater asks for the list of releases: GitHub's releases endpoint for
// Fernanda's repository
inline constexpr auto UPDATE_RELEASES_URL =
    "https://api.github.com/repos/fairybow/Fernanda/releases";

// When Updater checks on its own: once, this long after launch (so the check
// doesn't compete with restoring the session), then at this interval for as
// long as Fernanda runs. A check that hears nothing for TIMEOUT gives up
inline constexpr int UPDATE_FIRST_CHECK_DELAY_MS = 5 * 1000;
inline constexpr int UPDATE_CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000;
inline constexpr int UPDATE_CHECK_TIMEOUT_MS = 15 * 1000;

// How long a GitRepository being destroyed waits for a running git command to
// finish before stopping it. A step that rewrites files in the working tree (a
// switch, a pull's merge, a discard's restore) gets the longer wait: stopped
// partway, it would leave files half updated
inline constexpr int GIT_EXIT_WAIT_MS = 3 * 1000;
inline constexpr int GIT_REWRITE_EXIT_WAIT_MS = 30 * 1000;

// How long a GitRepository lets a fetch or a push run before it stops it, so
// a sign-in that can't be shown, or a remote that never answers, can't hold
// the repository's other commands back forever. Neither touches the working
// tree, so stopping one leaves no file half written
inline constexpr int GIT_NETWORK_TIMEOUT_MS = 2 * 60 * 1000;

// How GitDiff::wordChanges marks the words that changed in a line. A line
// marks nothing when less than MIN_UNCHANGED_PERCENT of its characters
// (leaving out whitespace) are unchanged. The middles of two lines, after
// their matching ends, are compared in a table of one cell per pair of words;
// past MAX_CELLS (a few megabytes, held only while comparing) the middle is
// marked as one span instead
inline constexpr int GIT_WORD_DIFF_MIN_UNCHANGED_PERCENT = 50;
inline constexpr qint64 GIT_WORD_DIFF_MAX_CELLS = 1000 * 1000;

} // namespace Fernanda
