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
#include <QTextLayout>
#include <QTextLine>
#include <QWidget>

#include <Fountain.h>

#include "core/Debug.h"
#include "models/TextFileModel.h"
#include "views/AbstractMarkupFileView.h"

/// TODO MU: Could leave preview in this semi-flow paginated state and only do
/// page stuff when drawing to PDF directly for print/export

/// TODO MU: Extra title page space on the semi-flow is Fountain.h CSS. We could
/// remove CSS from the Renderer entirely and leave that to the caller - kind of
/// makes sense? Or, split rendering and html creation and allow a caller to
/// only use HTML if they want. Could also provide two sets of CSS in Renderer -
/// optional stuff (like padding on the title page) vs very important stuff
/// (dialog padding)

namespace Fernanda {

class FountainFileView : public AbstractMarkupFileView
{
    Q_OBJECT

public:
    explicit FountainFileView(
        TextFileModel* fileModel,
        QWidget* parent = nullptr)
        : AbstractMarkupFileView(fileModel, parent)
    {
    }

    virtual ~FountainFileView() override {}

protected:
    virtual QString renderToHtml(const QString& plainText) const override
    {
        auto parser = Fountain::Parser(plainText.toStdString());
        auto font = QFont("Courier Prime");
        font.setPixelSize(12);

        auto measure_fn = [font](
                              const std::string& text,
                              int maxWidth,
                              int lineHeight) -> int {
            QTextLayout layout(QString::fromStdString(text), font);
            layout.beginLayout();

            auto line_count = 0;

            while (true) {
                auto line = layout.createLine();
                if (!line.isValid()) break;

                line.setLineWidth(maxWidth);
                ++line_count;
            }

            layout.endLayout();
            return line_count * lineHeight;
        };

        auto renderer = Fountain::Renderer(parser, measure_fn);
        return QString::fromStdString(renderer.html());
    }
};

} // namespace Fernanda
