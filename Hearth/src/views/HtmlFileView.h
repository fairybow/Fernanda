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
#include "models/RawFileModel.h"
#include "views/AbstractFileView.h"
#include "views/WebEnginePage.h"
#include "views/WebEngineView.h"

namespace Fernanda {

class HtmlFileView : public AbstractFileView
{
    Q_OBJECT

public:
    explicit HtmlFileView(RawFileModel* fileModel, QWidget* parent = nullptr)
        : AbstractFileView(fileModel, parent)
    {
    }

    virtual ~HtmlFileView() override { TRACER; }

    virtual bool isUserEditable() const override { return false; }

protected:
    virtual QWidget* setupWidget() override
    {
        auto raw_model = qobject_cast<RawFileModel*>(model());
        ASSERT(raw_model, "RawFileModel cast failed!");

        webView_->setPage(new WebEnginePage(webView_));
        webView_->setHtml(QString::fromUtf8(raw_model->data()));

        return webView_;
    }

private:
    WebEngineView* webView_ = new WebEngineView(this);
};

} // namespace Fernanda
