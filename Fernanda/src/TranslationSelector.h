/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

class TranslationSelector : public QWidget
{
    Q_OBJECT

public:
    explicit TranslationSelector(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~TranslationSelector() override { TRACER; }

signals:
    //...

private:
    void setup_()
    {
        // Populate
        //...

        // Layout
        //...

        // Connect
        //...
    }
};

} // namespace Fernanda
