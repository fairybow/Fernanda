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

#include "ControlField.h"
#include "Debug.h"
#include "Ini.h"
#include "SettingsPanel.h"
#include "Tr.h"

namespace Fernanda {

/// TODO KFS
class KeyFiltersPanel : public SettingsPanel
{
    Q_OBJECT

public:
    explicit KeyFiltersPanel(
        const QVariantMap& values,
        QWidget* parent = nullptr)
        : SettingsPanel(Tr::keyFiltersPanelTitle(), parent)
    {
        setup_(values);
    }

    virtual ~KeyFiltersPanel() override { TRACER; }

private:
    QCheckBox* autoCloseCheck_ = new QCheckBox(this);
    ControlField<QCheckBox>* barging_ =
        new ControlField<QCheckBox>(FieldKind::Info, this);

    void setup_(const QVariantMap& values)
    {
        // Populate
        auto group_box = groupBox();
        group_box->setCheckable(true);
        group_box->setChecked(values[Ini::Keys::KEY_FILTERS_ACTIVE].toBool());

        autoCloseCheck_->setText(Tr::keyFiltersPanelAutoClose());
        autoCloseCheck_->setChecked(
            values[Ini::Keys::KEY_FILTERS_AUTO_CLOSE].toBool());

        auto barging_check = barging_->control();
        barging_check->setText(Tr::keyFiltersPanelBarging());
        barging_check->setChecked(
            values[Ini::Keys::KEY_FILTERS_BARGING].toBool());
        barging_->setInfo(Tr::keyFiltersPanelBargingTooltip());

        // Layout
        auto layout = group_box->layout();
        layout->addWidget(autoCloseCheck_);
        layout->addWidget(barging_);

        // Connect
        connectGroupBox(Ini::Keys::KEY_FILTERS_ACTIVE);
        connectCheckBox(autoCloseCheck_, Ini::Keys::KEY_FILTERS_AUTO_CLOSE);
        connectCheckBox(barging_check, Ini::Keys::KEY_FILTERS_BARGING);
    }
};

} // namespace Fernanda
