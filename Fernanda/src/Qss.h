#pragma once

#include <QHash>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Example:
//
// clang-format off
// 
// QTreeView {
//     background-color: {{base}};    /* Required (line removed if missing) */
//     color: {{text}};               /* Required */
//     border: 1px solid {{border|}}; /* Optional (falls back to `1px solid;`) */
//     border-radius: {{radius|0}}px; /* Optional (falls back to 0) */
// }
//
// clang-format on
//
// Limitations:
// - A line with {{ in a comment would be processed
// - Line-based only, can't span variables across multiple lines

namespace Qss {

inline QString render(
    const QString& templateStyleSheet,
    const QHash<QString, QString>& assignments)
{
    QStringList output{};
    QRegularExpression var_pattern(R"(\{\{(\w+)(?:\|([^}]*))?\}\})");

    for (auto& line : templateStyleSheet.split('\n')) {

        QString processed = line;
        auto keep_line = true;

        for (auto& match : var_pattern.globalMatchView(line)) {
            auto name = match.captured(1);
            auto fallback = match.captured(2); // Empty if no pipe

            // Handle empty fallback like {{var|}}
            auto has_fallback =
                match.capturedLength(2) > 0 || line.contains("|}}");

            if (assignments.contains(name) && !assignments[name].isEmpty()) {
                processed.replace(match.captured(0), assignments[name]);
            } else if (has_fallback) {
                processed.replace(match.captured(0), fallback);
            } else {
                keep_line = false;
                break;
            }
        }

        if (keep_line) output << processed;
    }

    return output.join('\n');
}

} // namespace Qss
