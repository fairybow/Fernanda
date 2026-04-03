/*
 * Fernanda — a plain-text-first workbench for creative writing
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
#include <QString>
#include <QWidget>

#include <md4c-html.h>

#include "models/TextFileModel.h"
#include "views/AbstractMarkupFileView.h"

namespace Fernanda {

class MarkdownFileView : public AbstractMarkupFileView
{
    Q_OBJECT

public:
    explicit MarkdownFileView(
        TextFileModel* fileModel,
        QWidget* parent = nullptr)
        // md4c doesn't need debouce
        : AbstractMarkupFileView(fileModel, 0, parent)
    {
    }

    virtual ~MarkdownFileView() override {}

protected:
    virtual QString renderToHtml(const QString& plainText) const override
    {
        auto input = plainText.toUtf8();
        QByteArray output{};

        md_html(
            input.constData(),
            MD_SIZE(input.size()),
            [](const MD_CHAR* chunk, MD_SIZE size, void* userdata) {
                static_cast<QByteArray*>(userdata)->append(chunk, size);
            },
            &output,
            MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS,
            0);

        auto body = QString::fromUtf8(output);

        /// TODO MU: ^ Print this to see if it returns inside body tags or without

        return QStringLiteral(
                   "<html><head><style>%1</style></head><body>%2</body></html>")
            .arg(CSS_, body);
    }

private:
    static constexpr const char* CSS_ = R"CSS(
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
)CSS";
};

} // namespace Fernanda
