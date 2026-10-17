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

#include <QFontDatabase>
#include <QSet>
#include <QString>
#include <QStringList>

#include <Coco/Debug.h>
#include <Coco/Path.h>

// The fonts Suzuri ships, compiled in from resources/Fonts.qrc and registered
// with the application's font database at startup, so any vault can name them
// whether or not they're installed on the machine.
//
// Registration walks the whole :/fonts tree rather than listing folders, so
// adding a font is a Fonts.qrc edit and, if it's a new family, one line in
// families() below. families() is the one list that still has to agree with
// the files by hand — it's what the font picker puts first (and dedups from
// the system list), and it can't be derived from the files alone without
// registering them first. registerFonts() checks the two agree and warns on
// drift, so a stale entry shows up in the log instead of as a picker item that
// silently renders in a fallback font
namespace Fernanda::BundledFonts {

using namespace Qt::StringLiterals;

// Family names exactly as the font files declare them (what
// QFontDatabase::applicationFontFamilies reports), in picker order
[[nodiscard]] inline const QStringList& families()
{
    static const QStringList families_ = { u"Courier Prime"_s,
                                           u"Literata"_s,
                                           u"mononoki"_s,
                                           u"OpenDyslexic"_s };
    return families_;
}

// Register every bundled font file with the application font database. Call
// once, after logging is up (App::init) and before any window builds a view.
// A file that fails to load is warned and skipped — text in that family falls
// back to a system font, nothing else breaks
inline void registerFonts()
{
    auto files =
        Coco::allFilePaths(Coco::Path(":/fonts"), { u"*.ttf"_s, u"*.otf"_s });

    QSet<QString> registered{};

    for (const auto& file : files) {
        auto id = QFontDatabase::addApplicationFont(file.toQString());
        if (id < 0) {
            WARN("Failed to load bundled font: {}", file);
            continue;
        }

        for (const auto& family : QFontDatabase::applicationFontFamilies(id)) {
            registered << family;
        }
    }

    for (const auto& family : families()) {
        if (!registered.contains(family)) {
            WARN(
                "Bundled family \"{}\" is listed but no font file declared "
                "it!",
                family);
        }
    }

    DEBUG(
        "Registered {} bundled font file(s): {}",
        files.size(),
        QStringList(registered.cbegin(), registered.cend()).join(u", "_s));
}

} // namespace Fernanda::BundledFonts
