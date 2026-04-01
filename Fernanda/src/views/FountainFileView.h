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

#include "core/Debug.h"
#include "models/TextFileModel.h"
#include "views/AbstractMarkupFileView.h"
#include "views/Fountain.h"

/// TODO MU: Potentially find a js Fountain parser/render - something well
/// tested and liked and used that

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

            ///
            // if (text.size() < 30) {
            //     qDebug() << "measure:" << QString::fromStdString(text)
            //              << "width:" << maxWidth << "lines:" << line_count;
            // }
            ///

            return line_count * lineHeight;
        };

        auto renderer = Fountain::Renderer(parser, measure_fn);
        return QString::fromStdString(renderer.html());
    }
};

} // namespace Fernanda
