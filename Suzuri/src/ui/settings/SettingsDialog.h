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

#include <QAction>
#include <QDialog>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPalette>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Updater.h"
#include "core/Vault.h"
#include "core/spell/SpellCheckers.h"
#include "ui/UiConstants.h"
#include "ui/settings/SettingsAppearancePage.h"
#include "ui/settings/SettingsEditorPage.h"
#include "ui/settings/SettingsGeneralPage.h"
#include "ui/settings/SettingsStatusBarPage.h"

namespace Suzuri::Ui {

// One vault's settings, modeled on Obsidian's settings modal: the vault's name
// and a navigation list down the left — pages under group titles ("Options") —
// and the chosen page on the right. Changes apply as they're made; there is no
// OK / Apply. Esc or the title bar's close hides it.
//
// Window-modal, opened with open() and never exec() (VaultWindow::
// onSettingsRequested_). Window-modal blocks only the window that opened it,
// as Obsidian's backdrop blocks only its own window; other vault windows and
// this window's pop-outs stay usable. Never exec(): its nested event loop
// would run inside the VaultWindow's own slot, and Ctrl+Q from in here closes
// and deletes that window (App::onQuitRequested_) while the loop is still on
// its stack. open() returns at once, so the dialog is just a child that dies
// with its window.
//
// Ctrl+Q works from in here because the quit action is added to the dialog
// too, as ManageVaults does: a window shortcut only fires in the active
// window, and a dialog is its own window.
//
// Built once per VaultWindow, on first use, and hidden rather than destroyed
// on close, so it reopens on the page last viewed — as Obsidian's does within
// a session. It borrows the Vault, the same object the window borrows; pages
// edit settings through Vault::setConfig, which applies and saves them. No
// setting-changed signal leaves the dialog. It borrows App's SpellCheckers too,
// for the Editor page's list of languages, and App's Updater, for the General
// page
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(
        Vault* vault,
        SpellCheckers* spellCheckers,
        Updater* updater,
        QAction* quitAction,
        QWidget* parentVaultWindow)
        : QDialog(parentVaultWindow)
        , vault_(vault)
        , spellCheckers_(spellCheckers)
        , updater_(updater)
    {
        setup_(quitAction);
    }

    ~SettingsDialog() override { TRACER; }

    // Select the General page, as the status bar's update notice does before
    // it opens the dialog
    void showGeneralPage() { selectPage_(generalPage_); }

protected:
    void changeEvent(QEvent* event) override
    {
        QDialog::changeEvent(event);

        if (event->type() == QEvent::PaletteChange) {
            recolorGroupTitles_();
        }
    }

private:
    // Where a page row keeps its page's index in the stack (addPage_)
    static constexpr auto PAGE_INDEX_ROLE_ = Qt::UserRole;

    Vault* vault_ = nullptr;

    // App's dictionaries, for the Editor page's language list. Not owned
    SpellCheckers* spellCheckers_ = nullptr;

    // App's update checker, for the General page. Not owned
    Updater* updater_ = nullptr;

    QLabel* vaultName_ = new QLabel(this);
    QListWidget* nav_ = new QListWidget(this);
    QStackedWidget* pages_ = new QStackedWidget(this);
    SettingsGeneralPage* generalPage_ = nullptr;

    // The nav's group-title rows, recolored on palette change. Owned by nav_
    QList<QListWidgetItem*> groupTitles_{};

    void setup_(QAction* quitAction)
    {
        auto name = vault_->root().nameQString();
        setWindowTitle(tr("Settings — %1").arg(name));
        resize(SETTINGS_DIALOG_WIDTH, SETTINGS_DIALOG_HEIGHT);

        addAction(quitAction);

        setupNavColumn_(name);

        addGroupTitle_(tr("Options"));
        generalPage_ = new SettingsGeneralPage(vault_, updater_, this);
        addPage_(tr("General"), generalPage_);
        addPage_(
            tr("Editor"),
            new SettingsEditorPage(vault_, spellCheckers_, this));
        addPage_(tr("Appearance"), new SettingsAppearancePage(vault_, this));
        addPage_(tr("Status bar"), new SettingsStatusBarPage(vault_, this));

        // Selecting a page row shows its page. Group titles can't be selected
        // (addGroupTitle_), so every row that becomes current is a page row
        connect(
            nav_,
            &QListWidget::currentItemChanged,
            this,
            [this](QListWidgetItem* current) {
                if (!current) {
                    return;
                }

                pages_->setCurrentIndex(
                    current->data(PAGE_INDEX_ROLE_).toInt());
            });

        selectFirstPage_();

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        auto* nav_column = new QVBoxLayout;
        nav_column->setContentsMargins(
            SETTINGS_NAV_MARGIN,
            SETTINGS_NAV_MARGIN,
            SETTINGS_NAV_MARGIN,
            SETTINGS_NAV_MARGIN);
        nav_column->setSpacing(SETTINGS_NAV_MARGIN);
        nav_column->addWidget(vaultName_);
        nav_column->addWidget(nav_, 1);

        auto* nav_host = new QWidget(this);
        nav_host->setLayout(nav_column);
        nav_host->setFixedWidth(SETTINGS_NAV_WIDTH);

        auto* divider = new QFrame(this);
        divider->setFrameShape(QFrame::VLine);
        divider->setFrameShadow(QFrame::Plain);
        divider->setForegroundRole(QPalette::Mid);

        layout->addWidget(nav_host);
        layout->addWidget(divider);
        layout->addWidget(pages_, 1);
    }

    // The vault's name above the list, so it's plain whose settings these are
    // — the dialog belongs to one vault, and several can be open at once
    void setupNavColumn_(const QString& name)
    {
        auto font = vaultName_->font();
        font.setBold(true);
        vaultName_->setFont(font);
        vaultName_->setText(name);
        vaultName_->setToolTip(vault_->root().prettyQString());

        nav_->setFrameShape(QFrame::NoFrame);
        nav_->setSelectionMode(QAbstractItemView::SingleSelection);

        // Fill with the column's own background rather than the list's Base
        // white, so the nav reads as a sidebar beside the page
        nav_->viewport()->setAutoFillBackground(false);
        nav_->setAutoFillBackground(false);
    }

    // A group title (Obsidian's "Options", "Core plugins"): a row that can be
    // neither selected nor focused, so clicks and arrow keys pass over it,
    // drawn in the muted role
    void addGroupTitle_(const QString& title)
    {
        auto* item = new QListWidgetItem(title, nav_);
        item->setFlags(Qt::NoItemFlags);

        auto font = item->font();
        font.setBold(true);
        item->setFont(font);

        groupTitles_ << item;
        recolorGroupTitles_();
    }

    // An item's foreground is a fixed color, not a role, so it's set again
    // whenever the palette changes (changeEvent) — an OS light/dark switch
    // included — or the titles would keep the old theme's color
    void recolorGroupTitles_()
    {
        auto color = palette().color(SETTINGS_NAV_GROUP_TITLE_ROLE);

        for (auto* item : groupTitles_) {
            item->setForeground(color);
        }
    }

    // A page row and its page. The row carries the page's index in the stack,
    // since group titles make row and page positions differ
    void addPage_(const QString& label, QWidget* page)
    {
        auto index = pages_->addWidget(page);
        auto* item = new QListWidgetItem(label, nav_);
        item->setData(PAGE_INDEX_ROLE_, index);
    }

    void selectPage_(QWidget* page)
    {
        auto index = pages_->indexOf(page);

        for (auto row = 0; row < nav_->count(); ++row) {
            auto* item = nav_->item(row);
            if ((item->flags() & Qt::ItemIsSelectable) &&
                item->data(PAGE_INDEX_ROLE_).toInt() == index) {
                nav_->setCurrentItem(item);
                return;
            }
        }
    }

    void selectFirstPage_()
    {
        for (auto row = 0; row < nav_->count(); ++row) {
            auto* item = nav_->item(row);
            if (item->flags() & Qt::ItemIsSelectable) {
                nav_->setCurrentItem(item);
                return;
            }
        }
    }
};

} // namespace Suzuri::Ui
