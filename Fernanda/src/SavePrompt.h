/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <Qt>

#include "Coco/Layout.h"

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
        // dialog.setWindowTitle(Tr::Dialogs::savePromptTitle());
        dialog.setMinimumSize(400, 200);
    }

    // TODO: Other sections from the exec functions?

} // namespace Internal

inline Choice exec(const QString& fileDisplayName, QWidget* parent = nullptr)
{
    QMessageBox box(parent);
    Internal::setCommonProperties_(box);

    box.setText(Tr::Dialogs::savePromptBodyFormat().arg(fileDisplayName));

    auto save = box.addButton(Tr::Buttons::save(), QMessageBox::AcceptRole);
    auto discard =
        box.addButton(Tr::Buttons::discard(), QMessageBox::DestructiveRole);
    auto cancel = box.addButton(Tr::Buttons::cancel(), QMessageBox::RejectRole);

    box.setDefaultButton(save);
    box.setEscapeButton(cancel);

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

    auto main_layout = Coco::Layout::make<QVBoxLayout*>(&dialog);

    // Message label
    auto message_label = new QLabel(&dialog);
    message_label->setText(
        Tr::Dialogs::savePromptMultiBodyFormat().arg(fileDisplayNames.size()));
    message_label->setWordWrap(true);
    main_layout->addWidget(message_label);

    // Scroll area with checkboxes
    auto scroll_area = new QScrollArea(&dialog);
    auto scroll_widget = new QWidget;
    auto scroll_layout = Coco::Layout::make<QVBoxLayout*>(scroll_widget);

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
    auto button_layout = new QHBoxLayout;
    button_layout->addStretch();

    auto save_button = new QPushButton(Tr::Buttons::save(), &dialog);
    auto discard_button = new QPushButton(Tr::Buttons::discard(), &dialog);
    auto cancel_button = new QPushButton(Tr::Buttons::cancel(), &dialog);

    save_button->setDefault(true);

    button_layout->addWidget(save_button);
    button_layout->addWidget(discard_button);
    button_layout->addWidget(cancel_button);
    main_layout->addLayout(button_layout);

    // Track result
    Choice choice = Cancel;

    dialog.connect(save_button, &QPushButton::clicked, [&] {
        choice = Save;
        dialog.accept();
    });

    dialog.connect(discard_button, &QPushButton::clicked, [&] {
        choice = Discard;
        dialog.accept();
    });

    dialog.connect(cancel_button, &QPushButton::clicked, [&] {
        choice = Cancel;
        dialog.reject();
    });

    dialog.exec();

    if (choice == Save) {
        QList<int> selected{};
        for (auto i = 0; i < checkboxes.size(); ++i)
            if (checkboxes[i]->isChecked()) selected << i;
        return { Save, selected };
    }

    return { choice, {} };
}

} // namespace Fernanda::SavePrompt
