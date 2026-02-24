/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QGroupBox>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "Debug.h"
#include "Ini.h"

namespace Fernanda {

class SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPanel(const QString& title, QWidget* parent = nullptr)
        : QWidget(parent)
        , groupBox_(new QGroupBox(title, this))
    {
        setup_();
    }

    virtual ~SettingsPanel() override { TRACER; }

signals:
    void settingChanged(const QString& key, const QVariant& value);

protected:
    QGroupBox* groupBox() const noexcept { return groupBox_; }

private:
    QGroupBox* groupBox_;

    void setup_()
    {
        groupBox_->setCheckable(false);

        auto main_layout = new QVBoxLayout(this);
        main_layout->setContentsMargins(0, 0, 0, 0);

        auto group_box_layout = new QVBoxLayout;
        groupBox_->setLayout(group_box_layout);

        main_layout->addWidget(groupBox_);
    }
};

} // namespace Fernanda
