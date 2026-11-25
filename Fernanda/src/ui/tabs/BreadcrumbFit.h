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

#include <QList>
#include <QtMinMax>
#include <QtTypes>

namespace Fernanda::Ui::Breadcrumbs {

// How wide each part of a path is drawn when the whole path doesn't fit: the
// folders, outermost first, each followed by a separator, then the name. Pure
// arithmetic on widths, in pixels, so the header (TabHeader) measures and
// draws and this decides.
//
// The name comes first: it's drawn whole while it fits at all, and shortened
// only once there's no room left for any folder. The folders share what's
// left, as Obsidian's breadcrumbs do: a folder narrower than its share keeps
// its full width, and the rest are shortened to equal shares, each no shorter
// than its minimum (a letter and an ellipsis, as the caller measures it). The
// widths drawn never add up to more than the width available.
//
// When even the minimums don't fit, folders are dropped from the inside out,
// keeping the outermost, and one ellipsis stands where they were. With no
// room even for that, only the name is drawn

struct Fit
{
    // How wide to draw the name
    int name = 0;

    // How wide to draw each folder kept, outermost first. The rest, if any,
    // are dropped
    QList<int> folders{};

    // Whether an ellipsis stands for the dropped folders, after those kept
    bool dropped = false;
};

[[nodiscard]] inline Fit
fit(int available,
    int name,
    const QList<int>& folders,
    const QList<int>& minimums,
    int separator,
    int ellipsis)
{
    Fit out{};

    if (available <= 0) {
        return out;
    }

    out.name = qMin(name, available);

    auto room = available - name;
    if (room <= 0) {
        return out;
    }

    auto count = folders.size();

    auto full = 0;
    for (auto width : folders) {
        full += width + separator;
    }

    if (full <= room) {
        out.folders = folders;
        return out;
    }

    // The most folders, outermost first, that fit at their minimums, with
    // the ellipsis's room when any are dropped
    auto needed = [&](qsizetype kept) {
        auto width = 0;

        for (qsizetype i = 0; i < kept; ++i) {
            width += qMin(minimums.value(i), folders.value(i)) + separator;
        }

        if (kept < count) {
            width += ellipsis + separator;
        }

        return width;
    };

    auto kept = count;
    while (kept > 0 && needed(kept) > room) {
        --kept;
    }

    if (needed(kept) > room) {
        return out;
    }

    out.dropped = kept < count;

    // Share out what's left, settling folders in two rounds, the share worked
    // out again after each pass over the rest. First, a folder whose minimum
    // is at least the share keeps its minimum; settling one can only lower
    // the share, so the rounds can't undo each other. Then a folder that fits
    // in the share keeps its full width; settling one can only raise it. The
    // minimums go first: a folder kept whole at a share the minimums later
    // lowered would push the path past its width
    auto left =
        room - kept * separator - (out.dropped ? ellipsis + separator : 0);

    QList<int> widths(kept, -1);
    auto unsettled = kept;

    auto settle_where = [&](const auto& settledWidth) {
        auto settling = true;

        while (settling && unsettled > 0) {
            settling = false;
            auto share = left / unsettled;

            for (qsizetype i = 0; i < kept; ++i) {
                if (widths[i] >= 0) {
                    continue;
                }

                auto width = settledWidth(i, share);
                if (width < 0) {
                    continue;
                }

                widths[i] = width;
                left -= width;
                --unsettled;
                settling = true;
            }
        }
    };

    settle_where([&](qsizetype i, int share) {
        auto minimum = qMin(minimums.value(i), folders[i]);
        return minimum >= share ? minimum : -1;
    });

    settle_where([&](qsizetype i, int share) {
        return folders[i] <= share ? folders[i] : -1;
    });

    // The folders still unsettled all take the share, the first few a pixel
    // more until the remainder is used. Each is wider than its minimum, since
    // the first round left none whose minimum reached the share
    if (unsettled > 0) {
        auto share = left / unsettled;
        auto extra = left % unsettled;

        for (qsizetype i = 0; i < kept; ++i) {
            if (widths[i] < 0) {
                widths[i] = share + (extra > 0 ? 1 : 0);
                --extra;
            }
        }
    }

    out.folders = widths;
    return out;
}

} // namespace Fernanda::Ui::Breadcrumbs
