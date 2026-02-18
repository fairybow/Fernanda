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
#include <QTextOption>
#include <QVBoxLayout>
#include <QWidget>

#include "ControlField.h"
#include "Debug.h"
#include "DisplaySlider.h"
#include "Ini.h"
#include "Tr.h"

namespace Fernanda {

/// TODO ES
class EditorPanel : public QWidget
{
    Q_OBJECT

public:
    struct InitialValues
    {
        bool centerOnScroll;
        bool overwrite;
        int tabStopDistance;
        QTextOption::WrapMode wrapMode;
        bool doubleClickWhitespace;
        bool lineNumbers;
        bool lineHighlight;
        bool selectionHandles;
    };

    explicit EditorPanel(
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_(initialValues);
    }

    virtual ~EditorPanel() override { TRACER; }

signals:
    void centerOnScrollChanged(bool centerOnScroll);
    void overwriteChanged(bool overwrite);
    void tabStopDistanceChanged(int tabStopDistance);
    void wrapModeChanged(QTextOption::WrapMode wrapMode);
    void doubleClickWhitespaceChanged(bool doubleClickWhitespace);
    void lineNumbersChanged(bool lineNumbers);
    void lineHighlightChanged(bool lineHighlight);
    void selectionHandlesChanged(bool selectionHandles);

private:
    QGroupBox* groupBox_ = new QGroupBox(this);
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

    void setup_(const InitialValues& initialValues)
    {
        // Populate
        groupBox_->setTitle(Tr::editorPanelTitle());
        groupBox_->setCheckable(false);

        centerOnScrollCheck_->setText(Tr::editorPanelCenterOnScroll());
        centerOnScrollCheck_->setChecked(initialValues.centerOnScroll);

        overwriteCheck_->setText(Tr::editorPanelOverwrite());
        overwriteCheck_->setChecked(initialValues.overwrite);

        tabStopDistance_->setText(Tr::editorPanelTabStopDistance());
        auto tab_stop_dist_slider = tabStopDistance_->control();
        tab_stop_dist_slider->setRange(
            Ini::Defaults::EDITOR_TAB_STOP_DISTANCE_MIN,
            Ini::Defaults::EDITOR_TAB_STOP_DISTANCE_MAX);
        tab_stop_dist_slider->setValue(initialValues.tabStopDistance);

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
        wrap_mode_box->setCurrentIndex(
            wrap_mode_box->findData(initialValues.wrapMode));

        doubleClickWhitespaceCheck_->setText(
            Tr::editorPanelDblClickWhitespace());
        doubleClickWhitespaceCheck_->setChecked(
            initialValues.doubleClickWhitespace);

        lineNumbersCheck_->setText(Tr::editorPanelLineNumbers());
        lineNumbersCheck_->setChecked(initialValues.lineNumbers);

        lineHighlightCheck_->setText(Tr::editorPanelLineHighlight());
        lineHighlightCheck_->setChecked(initialValues.lineHighlight);

        selectionHandlesCheck_->setText(Tr::editorPanelSelectionHandles());
        selectionHandlesCheck_->setChecked(initialValues.selectionHandles);

        // Layout
        auto main_layout = new QVBoxLayout(this);
        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(centerOnScrollCheck_);
        group_box_layout->addWidget(overwriteCheck_);
        group_box_layout->addWidget(tabStopDistance_);
        group_box_layout->addWidget(wrapMode_);
        group_box_layout->addWidget(doubleClickWhitespaceCheck_);
        group_box_layout->addWidget(lineNumbersCheck_);
        group_box_layout->addWidget(lineHighlightCheck_);
        group_box_layout->addWidget(selectionHandlesCheck_);
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);

        // Connect
        connect(
            centerOnScrollCheck_,
            &QCheckBox::toggled,
            this,
            [&](bool toggled) { emit centerOnScrollChanged(toggled); });

        connect(overwriteCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit overwriteChanged(toggled);
        });

        connect(
            tab_stop_dist_slider,
            &DisplaySlider::valueChanged,
            this,
            [&](int value) { emit tabStopDistanceChanged(value); });

        connect(
            wrap_mode_box,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                emit wrapModeChanged(wrapMode_->control()
                                         ->itemData(index)
                                         .value<QTextOption::WrapMode>());
            });

        connect(
            doubleClickWhitespaceCheck_,
            &QCheckBox::toggled,
            this,
            [&](bool toggled) { emit doubleClickWhitespaceChanged(toggled); });

        connect(
            lineNumbersCheck_,
            &QCheckBox::toggled,
            this,
            [&](bool toggled) { emit lineNumbersChanged(toggled); });

        connect(
            lineHighlightCheck_,
            &QCheckBox::toggled,
            this,
            [&](bool toggled) { emit lineHighlightChanged(toggled); });

        connect(
            selectionHandlesCheck_,
            &QCheckBox::toggled,
            this,
            [&](bool toggled) { emit selectionHandlesChanged(toggled); });
    }
};

} // namespace Fernanda
