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

//#include <screenplay-tools/>

#include "models/TextFileModel.h"
#include "views/AbstractMarkupFileView.h"

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
        // 1. Convert plainText to std::string
        // 2. Parse with ScreenplayTools::Fountain::Parser
        // 3. Walk getScript()->getElements()
        // 4. Emit HTML divs with class names per element type
        // 5. Wrap in <style> block with screenplay CSS
        // 6. Return as QString
    }

private:
    //
};

} // namespace Fernanda
