/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QDialog>
#include <QFont>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"
#include "FontSelector.h"

namespace Fernanda {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    struct InitialValues
    {
        QFont font;
        int fontSizeMin;
        int fontSizeMax;

        //...
    };

    explicit SettingsDialog(
        const QString& title,
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setup_(title, initialValues);
    }

    virtual ~SettingsDialog() override { TRACER; }

signals:
    void fontChanged(const QFont& font);

private:
    FontSelector* fontSelector_ = nullptr;

    void setup_(const QString& title, const InitialValues& initialValues)
    {
        setModal(false);
        setWindowTitle(title);

        fontSelector_ = new FontSelector(
            initialValues.font,
            initialValues.fontSizeMin,
            initialValues.fontSizeMax,
            this);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(fontSelector_);

        connect(
            fontSelector_,
            &FontSelector::currentChanged,
            this,
            [&](const QFont& font) { emit fontChanged(font); });
    }
};

} // namespace Fernanda
