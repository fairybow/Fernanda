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

#include "core/git/GitRefName.h"
#include "core/git/GitRepository.h"
#include "ui/UiConstants.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// Names a new branch, made at the current commit and switched to
// (GitRepository::createBranch). The name is fixed as it's typed, as GitHub
// Desktop fixes it (GitRefName::sanitized): when that changes it, a line under
// the field says what the branch will be called. Git still judges the name on
// Create, and a name it refuses keeps the dialog open, showing git's own
// words, until the branch is made or the dialog is cancelled.
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
    QLabel* noteLabel_ = nullptr;
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
        setMinimumWidth(GIT_NEW_BRANCH_DIALOG_MIN_WIDTH);

        nameEdit_ = new QLineEdit(this);
        nameEdit_->setPlaceholderText(tr("Branch name"));

        noteLabel_ = new QLabel(this);
        noteLabel_->setForegroundRole(GIT_NEW_BRANCH_NOTE_ROLE);
        noteLabel_->hide();

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
        layout->addWidget(noteLabel_);
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
            &GitNewBranchDialog::onNameChanged_);

        connect(
            repository_,
            &GitRepository::busyChanged,
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

    // The name the branch would get, as typed and fixed
    [[nodiscard]] QString name_() const
    {
        return GitRefName::sanitized(nameEdit_->text().trimmed());
    }

    // The note says what the branch will be called, when fixing the name
    // changed it. Git's last refusal no longer applies to a new name
    void onNameChanged_()
    {
        auto typed = nameEdit_->text().trimmed();
        auto name = name_();

        noteLabel_->setText(tr("Will be created as %1").arg(name));
        noteLabel_->setVisible(!name.isEmpty() && name != typed);
        errorLabel_->hide();

        updateCreate_();
    }

    // A name, and nothing underway: neither this dialog's own branch nor
    // anything else the repository is busy with
    void updateCreate_()
    {
        createButton_->setEnabled(
            !waiting_ && !repository_->isBusy() && !name_().isEmpty());
    }

    void create_()
    {
        auto name = name_();

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
