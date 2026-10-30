/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"

namespace Fernanda::Ui {

// One setting on a settings page — Obsidian's setting item: the setting's name,
// and under it a smaller muted description, on the left; its control on the
// right, vertically centered. Every setting on every page is one of these, so
// the pages only choose controls and wording. The line between rows is a
// separate SettingRule (below) that the page lays in, not something a row
// paints on itself.
//
// The row takes ownership of the control (the layout reparents it) and knows
// nothing about what it does: the page connects the control to the Vault. An
// empty description leaves the name alone, centered on the control. A page
// whose description reports something that changes (the General page's update
// check) sets it again with setDescription.
//
// Text colors are roles set with setForegroundRole, not colors read once, so
// they follow palette and color-group changes like everything Qt paints
class SettingRow : public QWidget
{
    Q_OBJECT

public:
    SettingRow(
        const QString& name,
        const QString& description,
        QWidget* control,
        QWidget* parentPage)
        : QWidget(parentPage)
    {
        setup_(name, description, control);
    }

    ~SettingRow() override { TRACER; }

    // Replace the description. An empty one hides it, leaving the name alone
    void setDescription(const QString& description)
    {
        if (!description_) {
            if (description.isEmpty()) {
                return;
            }

            addDescription_();
        }

        description_->setText(description);
        description_->setVisible(!description.isEmpty());
    }

private:
    QVBoxLayout* info_ = new QVBoxLayout;

    // Made when the row first has a description
    QLabel* description_ = nullptr;

    void
    setup_(const QString& name, const QString& description, QWidget* control)
    {
        info_->setContentsMargins(0, 0, 0, 0);
        info_->setSpacing(SETTING_ROW_INFO_SPACING);

        auto* name_label = new QLabel(name, this);
        name_label->setWordWrap(true);
        info_->addWidget(name_label);

        setDescription(description);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(
            0,
            SETTING_ROW_V_PADDING,
            0,
            SETTING_ROW_V_PADDING);
        layout->setSpacing(SETTING_ROW_CONTROL_SPACING);
        layout->addLayout(info_, 1);
        layout->addWidget(control, 0, Qt::AlignVCenter);
    }

    void addDescription_()
    {
        description_ = new QLabel(this);
        description_->setWordWrap(true);
        description_->setForegroundRole(SETTING_DESCRIPTION_ROLE);

        // A font set in pixels reports no point size; it stays full size
        auto font = description_->font();
        if (font.pointSizeF() > 0) {
            font.setPointSizeF(
                font.pointSizeF() * SETTING_DESCRIPTION_FONT_SCALE);
            description_->setFont(font);
        }

        info_->addWidget(description_);
    }
};

// The thin line above each row on a settings page (Obsidian's row border),
// laid in by the page before every SettingRow. A plain widget one pixel tall
// that fills itself with its background role — no custom painting — so it
// can't be skipped or painted over, and it follows palette changes like any
// widget's background.
//
// A line drawn by each row along its own top edge in paintEvent shows above
// some rows and not others on Windows 11 (above the rows holding a combo box
// and a slider, never above those holding a switch), for a reason not pinned
// down. A widget of its own takes that painting path out of the picture
class SettingRule : public QWidget
{
    Q_OBJECT

public:
    explicit SettingRule(QWidget* parentPage)
        : QWidget(parentPage)
    {
        setup_();
    }

    ~SettingRule() override { TRACER; }

private:
    void setup_()
    {
        setFixedHeight(SETTING_ROW_RULE_WIDTH);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setBackgroundRole(SETTING_ROW_RULE_ROLE);
        setAutoFillBackground(true);
    }
};

// A section heading on a settings page (Obsidian's setting-item heading, e.g.
// Appearance → Font): bold text, with room above it to set the section off
// from the one before. No rule of its own; the rule the page lays in before
// the first row under it is the line beneath
class SettingHeading : public QLabel
{
    Q_OBJECT

public:
    SettingHeading(const QString& text, QWidget* parentPage)
        : QLabel(text, parentPage)
    {
        setup_();
    }

    ~SettingHeading() override { TRACER; }

private:
    void setup_()
    {
        auto bold = font();
        bold.setBold(true);
        setFont(bold);
        setContentsMargins(
            0,
            SETTING_HEADING_TOP_PADDING,
            0,
            SETTING_HEADING_BOTTOM_PADDING);
    }
};

} // namespace Fernanda::Ui
