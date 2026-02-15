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
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"
#include "Ini.h"
#include "Tr.h"

namespace Fernanda {

class WordCounterPanel : public QWidget
{
    Q_OBJECT

public:
    struct InitialValues
    {
        bool lineCount;
        bool wordCount;
        bool charCount;
        bool selection;
        bool selReplace;
        bool linePos;
        bool colPos;
    };

    explicit WordCounterPanel(
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_(initialValues);
    }

    virtual ~WordCounterPanel() override { TRACER; }

signals:
    void lineCountChanged(bool lineCount);
    void wordCountChanged(bool wordCount);
    void charCountChanged(bool charCount);
    void selectionChanged(bool selection);
    void selReplaceChanged(bool selReplace);
    void linePosChanged(bool linePos);
    void colPosChanged(bool colPos);

private:
    QGroupBox* groupBox_ = new QGroupBox(this);
    QCheckBox* lineCountCheck_ = new QCheckBox(this);
    QCheckBox* wordCountCheck_ = new QCheckBox(this);
    QCheckBox* charCountCheck_ = new QCheckBox(this);
    QCheckBox* selectionCheck_ = new QCheckBox(this);
    QCheckBox* selReplaceCheck_ = new QCheckBox(this);
    QCheckBox* linePosCheck_ = new QCheckBox(this);
    QCheckBox* colPosCheck_ = new QCheckBox(this);

    void setup_(const InitialValues& initialValues)
    {
        // Populate
        groupBox_->setTitle(Tr::wordCounterTitle());
        groupBox_->setCheckable(false); // TODO: Active check

        lineCountCheck_->setText(Tr::wordCounterLineCount());
        lineCountCheck_->setChecked(initialValues.lineCount);

        wordCountCheck_->setText(Tr::wordCounterWordCount());
        wordCountCheck_->setChecked(initialValues.wordCount);

        charCountCheck_->setText(Tr::wordCounterCharCount());
        charCountCheck_->setChecked(initialValues.charCount);

        selectionCheck_->setText(Tr::wordCounterSelection());
        selectionCheck_->setChecked(initialValues.selection);

        selReplaceCheck_->setText(Tr::wordCounterSelReplace());
        selReplaceCheck_->setChecked(initialValues.selReplace);

        linePosCheck_->setText(Tr::wordCounterLinePos());
        linePosCheck_->setChecked(initialValues.linePos);

        colPosCheck_->setText(Tr::wordCounterColPos());
        colPosCheck_->setChecked(initialValues.colPos);

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto group_box_layout = new QVBoxLayout;
        group_box_layout->addWidget(lineCountCheck_);
        group_box_layout->addWidget(wordCountCheck_);
        group_box_layout->addWidget(charCountCheck_);
        group_box_layout->addWidget(selectionCheck_);
        group_box_layout->addWidget(selReplaceCheck_);
        group_box_layout->addWidget(linePosCheck_);
        group_box_layout->addWidget(colPosCheck_);
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);

        // Connect
        connect(lineCountCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit lineCountChanged(toggled);
        });

        connect(wordCountCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit wordCountChanged(toggled);
        });

        connect(charCountCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit charCountChanged(toggled);
        });

        connect(selectionCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit selectionChanged(toggled);
        });

        connect(selReplaceCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit selReplaceChanged(toggled);
        });

        connect(linePosCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit linePosChanged(toggled);
        });

        connect(colPosCheck_, &QCheckBox::toggled, this, [&](bool toggled) {
            emit colPosChanged(toggled);
        });
    }
};

} // namespace Fernanda
