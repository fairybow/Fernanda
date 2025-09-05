/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>
#include <QLabel>
#include <QObject>
#include <QWidget>
#include <Qt>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "IFileView.h"
#include "NoOpFileModel.h"

namespace Fernanda {

// Read-only placeholder view for unsupported file types, displaying simple
// placeholder content potentially for binary files or other non-viewable
// formats
class NoOpFileView : public IFileView
{
    Q_OBJECT

public:
    explicit NoOpFileView(NoOpFileModel* model, QWidget* parent = nullptr)
        : IFileView(model, parent)
    {
    }

    virtual ~NoOpFileView() override { COCO_TRACER; }

protected:
    virtual QWidget* setupWidget() override
    {
        auto label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        QFont font = label->font();
        font.setPointSize(24);
        font.setBold(true);
        label->setFont(font);
        label->setText(":')");
        return label;
    }
};

} // namespace Fernanda
