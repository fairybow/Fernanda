/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/UiUtility.h"
#include "ui/widgets/UserTextEntry.h"

namespace Suzuri::Ui {

class NewVaultDialog : public QDialog
{
    Q_OBJECT

public:
    NewVaultDialog(
        QWidget* parentManageVaults,
        const Coco::Path& initialLocation)
        : QDialog(parentManageVaults)
        , parentDir_(initialLocation)
    {
        setup_();
    }

    ~NewVaultDialog() override { TRACER; }

    // Only meaningful after exec() returns Accepted
    [[nodiscard]] Coco::Path vaultRoot() const
    {
        return parentDir_ / nameEdit_->text().trimmed();
    }

    // Shown after a failed create, set by ManageVaults. The next edit clears it
    // via revalidate_
    void setError(const QString& message) { setStatus_(message, false); }

private:
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* locationEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* createButton_ = nullptr;
    Coco::Path parentDir_{};

    void setup_()
    {
        setWindowTitle(tr("Create new vault"));
        setMinimumWidth(440);

        setupUi_();
        revalidate_();
    }

    void setupUi_()
    {
        auto root_layout = new QVBoxLayout(this);

        auto form = new QFormLayout;
        setupNameEdit_();
        form->addRow(tr("Name:"), nameEdit_);

        auto location_row = new QHBoxLayout;
        setupLocationEdit_();
        location_row->addWidget(locationEdit_, 1);
        location_row->addWidget(
            Ui::buildPushButton(
                tr("Browse…"),
                this,
                &NewVaultDialog::onBrowse_));

        form->addRow(tr("Location:"), location_row);

        root_layout->addLayout(form);

        setupStatusLabel_();
        root_layout->addWidget(statusLabel_);
        root_layout->addWidget(buildButtons_());
    }

    void setupNameEdit_()
    {
        nameEdit_ = new QLineEdit(this);
        nameEdit_->setPlaceholderText(tr("Vault name"));

        connect(
            nameEdit_,
            &QLineEdit::textChanged,
            this,
            &NewVaultDialog::revalidate_);
    }

    void setupLocationEdit_()
    {
        locationEdit_ = new QLineEdit(this);
        locationEdit_->setReadOnly(true);
        locationEdit_->setPlaceholderText(tr("Choose a location…"));

        // Seeded from the create/open MRU dir (Documents on first run) so the
        // user usually only needs to type a name; Browse still overrides it
        if (!parentDir_.isEmpty()) {
            locationEdit_->setText(parentDir_.toQString());
        }
    }

    void setupStatusLabel_()
    {
        statusLabel_ = new QLabel(this);
        statusLabel_->setWordWrap(true);
        statusLabel_->setEnabled(false);
    }

    QDialogButtonBox* buildButtons_()
    {
        auto buttons = new QDialogButtonBox(
            QDialogButtonBox::Cancel,
            Qt::Horizontal,
            this);

        createButton_ =
            buttons->addButton(tr("Create"), QDialogButtonBox::AcceptRole);
        createButton_->setDefault(true);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        return buttons;
    }

    void onBrowse_()
    {
        auto dir = Coco::getDir(this, tr("Choose a location"), parentDir_);
        if (dir.isEmpty()) {
            return; // cancelled
        }

        parentDir_ = dir;
        locationEdit_->setText(dir.toQString());
        revalidate_();
    }

    void revalidate_()
    {
        auto name = nameEdit_->text().trimmed();

        // Empty location or name is the neutral state — no error, no Create
        if (parentDir_.isEmpty() || name.isEmpty()) {
            setStatus_({}, false);
            return;
        }

        // Character / reserved-name / trailing rules, shared with file + folder
        // rename via ui/widgets/UserTextEntry.h. Advisory only — the mkdir in
        // ManageVaults is authoritative
        if (auto problem = UserTextEntry::problemOf(name);
            problem != UserTextEntry::Problem::None) {
            setStatus_(UserTextEntry::describe(problem), false);
            return;
        }

        if ((parentDir_ / name).exists()) {
            setStatus_(
                tr("A folder named \"%1\" already exists here.").arg(name),
                false);
            return;
        }

        setStatus_((parentDir_ / name).prettyQString(), true);
    }

    void setStatus_(const QString& text, bool enableCreate)
    {
        statusLabel_->setText(text);
        createButton_->setEnabled(enableCreate);
    }
};

} // namespace Suzuri::Ui
