/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QByteArray>
#include <QChar>
#include <QList>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QWidget>

#include <md4c-html.h>

#include "models/TextFileModel.h"
#include "views/AbstractMarkupFileView.h"

namespace Hearth {

using namespace Qt::StringLiterals;

class MarkdownFileView : public AbstractMarkupFileView
{
    Q_OBJECT

public:
    explicit MarkdownFileView(
        TextFileModel* fileModel,
        QWidget* parent = nullptr)
        : AbstractMarkupFileView(fileModel, parent)
    {
    }

protected:
    virtual QStringView css() const override
    {
        static const auto s = uR"CSS(
* {
    -webkit-touch-callout: none;
    -webkit-user-select: none;
}
html {
    margin: 0;
    padding: 0;
}
body {
    background-color: #fff;
    color: #3e3e3e;
    font: 16px/1.6em 'Segoe UI', 'Noto Sans', sans-serif;
    padding: 40px;
    margin: 0 auto;
    max-width: 680px;
}
h1, h2, h3, h4, h5, h6 {
    margin-top: 1.4em;
    margin-bottom: 0.6em;
    line-height: 1.25;
    color: #1a1a1a;
}
h1 { font-size: 1.6em; }
h2 { font-size: 1.35em; }
h3 { font-size: 1.15em; }
p {
    margin: 0 0 1em;
    word-wrap: break-word;
}
blockquote {
    margin: 1em 0;
    padding: 0 1em;
    border-left: 3px solid #ccc;
    color: #666;
}
code {
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.9em;
    background: #f4f4f4;
    padding: 2px 4px;
    border-radius: 3px;
}
pre {
    background: #f4f4f4;
    padding: 12px 16px;
    border-radius: 4px;
    overflow-x: auto;
    line-height: 1.4;
}
pre code {
    background: none;
    padding: 0;
}
hr {
    border: none;
    border-bottom: 1px solid #ddd;
    margin: 2em 0;
}
table {
    border-collapse: collapse;
    margin: 1em 0;
}
th, td {
    border: 1px solid #ddd;
    padding: 6px 12px;
    text-align: left;
}
th {
    background: #f4f4f4;
}
ul, ol {
    padding-left: 2em;
    margin: 0 0 1em;
}
li {
    margin-bottom: 0.3em;
}
img {
    max-width: 100%;
}
a {
    color: #4271ae;
    text-decoration: none;
}
input[type="checkbox"] {
    margin-right: 0.4em;
}
p, h1, h2, h3, h4, h5, h6,
blockquote, pre, table, ul, ol, hr {
    contain: layout style;
}
)CSS"_s;

        return s;
    }

    virtual QStringList htmlBlocks(const QString& plainText) const override
    {
        auto input = plainText.toUtf8();
        QByteArray output{};
        output.reserve(input.size() * 2);

        md_html(
            input.constData(),
            MD_SIZE(input.size()),
            [](const MD_CHAR* chunk, MD_SIZE size, void* userdata) {
                static_cast<QByteArray*>(userdata)->append(chunk, size);
            },
            &output,
            MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS,
            0);

        return splitMdHtml_(QString::fromUtf8(output));
    }

private:
    static QStringList splitMdHtml_(const QString& html)
    {
        struct Block
        {
            qsizetype start; // index of '<' that opens this block
            qsizetype injectAt; // index right after tag name
            qsizetype end; // past-the-end index of this block
        };

        QList<Block> found_blocks{};
        const qsizetype len = html.size();
        auto depth = 0;
        qsizetype i = 0;

        while (i < len) {
            if (html[i] != QLatin1Char('<')) {
                i++;
                continue;
            }

            auto closing = (i + 1 < len && html[i + 1] == QLatin1Char('/'));
            auto name_start = closing ? i + 2 : i + 1;
            auto name_end = name_start;

            while (name_end < len) {
                auto c = html[name_end];
                if (c == QLatin1Char(' ') || c == QLatin1Char('>')
                    || c == QLatin1Char('/') || c == QLatin1Char('\n'))
                    break;

                name_end++;
            }

            auto tag_end = name_end;

            while (tag_end < len && html[tag_end] != QLatin1Char('>')) {
                tag_end++;
            }

            if (tag_end >= len) break;

            auto name =
                QStringView(html).mid(name_start, name_end - name_start);

            auto is_void = name.compare(u"hr", Qt::CaseInsensitive) == 0
                           || name.compare(u"br", Qt::CaseInsensitive) == 0
                           || name.compare(u"img", Qt::CaseInsensitive) == 0
                           || name.compare(u"input", Qt::CaseInsensitive) == 0;

            if (closing) {
                i = tag_end + 1;
                depth--;

                if (depth == 0 && !found_blocks.isEmpty()) {
                    auto end = i;

                    while (end < len && html[end] == QLatin1Char('\n')) {
                        end++;
                    }

                    found_blocks.last().end = end;
                }

            } else if (is_void) {
                if (depth == 0) {
                    auto end = tag_end + 1;

                    while (end < len && html[end] == QLatin1Char('\n')) {
                        end++;
                    }

                    found_blocks.append({ i, name_end, end });
                }

                i = tag_end + 1;

            } else {
                if (depth == 0) found_blocks.append({ i, name_end, -1 });
                depth++;
                i = tag_end + 1;
            }
        }

        QStringList edited_blocks{};
        edited_blocks.reserve(found_blocks.size());
        auto index = 0;

        for (const auto& found_block : found_blocks) {
            if (found_block.end < 0) continue;

            QString block{};

            block.reserve(found_block.end - found_block.start + 20);

            block.append(QStringView(html).mid(
                found_block.start,
                found_block.injectAt - found_block.start));

            block.append(u" data-idx='%1'"_s.arg(index++));

            block.append(QStringView(html).mid(
                found_block.injectAt,
                found_block.end - found_block.injectAt));

            edited_blocks.append(std::move(block));
        }

        return edited_blocks;
    }
};

} // namespace Hearth
