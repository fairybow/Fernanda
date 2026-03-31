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
#include <md4c.h>

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
        : AbstractMarkupFileView(fileModel, parent)
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

        return QString::fromUtf8(output);
    }
};

} // namespace Fernanda
