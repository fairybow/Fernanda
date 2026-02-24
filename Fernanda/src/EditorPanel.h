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
#include <QGroupBox>
#include <QString>
#include <QTextOption>
#include <QVariant>
#include <QVariantMap>

#include "ControlField.h"
#include "Debug.h"
#include "DisplaySlider.h"
#include "Ini.h"
#include "SettingsPanel.h"
#include "Tr.h"

namespace Fernanda {

/// TODO ES
class EditorPanel : public SettingsPanel
{
    Q_OBJECT

public:
    explicit EditorPanel(const QVariantMap& values, QWidget* parent = nullptr)
        : SettingsPanel(Tr::editorPanelTitle(), parent)
    {
        setup_(values);
    }

    virtual ~EditorPanel() override { TRACER; }

private:
    QCheckBox* centerOnScrollCheck_ = new QCheckBox(this);
    QCheckBox* overwriteCheck_ = new QCheckBox(this);
    ControlField<DisplaySlider*>* tabStopDistance_ =
        new ControlField<DisplaySlider*>(
            ControlField<DisplaySlider*>::Label,
            this);
    ControlField<QComboBox*>* wrapMode_ = new ControlField<QComboBox*>(
        ControlField<QComboBox*>::LabelAndInfo,
        this);
    QCheckBox* doubleClickWhitespaceCheck_ = new QCheckBox(this);
    QCheckBox* lineNumbersCheck_ = new QCheckBox(this);
    QCheckBox* lineHighlightCheck_ = new QCheckBox(this);
    QCheckBox* selectionHandlesCheck_ = new QCheckBox(this);

    void setup_(const QVariantMap& values)
    {
        centerOnScrollCheck_->setText(Tr::editorPanelCenterOnScroll());
        centerOnScrollCheck_->setChecked(
            values[Ini::Keys::EDITOR_CENTER_ON_SCROLL].toBool());

        overwriteCheck_->setText(Tr::editorPanelOverwrite());
        overwriteCheck_->setChecked(
            values[Ini::Keys::EDITOR_OVERWRITE].toBool());

        tabStopDistance_->setText(Tr::editorPanelTabStopDistance());
        auto tab_stop_dist_slider = tabStopDistance_->control();
        tab_stop_dist_slider->setRange(
            Ini::Defaults::EDITOR_TAB_STOP_DISTANCE_MIN,
            Ini::Defaults::EDITOR_TAB_STOP_DISTANCE_MAX);
        tab_stop_dist_slider->setValue(
            values[Ini::Keys::EDITOR_TAB_STOP_DISTANCE].toInt());

        wrapMode_->setText(Tr::editorPanelWrapMode());
        wrapMode_->setInfo(Tr::editorPanelWrapModeTooltip());
        auto wrap_mode_box = wrapMode_->control();
        wrap_mode_box->addItem(Tr::editorPanelNoWrap(), QTextOption::NoWrap);
        wrap_mode_box->addItem(
            Tr::editorPanelWordWrap(),
            QTextOption::WordWrap);
        wrap_mode_box->addItem(
            Tr::editorPanelWrapAnywhere(),
            QTextOption::WrapAnywhere);
        wrap_mode_box->addItem(
            Tr::editorPanelWrapAtWordBoundaryOrAnywhere(),
            QTextOption::WrapAtWordBoundaryOrAnywhere);
        wrap_mode_box->setCurrentIndex(wrap_mode_box->findData(
            values[Ini::Keys::EDITOR_WRAP_MODE]
                .value<QTextOption::WrapMode>()));

        doubleClickWhitespaceCheck_->setText(
            Tr::editorPanelDblClickWhitespace());
        doubleClickWhitespaceCheck_->setChecked(
            values[Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE].toBool());

        lineNumbersCheck_->setText(Tr::editorPanelLineNumbers());
        lineNumbersCheck_->setChecked(
            values[Ini::Keys::EDITOR_LINE_NUMBERS].toBool());

        lineHighlightCheck_->setText(Tr::editorPanelLineHighlight());
        lineHighlightCheck_->setChecked(
            values[Ini::Keys::EDITOR_LINE_HIGHLIGHT].toBool());

        selectionHandlesCheck_->setText(Tr::editorPanelSelectionHandles());
        selectionHandlesCheck_->setChecked(
            values[Ini::Keys::EDITOR_SELECTION_HANDLES].toBool());

        // Layout
        auto layout = groupBox()->layout();
        layout->addWidget(centerOnScrollCheck_);
        layout->addWidget(overwriteCheck_);
        layout->addWidget(tabStopDistance_);
        layout->addWidget(wrapMode_);
        layout->addWidget(doubleClickWhitespaceCheck_);
        layout->addWidget(lineNumbersCheck_);
        layout->addWidget(lineHighlightCheck_);
        layout->addWidget(selectionHandlesCheck_);

        // Connect
        connectCheckBox(
            centerOnScrollCheck_,
            Ini::Keys::EDITOR_CENTER_ON_SCROLL);

        connectCheckBox(overwriteCheck_, Ini::Keys::EDITOR_OVERWRITE);

        connect(
            tab_stop_dist_slider,
            &DisplaySlider::valueChanged,
            this,
            [&](int value) {
                emit settingChanged(Ini::Keys::EDITOR_TAB_STOP_DISTANCE, value);
            });

        connectComboBox(wrap_mode_box, Ini::Keys::EDITOR_WRAP_MODE);

        connectCheckBox(
            doubleClickWhitespaceCheck_,
            Ini::Keys::EDITOR_DBL_CLICK_WHITESPACE);

        connectCheckBox(lineNumbersCheck_, Ini::Keys::EDITOR_LINE_NUMBERS);

        connectCheckBox(lineHighlightCheck_, Ini::Keys::EDITOR_LINE_HIGHLIGHT);

        connectCheckBox(
            selectionHandlesCheck_,
            Ini::Keys::EDITOR_SELECTION_HANDLES);
    }
};

} // namespace Fernanda
