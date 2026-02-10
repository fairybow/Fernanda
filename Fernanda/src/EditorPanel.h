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
#include <QHBoxLayout>
#include <QLabel>
#include <QTextOption>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"
#include "DisplaySlider.h"
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
        QTextOption::WrapMode wordWrapMode;
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
    void wordWrapModeChanged(QTextOption::WrapMode wordWrapMode);

private:
    QGroupBox* groupBox_ = new QGroupBox(this);
    QCheckBox* centerOnScrollCheck_ = new QCheckBox(this);
    QCheckBox* overwriteCheck_ = new QCheckBox(this);
    QLabel* tabStopDistanceSliderLabel_ = new QLabel(this);
    DisplaySlider* tabStopDistanceSlider_ = new DisplaySlider(this);
    QLabel* wordWrapModeComboBoxLabel_ = new QLabel(this);
    QComboBox* wordWrapModeComboBox_ = new QComboBox(this);

    void setup_(const InitialValues& initialValues)
    {
        // Populate
        groupBox_->setTitle(Tr::editorPanelTitle());
        groupBox_->setCheckable(false);

        centerOnScrollCheck_->setText(Tr::editorPanelCenterOnScroll());
        centerOnScrollCheck_->setChecked(initialValues.centerOnScroll);

        overwriteCheck_->setText(Tr::editorPanelOverwrite());
        overwriteCheck_->setChecked(initialValues.overwrite);

        tabStopDistanceSliderLabel_->setText(Tr::editorPanelTabStopDistance());
        tabStopDistanceSlider_->setRange(
            Ini::Defaults::EDITOR_TAB_STOP_DISTANCE_MIN,
            Ini::Defaults::EDITOR_TAB_STOP_DISTANCE_MAX);
        tabStopDistanceSlider_->setValue(initialValues.tabStopDistance);

        wordWrapModeComboBoxLabel_->setText(Tr::editorPanelWordWrapMode());
        wordWrapModeComboBox_->addItem(
            Tr::editorPanelNoWrap(),
            QTextOption::NoWrap);
        wordWrapModeComboBox_->addItem(
            Tr::editorPanelWordWrap(),
            QTextOption::WordWrap);
        wordWrapModeComboBox_->addItem(
            Tr::editorPanelWrapAnywhere(),
            QTextOption::WrapAnywhere);
        wordWrapModeComboBox_->addItem(
            Tr::editorPanelWrapAtWordBoundaryOrAnywhere(),
            QTextOption::WrapAtWordBoundaryOrAnywhere);
        wordWrapModeComboBox_->setCurrentIndex(
            wordWrapModeComboBox_->findData(initialValues.wordWrapMode));

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto tab_stop_layout = new QHBoxLayout;
        tab_stop_layout->addWidget(tabStopDistanceSliderLabel_);
        tab_stop_layout->addWidget(tabStopDistanceSlider_);

        auto word_wrap_layout = new QHBoxLayout;
        word_wrap_layout->addWidget(wordWrapModeComboBoxLabel_);
        word_wrap_layout->addWidget(wordWrapModeComboBox_);

        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(centerOnScrollCheck_);
        group_box_layout->addWidget(overwriteCheck_);
        group_box_layout->addLayout(tab_stop_layout);
        group_box_layout->addLayout(word_wrap_layout);
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
            tabStopDistanceSlider_,
            &DisplaySlider::valueChanged,
            this,
            [&](int value) { emit tabStopDistanceChanged(value); });

        connect(
            wordWrapModeComboBox_,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                emit wordWrapModeChanged(wordWrapModeComboBox_->itemData(index)
                                             .value<QTextOption::WrapMode>());
            });
    }
};

} // namespace Fernanda
