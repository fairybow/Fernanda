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

#include <QWidget>

#include "core/Debug.h"
#include "models/AbstractFileModel.h"
#include "models/HtmlFileModel.h"
#include "views/AbstractFileView.h"

namespace Fernanda {

class HtmlFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit HtmlFileView(HtmlFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~HtmlFileView() override { TRACER; }

    virtual bool isUserEditable() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        //

        return nullptr;
    }

private:
    //
};

} // namespace Fernanda
