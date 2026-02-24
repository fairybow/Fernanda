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
#include <QGroupBox>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Debug.h"
#include "Ini.h"
#include "SettingsPanel.h"
#include "Tr.h"

namespace Fernanda {

class WordCounterPanel : public SettingsPanel
{
    Q_OBJECT

public:
    explicit WordCounterPanel(
        const QVariantMap& values,
        QWidget* parent = nullptr)
        : SettingsPanel(Tr::wordCounterPanelTitle(), parent)
    {
        setup_(values);
    }

    virtual ~WordCounterPanel() override { TRACER; }

private:
    QCheckBox* lineCountCheck_ = new QCheckBox(this);
    QCheckBox* wordCountCheck_ = new QCheckBox(this);
    QCheckBox* charCountCheck_ = new QCheckBox(this);
    QCheckBox* selectionCheck_ = new QCheckBox(this);
    QCheckBox* selReplaceCheck_ = new QCheckBox(this);
    QCheckBox* linePosCheck_ = new QCheckBox(this);
    QCheckBox* colPosCheck_ = new QCheckBox(this);

    void setup_(const QVariantMap& values)
    {
        // Populate
        auto group_box = groupBox();
        group_box->setCheckable(true);
        group_box->setChecked(values[Ini::Keys::WORD_COUNTER_ACTIVE].toBool());

        lineCountCheck_->setText(Tr::wordCounterPanelLineCount());
        lineCountCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_LINE_COUNT].toBool());

        wordCountCheck_->setText(Tr::wordCounterPanelWordCount());
        wordCountCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_WORD_COUNT].toBool());

        charCountCheck_->setText(Tr::wordCounterPanelCharCount());
        charCountCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_CHAR_COUNT].toBool());

        selectionCheck_->setText(Tr::wordCounterPanelSel());
        selectionCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_SELECTION].toBool());

        selReplaceCheck_->setText(Tr::wordCounterPanelSelReplace());
        selReplaceCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_SEL_REPLACE].toBool());

        linePosCheck_->setText(Tr::wordCounterPanelLinePos());
        linePosCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_LINE_POS].toBool());

        colPosCheck_->setText(Tr::wordCounterPanelColPos());
        colPosCheck_->setChecked(
            values[Ini::Keys::WORD_COUNTER_COL_POS].toBool());

        // Layout
        auto layout = group_box->layout();
        layout->addWidget(lineCountCheck_);
        layout->addWidget(wordCountCheck_);
        layout->addWidget(charCountCheck_);
        layout->addWidget(selectionCheck_);
        layout->addWidget(selReplaceCheck_);
        layout->addWidget(linePosCheck_);
        layout->addWidget(colPosCheck_);

        // Connect
        connect(group_box, &QGroupBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_ACTIVE, toggled);
        });

        connect(lineCountCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_LINE_COUNT, toggled);
        });

        connect(wordCountCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_WORD_COUNT, toggled);
        });

        connect(charCountCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_CHAR_COUNT, toggled);
        });

        connect(selectionCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_SELECTION, toggled);
        });

        connect(selReplaceCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_SEL_REPLACE, toggled);
        });

        connect(linePosCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_LINE_POS, toggled);
        });

        connect(colPosCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit settingChanged(Ini::Keys::WORD_COUNTER_COL_POS, toggled);
        });
    }
};

} // namespace Fernanda
