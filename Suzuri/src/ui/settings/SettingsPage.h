/*
 * Suzuri — A plain-text editor for creative writing
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QAbstractButton>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "ui/UiConstants.h"
#include "ui/settings/SettingRow.h"
#include "ui/widgets/ToggleSwitch.h"

namespace Suzuri::Ui {

// The base of every page in the settings dialog: a scroll area over a single
// column of headings and rows, top-aligned by a trailing stretch, so a page
// that outgrows the dialog scrolls rather than squeezing its rows. The column
// and its helpers are all the pages have in common, and are all that's here.
//
// A page borrows the Vault (vault()), seeds each control from its config
// once, in its constructor, and connects each control straight to the
// Vault's setter — which applies the change wherever it shows and saves it
// (Vault::onConfigChanged_). Nothing flows back: a page is the only editor of
// its settings while it's open (one window per vault, one dialog per window),
// so its controls can't go stale. A hand edit of a config file is read only
// when the vault next opens. The General page is the exception: its settings
// are App's, shared by every vault's dialog, so it follows them (see there)
class SettingsPage : public QScrollArea
{
    Q_OBJECT

public:
    SettingsPage(Vault* vault, QWidget* parentSettingsDialog)
        : QScrollArea(parentSettingsDialog)
        , vault_(vault)
    {
        setup_();
    }

    ~SettingsPage() override { TRACER; }

protected:
    [[nodiscard]] Vault* vault() const noexcept { return vault_; }

    // The parent for a control a page builds; addRow's row then takes it
    [[nodiscard]] QWidget* content() const noexcept { return content_; }

    // A section heading, below everything added so far
    void addHeading(const QString& title)
    {
        insert_(new SettingHeading(title, content_));
    }

    // A setting row, with the rule above it — the first row under a heading
    // included, whose rule is the line beneath the heading. Returns the row
    // for a page that needs to disable it (a setting that only applies while
    // another is on); most callers ignore it.
    //
    // No button in the control is ever the dialog's default
    // (clearAutoDefaults_)
    SettingRow*
    addRow(const QString& name, const QString& description, QWidget* control)
    {
        clearAutoDefaults_(control);
        insert_(new SettingRule(content_));

        auto* row = new SettingRow(name, description, control, content_);
        insert_(row);
        return row;
    }

    // A switch and the row it sits in: the switch for a page that hangs other
    // rows on it, the row for one that is hung. Most callers ignore both
    struct SwitchRow
    {
        ToggleSwitch* toggle = nullptr;
        SettingRow* row = nullptr;
    };

    // One on/off setting: a row holding a switch seeded with checked, each
    // flip going to the given VaultConfig setter, through the Vault
    SwitchRow addSwitch(
        const QString& name,
        const QString& description,
        bool checked,
        bool (VaultConfig::*setter)(bool))
    {
        auto* toggle = new ToggleSwitch(content_);
        toggle->setChecked(checked);
        connect(
            toggle,
            &QAbstractButton::toggled,
            this,
            [this, setter](bool now_checked) {
                vault_->setConfig(setter, now_checked);
            });

        return { toggle, addRow(name, description, toggle) };
    }

private:
    Vault* vault_ = nullptr;

    QWidget* content_ = new QWidget(this);
    QVBoxLayout* layout_ = new QVBoxLayout(content_);

    void setup_()
    {
        setFrameShape(QFrame::NoFrame);
        setWidgetResizable(true); // content tracks the page's width

        layout_->setContentsMargins(
            SETTINGS_PAGE_MARGIN,
            SETTINGS_PAGE_MARGIN,
            SETTINGS_PAGE_MARGIN,
            SETTINGS_PAGE_MARGIN);
        layout_->setSpacing(0);
        layout_->addStretch(1);

        setWidget(content_);
    }

    // The settings dialog has nothing for Enter to confirm. In a QDialog,
    // though, a push button is auto-default unless told otherwise: with
    // nothing else the default, one becomes it, Enter anywhere presses it,
    // and the style marks it in the accent. So no button in a row is
    // auto-default, the control itself included when it's a button
    static void clearAutoDefaults_(QWidget* control)
    {
        if (auto* button = qobject_cast<QPushButton*>(control)) {
            button->setAutoDefault(false);
        }

        for (auto* button : control->findChildren<QPushButton*>()) {
            button->setAutoDefault(false);
        }
    }

    // Append above the trailing stretch, which keeps the column top-aligned
    void insert_(QWidget* widget)
    {
        layout_->insertWidget(layout_->count() - 1, widget);
    }
};

} // namespace Suzuri::Ui
