#pragma once

#include <QHash>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Example:
//
// clang-format off
// 
// base|#f5f5f5                           /* Global default (consumed, not output) */
// 
// Widget {
//     background-color: {{base}};        /* Will default to global base (`#f5f5f5`) */
//     selection-color: {{base|#0000ff}}; /* Will default to `#0000ff` */
//     text-color: {{text}};              /* Required (line removed if missing) */
//     border: 1px solid {{border|}};     /* Optional (falls back to `1px solid;`) */
//     border-radius: {{radius|0}}px;     /* Optional (falls back to `0`) */
// }
//
// clang-format on
//
// Limitations:
// - A line with `{{` in a comment would be processed
// - Line-based only, can't span variables across multiple lines

namespace Qss {

inline QString render(
    const QString& templateStyleSheet,
    const QHash<QString, QString>& assignments)
{
    // First pass: extract global defaults
    QRegularExpression global_default_re(R"(^\s*(\w+)\|(.*)$)");
    QHash<QString, QString> global_defaults{};
    QStringList content_lines{};

    for (auto& line : templateStyleSheet.split('\n')) {

        // A global default declaration is a line with just `varName|value` (no
        // `{{`/`}}`)
        if (!line.contains("{{")) {
            auto match = global_default_re.match(line);

            if (match.hasMatch()) {
                global_defaults[match.captured(1)] =
                    match.captured(2).trimmed();

                // Consume line
                continue;
            }
        }

        content_lines << line;
    }

    // Second pass: substitute variables
    QRegularExpression var_re(R"(\{\{(\w+)(?:\|([^}]*))?\}\})");
    QStringList qss_lines{};

    for (auto& line : content_lines) {

        QString processed = line;
        auto keep_line = true;

        for (auto& match : var_re.globalMatchView(line)) {
            auto name = match.captured(1);
            auto inline_fallback = match.captured(2); // Empty if no `|`

            // Handle deliberately empty fallback (`{{var|}}`)
            auto has_inline_fallback = match.capturedLength(2) > 0
                                       || match.captured(0).contains("|}}");

            if (assignments.contains(name) && !assignments[name].isEmpty()) {
                processed.replace(match.captured(0), assignments[name]);

            } else if (has_inline_fallback) {
                processed.replace(match.captured(0), inline_fallback);

            } else if (global_defaults.contains(name)) {
                processed.replace(match.captured(0), global_defaults[name]);

            } else {
                keep_line = false;
                break;
            }
        }

        if (keep_line) qss_lines << processed;
    }

    return qss_lines.join('\n');
}

} // namespace Qss
