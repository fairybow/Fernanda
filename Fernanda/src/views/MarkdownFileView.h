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

#include <QString>
#include <QWidget>

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
    virtual QWidget* setupWidget() override {}
    virtual QString renderToHtml(const QString& plainText) const override {}

private:
    //
};

} // namespace Fernanda
