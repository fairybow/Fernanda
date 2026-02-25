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
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "Debug.h"
#include "DisplaySlider.h"
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

    void connectGroupBox(const QString& key)
    {
        connect(
            groupBox_,
            &QGroupBox::toggled,
            this,
            [this, key](bool toggled) { emit settingChanged(key, toggled); });
    }

    void connectCheckBox(QCheckBox* box, const QString& key)
    {
        connect(box, &QCheckBox::toggled, this, [this, key](bool toggled) {
            emit settingChanged(key, toggled);
        });
    }

    void connectComboBox(QComboBox* box, const QString& key)
    {
        connect(
            box,
            &QComboBox::currentIndexChanged,
            this,
            [this, key, box](int index) {
                emit settingChanged(key, box->itemData(index));
            });
    }

    void connectDisplaySlider(DisplaySlider* slider, const QString& key)
    {
        connect(
            slider,
            &DisplaySlider::valueChanged,
            this,
            [this, key](int value) { emit settingChanged(key, value); });
    }

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
