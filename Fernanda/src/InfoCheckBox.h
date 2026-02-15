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
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QStyle>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

class InfoCheckBox : public QWidget
{
    Q_OBJECT

public:
    explicit InfoCheckBox(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_();
    }

    virtual ~InfoCheckBox() override { TRACER; }

    void setChecked(bool checked) { checkBox_->setChecked(checked); }
    void setText(const QString& text) { checkBox_->setText(text); }

    void setInfoToolTip(const QString& infoToolTip)
    {
        infoLabel_->setToolTip(infoToolTip);
    }

signals:
    void toggled(bool checked);

private:
    QCheckBox* checkBox_ = new QCheckBox(this);
    QLabel* infoLabel_ = new QLabel(this);

    void setup_()
    {
        infoLabel_->setPixmap(
            style()
                ->standardPixmap(QStyle::SP_MessageBoxInformation)
                .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(checkBox_);
        layout->addWidget(infoLabel_);
        layout->addStretch();

        connect(checkBox_, &QCheckBox::toggled, this, [&](bool checked) {
            emit toggled(checked);
        });
    }
};

} // namespace Fernanda
