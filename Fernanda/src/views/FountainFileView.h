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

#include <string>

#include <QFont>
#include <QString>
#include <QStringView>
#include <QTextLayout>
#include <QTextLine>
#include <QWidget>

#include <fountain-html.h>

#include "core/Debug.h"
#include "models/TextFileModel.h"
#include "views/AbstractMarkupFileView.h"

namespace Fernanda {

/// TODO MU: Printing layout
class FountainFileView : public AbstractMarkupFileView
{
    Q_OBJECT

public:
    explicit FountainFileView(
        TextFileModel* fileModel,
        QWidget* parent = nullptr)
        /// TODO MU: dynamic debounce for larger files?
        : AbstractMarkupFileView(fileModel, 25, parent)
    {
    }

    virtual ~FountainFileView() override {}

protected:
    virtual QStringView css() const override
    {
        static const auto s = QStringLiteral(R"CSS(
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
    font: 14px/1.3em 'Courier Prime', 'Courier', monospace;
    padding: 0;
    margin: 0;
}
article {
    padding: 40px 0;
    margin: 0;
}
section {
    padding: 0 0 0 40px;
    width: 465px;
    margin-right: auto;
    margin-left: auto;
}
p {
    margin: 1.3em auto;
    word-wrap: break-word;
    padding: 0 10px;
}
p.page-break {
    text-align: right;
    border-top: 1px solid #ccc;
    padding-top: 20px;
    margin-top: 20px;
}
body > p:first-child {
    margin-top: 0;
}
.scene-heading, .transition, .character {
    text-transform: uppercase;
}
.transition {
    text-align: right;
}
.character {
    margin: 1.3em auto 0;
    width: 180px;
}
.dialogue {
    margin: 0 auto;
    width: 310px;
}
.parenthetical {
    margin: 0 auto;
    width: 250px;
}
.scene-heading {
    margin-top: 2.6em;
    font-weight: bold;
    position: relative;
    padding-right: 40px;
}
.scene-number-left {
    float: left;
    margin-left: -50px;
}
.scene-number-right {
    position: absolute;
    right: 0;
    top: 0;
}
#script-title {
    overflow: hidden;
    display: block;
    /*padding-bottom: 2.6em;
    margin-bottom: 2.6em;*/
    /*TODO: ^ May want to leave spacing like this a display concern */
    /*TODO: Additionally, may want to provide two levels of CSS - essential
    and optional and/or a callback for providing CSS to renderer */
}
#script-title .title {
    text-align: center;
    margin: 1.3em 0;
    text-decoration: underline;
    font-weight: bold;
    text-transform: uppercase;
}
#script-title .credit {
    text-align: center;
}
#script-title .authors {
    text-align: center;
}
#script-title .source {
    text-align: center;
    padding-top: 1.3em;
}
#script-title .notes {
    padding-top: 2.6em;
    white-space: pre-line;
}
.center {
    text-align: center !important;
}
hr {
    height: 0px;
    border: none;
    border-bottom: 1px solid #ccc;
}
.dual-dialogue {
    overflow: hidden;
}
.dual-dialogue .dual-dialogue-left,
.dual-dialogue .dual-dialogue-right {
    width: 228px;
    float: left;
}
.dual-dialogue p {
    width: auto;
}
.dual-dialogue .character {
    padding-left: 40px;
}
.dual-dialogue .parenthetical {
    padding-left: 40px;
}
.lyrics {
    font-style: italic;
}
)CSS");

        return s;
    }

    virtual QString htmlBody(const QString& plainText) const override
    {
        auto input = plainText.toUtf8();
        QByteArray output{};
        output.reserve(input.size() * 2);

        fn_html(
            input.constData(),
            FN_SIZE(input.size()),
            [](const FN_CHAR* chunk, FN_SIZE size, void* userdata) {
                static_cast<QByteArray*>(userdata)->append(chunk, size);
            },
            &output,
            0, /// TODO MU: flags?
            0);

        return QString::fromUtf8(output);
    }
};

} // namespace Fernanda
