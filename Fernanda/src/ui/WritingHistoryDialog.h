/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
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

#include <QAction>
#include <QDate>
#include <QDialog>
#include <QLabel>
#include <QLocale>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Time.h>

#include "core/CoreConstants.h"
#include "core/Vault.h"
#include "core/WritingHistory.h"
#include "ui/UiConstants.h"
#include "ui/UiUtility.h"
#include "ui/settings/SettingRow.h"

namespace Fernanda::Ui {

// One vault's writing history (Vault::history): what was written today and
// over the vault's life, from every machine that has written in it. Laid out
// as a settings page is, with the same headings and rows, each row's control
// being its figure. Read-only: there is nothing to set.
//
// The figures follow the history as it changes (Vault::historyChanged), since
// the dialog blocks only its own window: typing in a pop-out, or an undo
// there, goes on while it shows. Today is the local date at each update.
//
// Window-modal and opened with open(), never exec() (see "Dialogs and event
// loops" in docs/Architecture.md). It deletes itself when closed. Ctrl+Q
// works from in here because the quit action is added to it, as the settings
// dialog does
class WritingHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    WritingHistoryDialog(
        Vault* vault,
        QAction* quitAction,
        QWidget* parentVaultWindow)
        : QDialog(parentVaultWindow)
        , vault_(vault)
    {
        setup_(quitAction);
    }

    ~WritingHistoryDialog() override { TRACER; }

private:
    // The labels showing one span's figures
    struct Figures_
    {
        QLabel* netWords = nullptr;
        QLabel* addedWords = nullptr;
        QLabel* activeTime = nullptr;
    };

    Vault* vault_;
    QVBoxLayout* layout_ = new QVBoxLayout(this);

    Figures_ today_{};
    Figures_ lifetime_{};
    QLabel* daysWritten_ = nullptr;

    void setup_(QAction* quitAction)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setMinimumWidth(WRITING_HISTORY_DIALOG_MIN_WIDTH);
        addAction(quitAction);

        layout_->setContentsMargins(
            SETTINGS_PAGE_MARGIN,
            SETTINGS_PAGE_MARGIN,
            SETTINGS_PAGE_MARGIN,
            SETTINGS_PAGE_MARGIN);
        layout_->setSpacing(0);

        setupToday_();
        setupLifetime_();
        layout_->addStretch(1);

        showVaultName_();
        connect(
            vault_,
            &Vault::rootMoved,
            this,
            &WritingHistoryDialog::showVaultName_);

        showFigures_();
        connect(
            vault_,
            &Vault::historyChanged,
            this,
            &WritingHistoryDialog::showFigures_);
    }

    // What each figure means is said once, here, and not again under Lifetime
    void setupToday_()
    {
        constexpr auto idle_minutes =
            static_cast<int>(WRITING_IDLE_LIMIT_MS / (60 * 1000));

        addHeading_(tr("Today"));

        today_.netWords = addFigure_(
            tr("Net words"),
            tr("Words added minus words deleted."));

        today_.addedWords = addFigure_(
            tr("Words written"),
            tr("Words added, not counting any deleted."));

        today_.activeTime = addFigure_(
            tr("Time writing"),
            tr("Time spent editing. (A pause of more than %n minute(s) isn't "
               "counted.)",
               nullptr,
               idle_minutes));
    }

    void setupLifetime_()
    {
        addHeading_(tr("Lifetime"));

        lifetime_.netWords = addFigure_(tr("Net words"), {});
        lifetime_.addedWords = addFigure_(tr("Words written"), {});
        lifetime_.activeTime = addFigure_(tr("Time writing"), {});

        daysWritten_ = addFigure_(
            tr("Days written"),
            tr("Days with any writing recorded."));
    }

    void addHeading_(const QString& title)
    {
        layout_->addWidget(new SettingHeading(title, this));
    }

    // A row with the rule above it, as a settings page lays one in. Returns
    // the label that shows the row's figure
    QLabel* addFigure_(const QString& name, const QString& description)
    {
        auto* figure = new QLabel(this);
        figure->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setTabularNumerals(figure);

        layout_->addWidget(new SettingRule(this));
        layout_->addWidget(new SettingRow(name, description, figure, this));
        return figure;
    }

    // The vault's name in the title, so it's plain whose history this is. A
    // folder with no name of its own, a drive's root, shows its path
    void showVaultName_()
    {
        auto root = vault_->root();
        auto name = root.nameQString();

        if (name.isEmpty()) {
            name = root.prettyQString();
        }

        setWindowTitle(tr("Writing History — %1").arg(name));
    }

    void showFigures_()
    {
        const auto& history = vault_->history();

        showSpan_(today_, history.day(QDate::currentDate()));
        showSpan_(lifetime_, history.lifetime());
        daysWritten_->setText(QLocale().toString(history.daysWritten()));
    }

    // Numbers in the user's locale's format; times as a clock reads a length
    // of time, rounded down as elapsed time is
    static void showSpan_(Figures_& figures, const WritingHistory::Day& span)
    {
        QLocale locale{};

        figures.netWords->setText(locale.toString(span.netWords));
        figures.addedWords->setText(locale.toString(span.addedWords));
        figures.activeTime->setText(
            Coco::Time::clockQString(span.activeMs, Coco::Time::RoundUp::No));
    }
};

} // namespace Fernanda::Ui
