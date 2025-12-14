/*
 * Fernanda  Copyright (C) 2025  fairybow
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
#include "Ini.h"
#include "Tr.h"

namespace Fernanda {

class FontSelector : public QWidget
{
    Q_OBJECT

public:
    explicit FontSelector(const QFont& initialFont, QWidget* parent = nullptr)
        : QWidget(parent)
        , currentFont_(initialFont)
    {
        setup_();
    }

    virtual ~FontSelector() override { TRACER; }

signals:
    void currentChanged(const QFont& font);

private:
    QFont currentFont_;

    QComboBox* fontsBox_ = new QComboBox(this);
    QCheckBox* boldCheckBox_ = new QCheckBox(
        Tr::FontSelector::bold(),
        this); // Eventually, use custom toggle switch widget (maybe)
    QCheckBox* italicCheckBox_ =
        new QCheckBox(Tr::FontSelector::italic(), this); // ^
    DisplaySlider* sizeSlider_ = new DisplaySlider(this);

private:
    void setup_()
    {
        // Setup
        QFontDatabase db{};
        fontsBox_->addItems(db.families());
        boldCheckBox_->setTristate(false);
        italicCheckBox_->setTristate(false);
        sizeSlider_->setRange(
            Ini::Editor::FONT_PT_SIZE_MIN,
            Ini::Editor::FONT_PT_SIZE_MAX);

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
