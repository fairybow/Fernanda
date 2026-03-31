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
#include "views/Fountain.h"

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
        auto renderer = Fountain::Renderer(parser);
        return QString::fromStdString(renderer.html());

        /*auto font = QFont("Courier", 12);
        auto metrics = QFontMetrics(font);

        auto measureFn = [metrics](const std::string& text, int maxWidth, int
        lineHeight) -> int { auto qtext = QString::fromStdString(text); auto rect =
        metrics.boundingRect( QRect(0, 0, maxWidth, INT_MAX), Qt::TextWordWrap, qtext);
            int numLines = (rect.height() + lineHeight - 1) / lineHeight;
            return numLines * lineHeight;
        };

        auto renderer = Fountain::Renderer(parser, measureFn);*/
    }
};

} // namespace Fernanda
