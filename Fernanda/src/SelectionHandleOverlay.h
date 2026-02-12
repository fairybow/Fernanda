/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QWidget>

#include "Debug.h"

namespace Fernanda {

class SelectionHandleOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit SelectionHandleOverlay(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~SelectionHandleOverlay() override { TRACER; }

private:
    void setup_()
    {
        //
    }
};

} // namespace Fernanda
