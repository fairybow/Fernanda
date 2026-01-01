/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"
#include "DisplaySlider.h"
#include "Tr.h"

namespace Fernanda {

// Font selection widget with family dropdown, bold/italic toggles, and size
// slider. Emits currentChanged on every user interaction for live update (write
// should be debounced, but that isn't FontSelector's concern)
//
// TODO: Use toggle switch widget (maybe)
class FontSelector : public QWidget
{
    Q_OBJECT

public:
    explicit FontSelector(
        const QFont& initialFont,
        int sizeMin,
        int sizeMax,
        QWidget* parent = nullptr)
        : QWidget(parent)
        , currentFont_(initialFont)
    {
        setup_(sizeMin, sizeMax);
    }

    virtual ~FontSelector() override { TRACER; }

    QFont currentFont() const { return currentFont_; }

signals:
    void currentChanged(const QFont& font);

private:
    QFont currentFont_;

    QComboBox* fontsBox_ = new QComboBox(this);
    QCheckBox* boldCheckBox_ = new QCheckBox(Tr::bold(), this);
    QCheckBox* italicCheckBox_ = new QCheckBox(Tr::italic(), this);
    DisplaySlider* sizeSlider_ = new DisplaySlider(this);

private:
    void setup_(int sizeMin, int sizeMax)
    {
        // Setup
        QFontDatabase db{};
        fontsBox_->addItems(db.families());
        boldCheckBox_->setTristate(false);
        italicCheckBox_->setTristate(false);
        sizeSlider_->setRange(sizeMin, sizeMax);

        // Populate
        fontsBox_->setCurrentText(currentFont_.family());
        boldCheckBox_->setChecked(currentFont_.bold());
        italicCheckBox_->setChecked(currentFont_.italic());
        sizeSlider_->setValue(currentFont_.pointSize());

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto top_layout = new QHBoxLayout;
        top_layout->addWidget(fontsBox_, 1);
        top_layout->addWidget(boldCheckBox_, 0);
        top_layout->addWidget(italicCheckBox_, 0);

        main_layout->addLayout(top_layout);
        main_layout->addWidget(sizeSlider_);

        // Connect
        connect(
            fontsBox_,
            &QComboBox::currentTextChanged,
            this,
            [&](const QString& text) {
                currentFont_.setFamily(text);
                emit currentChanged(currentFont_);
            });

        connect(boldCheckBox_, &QCheckBox::toggled, this, [&](bool checked) {
            currentFont_.setBold(checked);
            emit currentChanged(currentFont_);
        });

        connect(italicCheckBox_, &QCheckBox::toggled, this, [&](bool checked) {
            currentFont_.setItalic(checked);
            emit currentChanged(currentFont_);
        });

        connect(
            sizeSlider_,
            &DisplaySlider::valueChanged,
            this,
            [&](int value) {
                currentFont_.setPointSize(value);
                emit currentChanged(currentFont_);
            });
    }
};

} // namespace Fernanda
