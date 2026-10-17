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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/widgets/UserTextEntry.h"

namespace Fernanda::Ui {

// One-field rename prompt for a tree entry. The user edits the STEM — a file's
// extension is preserved and reattached in newLeaf(), a folder has none.
// Validation mirrors NewVaultDialog: character/reserved rules from
// UserTextEntry, then a collision check, both advisory — the Vault's
// Coco::rename is authoritative. A case-only edit ("foo" -> "Foo") is allowed
// through: it resolves to the same file, which the Vault renames via a temp
// step
class RenameDialog : public QDialog
{
    Q_OBJECT

public:
    RenameDialog(
        QWidget* parent,
        const Coco::Path& parentDir,
        const QString& originalLeaf,
        const QString& initialStem,
        const QString& suffix)
        : QDialog(parent)
        , parentDir_(parentDir)
        , originalLeaf_(originalLeaf)
        , suffix_(suffix)
    {
        setup_(initialStem);
    }

    ~RenameDialog() override { TRACER; }

    // The new on-disk leaf name (edited stem + preserved suffix). Meaningful
    // only after exec() returns Accepted
    [[nodiscard]] QString newLeaf() const
    {
        return nameEdit_->text().trimmed() + suffix_;
    }

private:
    QLineEdit* nameEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* okButton_ = nullptr;
    Coco::Path parentDir_{};
    QString originalLeaf_;
    QString suffix_;

    void setup_(const QString& initialStem)
    {
        setWindowTitle(tr("Rename"));
        setMinimumWidth(360);

        setupUi_(initialStem);
        revalidate_();
    }

    void setupUi_(const QString& initialStem)
    {
        auto root_layout = new QVBoxLayout(this);

        setupNameEdit_(initialStem);
        root_layout->addWidget(nameEdit_);

        setupStatusLabel_();
        root_layout->addWidget(statusLabel_);
        root_layout->addWidget(buildButtons_());
    }

    void setupNameEdit_(const QString& initialStem)
    {
        nameEdit_ = new QLineEdit(this);
        nameEdit_->setText(initialStem);
        nameEdit_->selectAll(); // overtype the current name straight away

        connect(
            nameEdit_,
            &QLineEdit::textChanged,
            this,
            &RenameDialog::revalidate_);
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
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            this);

        okButton_ = buttons->button(QDialogButtonBox::Ok);
        okButton_->setDefault(true);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        return buttons;
    }

    void revalidate_()
    {
        auto stem = nameEdit_->text().trimmed();

        // Empty is the neutral state — no error, no OK
        if (stem.isEmpty()) {
            setStatus_({}, false);
            return;
        }

        // Character / reserved / trailing rules (shared with vault creation)
        if (auto problem = UserTextEntry::problemOf(stem);
            problem != UserTextEntry::Problem::None) {
            setStatus_(UserTextEntry::describe(problem), false);
            return;
        }

        auto leaf = stem + suffix_;

        // No change — nothing to do. Exact match only; a case-only edit differs
        // and falls through as a real (allowed) rename
        if (leaf == originalLeaf_) {
            setStatus_({}, false);
            return;
        }

        // A collision with a DIFFERENT entry. A case-only rename resolves to
        // the same file, so don't count it as a collision — the Vault handles
        // it
        auto same_file =
            (leaf.compare(originalLeaf_, Qt::CaseInsensitive) == 0);
        if (!same_file && (parentDir_ / leaf).exists()) {
            setStatus_(tr("\"%1\" already exists here.").arg(leaf), false);
            return;
        }

        setStatus_({}, true);
    }

    void setStatus_(const QString& text, bool enableOk)
    {
        statusLabel_->setText(text);
        okButton_->setEnabled(enableOk);
    }
};

} // namespace Fernanda::Ui
