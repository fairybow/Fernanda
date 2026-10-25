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
#include <QWidget>

#include <Coco/Debug.h>

#include "core/git/GitRepository.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// Names a new branch, made at the current commit and switched to
// (GitRepository::createBranch). Git is the judge of the name: Create asks
// it, and the dialog stays open, showing git's own words, until the branch is
// made or the dialog is cancelled. Checking a name as it's typed would mean
// copying git's rules for branch names, which are git's to keep.
//
// Window-modal, opened without exec, and deleted when it closes: the window
// may close while it's up
class GitNewBranchDialog : public QDialog
{
    Q_OBJECT

public:
    GitNewBranchDialog(GitRepository* repository, QWidget* parentWindow)
        : QDialog(parentWindow)
        , repository_(repository)
    {
        setup_();
    }

    ~GitNewBranchDialog() override { TRACER; }

private:
    GitRepository* repository_;

    QLineEdit* nameEdit_ = nullptr;
    QLabel* errorLabel_ = nullptr;
    QPushButton* createButton_ = nullptr;

    // Whether this dialog asked for a branch and awaits the answer, so an
    // answer to anything else asked of the repository is left alone
    bool waiting_ = false;

    void setup_()
    {
        setWindowTitle(tr("New Branch"));
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose);
        setMinimumWidth(360);

        nameEdit_ = new QLineEdit(this);
        nameEdit_->setPlaceholderText(tr("Branch name"));

        errorLabel_ = new QLabel(this);
        errorLabel_->setWordWrap(true);
        errorLabel_->hide();

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Cancel,
            Qt::Horizontal,
            this);
        createButton_ =
            buttons->addButton(tr("Create"), QDialogButtonBox::AcceptRole);
        createButton_->setDefault(true);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(nameEdit_);
        layout->addWidget(errorLabel_);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(
            buttons,
            &QDialogButtonBox::accepted,
            this,
            &GitNewBranchDialog::create_);

        connect(
            nameEdit_,
            &QLineEdit::textChanged,
            this,
            &GitNewBranchDialog::updateCreate_);

        connect(repository_, &GitRepository::switched, this, [this] {
            if (waiting_) {
                accept();
            }
        });

        connect(
            repository_,
            &GitRepository::switchFailed,
            this,
            [this](const QString& error) {
                if (!waiting_) {
                    return;
                }

                waiting_ = false;
                errorLabel_->setText(error);
                errorLabel_->show();
                updateCreate_();
            });

        updateCreate_();
    }

    void updateCreate_()
    {
        createButton_->setEnabled(
            !waiting_ && !nameEdit_->text().trimmed().isEmpty());
    }

    void create_()
    {
        auto name = nameEdit_->text().trimmed();

        if (name.isEmpty() || waiting_ || repository_->isBusy()) {
            return;
        }

        waiting_ = true;
        errorLabel_->hide();
        updateCreate_();

        repository_->createBranch(name);
    }
};

} // namespace Fernanda::Ui
