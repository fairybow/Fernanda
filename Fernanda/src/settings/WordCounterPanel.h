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
#include <QGroupBox>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "core/Debug.h"
#include "core/Tr.h"
#include "settings/Ini.h"
#include "settings/SettingsPanel.h"

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
        connectGroupBox(Ini::Keys::WORD_COUNTER_ACTIVE);
        connectCheckBox(lineCountCheck_, Ini::Keys::WORD_COUNTER_LINE_COUNT);
        connectCheckBox(wordCountCheck_, Ini::Keys::WORD_COUNTER_WORD_COUNT);
        connectCheckBox(charCountCheck_, Ini::Keys::WORD_COUNTER_CHAR_COUNT);
        connectCheckBox(selectionCheck_, Ini::Keys::WORD_COUNTER_SELECTION);
        connectCheckBox(selReplaceCheck_, Ini::Keys::WORD_COUNTER_SEL_REPLACE);
        connectCheckBox(linePosCheck_, Ini::Keys::WORD_COUNTER_LINE_POS);
        connectCheckBox(colPosCheck_, Ini::Keys::WORD_COUNTER_COL_POS);
    }
};

} // namespace Fernanda
