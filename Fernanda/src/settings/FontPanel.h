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

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFontDatabase>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QTextOption>
#include <QVariant>

#include <Coco/Utility.h>

#include "core/Debug.h"
#include "core/Fonts.h"
#include "core/Tr.h"
#include "settings/Ini.h"
#include "settings/SettingsPanel.h"
#include "ui/DisplaySlider.h"

namespace Fernanda {

// Font selection widget with family dropdown, bold/italic toggles, and size
// slider. Emits currentChanged on every user interaction for live update (write
// should be debounced, but that isn't FontSelector's concern)
//
// TODO: Use toggle switch widget (maybe)
class FontPanel : public SettingsPanel
{
    Q_OBJECT

public:
    explicit FontPanel(const Ini::Map& values, QWidget* parent = nullptr)
        : SettingsPanel(Tr::fontPanelTitle(), parent)
        , currentFont_(values[Ini::Keys::EDITOR_FONT].value<QFont>())
    {
        setup_(values);
    }

    virtual ~FontPanel() override { TRACER; }

private:
    QFont currentFont_;

    QComboBox* fontsBox_ = new QComboBox(this);
    QCheckBox* boldCheckBox_ = new QCheckBox(Tr::fontPanelBold(), this);
    QCheckBox* italicCheckBox_ = new QCheckBox(Tr::fontPanelItalic(), this);
    DisplaySlider* sizeSlider_ = new DisplaySlider(this);

    void setup_(const Ini::Map& values)
    {
        // Populate
        auto families = QFontDatabase::families();

        // Filter out any family name metadata quirks
        families.removeIf([](const QString& f) {
            static const auto suffixes = { "Bold", "Italic", "Light",  "Medium",
                                           "Thin", "Black",  "Regular" };
            for (auto& suffix : suffixes)
                if (f.endsWith(suffix, Qt::CaseInsensitive)) return true;

            return false;
        });

        // Remove bundled fonts from the system list to avoid duplicates
        auto bundled = Fonts::families();

        for (auto& name : bundled)
            families.removeAll(name);

        fontsBox_->addItems(bundled);
        fontsBox_->insertSeparator(bundled.size());
        fontsBox_->addItems(families);
        fontsBox_->setCurrentText(currentFont_.family());

        boldCheckBox_->setChecked(currentFont_.bold());
        italicCheckBox_->setChecked(currentFont_.italic());

        sizeSlider_->setRange(
            Ini::Limits::FONT_SIZE_MIN,
            Ini::Limits::FONT_SIZE_MAX);
        sizeSlider_->setValue(currentFont_.pointSize());

        // Layout
        auto top_layout = new QHBoxLayout;
        top_layout->addWidget(fontsBox_, 1);
        top_layout->addWidget(boldCheckBox_, 0);
        top_layout->addWidget(italicCheckBox_, 0);

        auto layout = groupBox()->layout();
        layout->addItem(top_layout);
        layout->addWidget(sizeSlider_);

        // Connect
        connect(
            fontsBox_,
            &QComboBox::currentTextChanged,
            this,
            [this](const QString& text) {
                currentFont_.setFamily(text);
                emitFont_();
            });

        connect(boldCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
            currentFont_.setBold(checked);
            emitFont_();
        });

        connect(
            italicCheckBox_,
            &QCheckBox::toggled,
            this,
            [this](bool checked) {
                currentFont_.setItalic(checked);
                emitFont_();
            });

        connect(
            sizeSlider_,
            &DisplaySlider::valueChanged,
            this,
            [this](int value) {
                currentFont_.setPointSize(value);
                emitFont_();
            });
    }

    void emitFont_()
    {
        emit settingChanged(Ini::Keys::EDITOR_FONT, qVar(currentFont_));
    }
};

} // namespace Fernanda
