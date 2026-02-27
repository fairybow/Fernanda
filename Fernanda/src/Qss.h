/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHash>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Example:
//
// clang-format off
// 
// base|#f5f5f5                              /* Global default (consumed, not output) */
// light_blue = #0096ff
// 
// Widget {
//     background-color: {{base}};           /* Will default to global base (`#f5f5f5`) */
//     selection-color: {{base|light_blue}}; /* Will default to `#0096ff` */
//     text-color: {{text}};                 /* Required (line removed if missing) */
//     border: 1px solid {{border|}};        /* Optional (falls back to `1px solid;`) */
//     border-radius: {{radius|0}}px;        /* Optional (falls back to `0`) */
// }
//
// clang-format on
//
// Limitations:
// - A line with `{{` in a comment would be processed
// - Line-based only, can't span variables across multiple lines

namespace Fernanda::Qss {

inline QString render(
    const QString& templateStyleSheet,
    const QHash<QString, QString>& assignments)
{
    // First pass: extract global defaults and template variables
    static const QRegularExpression global_default_re(R"(^\s*(\w+)\|(.*)$)");
    static const QRegularExpression template_var_re(R"(^\s*(\w+)\s*=\s*(.*)$)");

    QHash<QString, QString> global_defaults{};
    QHash<QString, QString> template_vars{};
    QStringList content_lines{};

    for (auto& line : templateStyleSheet.split('\n')) {

        // A global default declaration is a line with just `varName|value` (no
        // `{{`/`}}`)
        if (!line.contains(QStringLiteral("{{"))) {
            auto global_default_match = global_default_re.match(line);

            if (global_default_match.hasMatch()) {
                global_defaults[global_default_match.captured(1)] =
                    global_default_match.captured(2).trimmed();

                // Consume line
                continue;
            }

            auto var_match = template_var_re.match(line);

            if (var_match.hasMatch()) {
                template_vars[var_match.captured(1)] =
                    var_match.captured(2).trimmed();

                // Consume line
                continue;
            }
        }

        content_lines << line;
    }

    // Second pass: substitute variables
    static const QRegularExpression var_re(R"(\{\{(\w+)(?:\|([^}]*))?\}\})");
    QStringList qss_lines{};

    for (auto& line : content_lines) {

        QString processed = line;
        auto keep_line = true;

        for (auto& match : var_re.globalMatchView(line)) {
            auto name = match.captured(1);
            auto inline_fallback = match.captured(2); // Empty if no `|`

            // Handle deliberately empty fallback (`{{var|}}`)
            auto has_inline_fallback =
                match.capturedLength(2) > 0
                || match.captured(0).contains(QStringLiteral("|}}"));

            QString value{};

            if (assignments.contains(name) && !assignments[name].isEmpty()) {
                value = assignments[name];

            } else if (has_inline_fallback) {
                value = inline_fallback;

            } else if (global_defaults.contains(name)) {
                value = global_defaults[name];

            } else if (template_vars.contains(name)) {
                value = template_vars[name];

            } else {
                keep_line = false;
                break;
            }

            // Expand template variable references
            if (template_vars.contains(value)) value = template_vars[value];

            processed.replace(match.captured(0), value);
        }

        if (keep_line) qss_lines << processed;
    }

    return qss_lines.join('\n');
}

} // namespace Fernanda::Qss
