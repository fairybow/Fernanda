/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

#include "Tr.h"

// Window-modal dialog utilities for prompting users to save, discard, or cancel
// unsaved changes, supporting both single and multiple file (with selection)
namespace Fernanda::SavePrompt {

enum Choice
{
    Cancel,
    Save,
    Discard
};

inline QString toQString(Choice choice) noexcept
{
    switch (choice) {
    default:
    case Cancel:
        return "SavePrompt::Cancel";
    case Save:
        return "SavePrompt::Save";
    case Discard:
        return "SavePrompt::Discard";
    }
}

struct MultiSaveResult
{
    Choice choice{};
    QList<int> selectedIndices{};
};

namespace Internal {

    inline void setCommonProperties_(QDialog& dialog)
    {
        // QDialog defaults to non-modal (though QMessageBox defaults to modal)
        dialog.setWindowModality(Qt::WindowModal);
        dialog.setMinimumSize(400, 200);
    }

} // namespace Internal

inline Choice exec(const QString& fileDisplayName, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    Internal::setCommonProperties_(box);
    box.setTextInteractionFlags(Qt::NoTextInteraction);

    box.setText(Tr::nxSavePromptBodyFormat().arg(fileDisplayName));

    // QMessageBox should handle platform-specific button ordering automatically
    auto save = box.addButton(Tr::save(), QMessageBox::AcceptRole);
    auto discard = box.addButton(Tr::dontSave(), QMessageBox::DestructiveRole);
    auto cancel = box.addButton(Tr::cancel(), QMessageBox::RejectRole);

    box.setDefaultButton(save);
    box.setEscapeButton(cancel);

    // TODO: Move to open/show
    box.exec();

    auto clicked = box.clickedButton();
    if (clicked == save) return Save;
    if (clicked == discard) return Discard;
    return Cancel;
}

inline MultiSaveResult
exec(const QStringList& fileDisplayNames, QWidget* parent = nullptr)
{
    if (fileDisplayNames.isEmpty()) return { Cancel, {} };

    // Delegate to single-file prompt
    if (fileDisplayNames.size() == 1) {
        auto choice = exec(fileDisplayNames.first(), parent);
        return { choice, (choice == Save) ? QList<int>{ 0 } : QList<int>{} };
    }

    QDialog dialog(parent);
    Internal::setCommonProperties_(dialog);

    auto main_layout = new QVBoxLayout(&dialog);

    // Message label
    auto message_label = new QLabel(&dialog);
    message_label->setTextInteractionFlags(Qt::NoTextInteraction);
    message_label->setText(
        Tr::nxSavePromptMultiBodyFormat().arg(fileDisplayNames.size()));
    message_label->setWordWrap(true);
    main_layout->addWidget(message_label);

    // Scroll area with checkboxes
    auto scroll_area = new QScrollArea(&dialog);
    auto scroll_widget = new QWidget;
    auto scroll_layout = new QVBoxLayout(scroll_widget);

    QList<QCheckBox*> checkboxes{};
    for (const auto& file_name : fileDisplayNames) {
        auto checkbox = new QCheckBox(file_name, scroll_widget);
        checkbox->setChecked(true);
        scroll_layout->addWidget(checkbox);
        checkboxes << checkbox;
    }

    scroll_area->setWidget(scroll_widget);
    scroll_area->setWidgetResizable(true);
    scroll_area->setMaximumHeight(300);
    main_layout->addWidget(scroll_area);

    // Buttons

    // To ensure platform-specific ordering, we'll use QDialogButtonBox
    auto button_box = new QDialogButtonBox(&dialog);
    auto save = button_box->addButton(Tr::save(), QDialogButtonBox::AcceptRole);
    button_box->addButton(Tr::dontSave(), QDialogButtonBox::DestructiveRole);
    auto cancel = button_box->addButton(Tr::cancel(), QDialogButtonBox::RejectRole);

    save->setDefault(true);
    // Escape button behavior is automatic with RejectRole

    dialog.connect(
        button_box,
        &QDialogButtonBox::clicked,
        &dialog,
        [&dialog, button_box](QAbstractButton* button) {
            switch (button_box->buttonRole(button)) {
            default:
            case QDialogButtonBox::RejectRole:
                dialog.done(Cancel);
                break;
            case QDialogButtonBox::AcceptRole:
                dialog.done(Save);
                break;
            case QDialogButtonBox::DestructiveRole:
                dialog.done(Discard);
                break;
            }
        });

    main_layout->addWidget(button_box);

    // TODO: Move to open/show
    auto result = dialog.exec();

    switch (result) {
    default:
    case Cancel:
    case Discard:
        return { static_cast<Choice>(result), {} };

    case Save: {
        QList<int> selected{};
        for (auto i = 0; i < checkboxes.size(); ++i)
            if (checkboxes[i]->isChecked()) selected << i;
        return { Save, selected };
    }
    }
}

} // namespace Fernanda::SavePrompt
