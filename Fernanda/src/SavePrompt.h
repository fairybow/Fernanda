#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

#include "Coco/Debug.h"
#include "Coco/Layout.h"
#include "Coco/Path.h"

#include "IFileModel.h"
#include "Tr.h"

namespace Fernanda {

enum class SaveChoice
{
    Cancel = 0,
    Save,
    Discard
};

// Window-modal dialog utilities for prompting users to save, discard, or cancel
// unsaved changes, supporting both single and multiple file scenarios as well
// as selection
namespace SavePrompt {

    namespace Internal {

        inline void setCommonProperties_(QDialog& dialog)
        {
            dialog.setWindowModality(Qt::WindowModal);
            dialog.setWindowTitle(Tr::Dialogs::savePromptTitle());
            dialog.setMinimumSize(400, 200);
        }

    } // namespace Internal

    struct Result
    {
        SaveChoice choice{};
        QList<IFileModel*> chosenSaves{};
    };

    /// WIP
    inline SaveChoice exec(IFileModel* model, QWidget* parent = nullptr)
    {
        if (!model) return SaveChoice::Cancel;
        auto meta = model->meta();
        if (!meta) return SaveChoice::Cancel;

        QMessageBox box(parent);
        Internal::setCommonProperties_(box);
        box.setText(
            Tr::Dialogs::savePromptSingleFileBodyFormat().arg(meta->title()));

        auto save = box.addButton(Tr::Buttons::save(), QMessageBox::AcceptRole);
        auto discard =
            box.addButton(Tr::Buttons::discard(), QMessageBox::AcceptRole);
        auto cancel =
            box.addButton(Tr::Buttons::cancel(), QMessageBox::AcceptRole);
        box.setDefaultButton(save);
        box.setEscapeButton(cancel);

        box.exec();

        SaveChoice result{};
        auto clicked = box.clickedButton();

        if (clicked == save) {
            result = SaveChoice::Save;
        } else if (clicked == discard) {
            result = SaveChoice::Discard;
        } else if (clicked == cancel) {
            result = SaveChoice::Cancel;
        }

        return result;
    }

    /// VERY WIP
    inline Result
    exec(QList<IFileModel*> modifiedModels, QWidget* parent = nullptr)
    {
        if (modifiedModels.isEmpty()) return { SaveChoice::Cancel, {} };

        // Single file case - use simpler message box
        if (modifiedModels.size() == 1) {
            auto choice = exec(modifiedModels.first(), parent);
            return { choice,
                     choice == SaveChoice::Save ? modifiedModels
                                                : QList<IFileModel*>{} };
        }

        QDialog dialog(parent);
        Internal::setCommonProperties_(dialog);

        // Main layout
        auto main_layout = Coco::Layout::make<QVBoxLayout*>(&dialog);

        // Message label
        auto messageLabel = new QLabel(&dialog);
        QString messageText = QString("You have unsaved changes in %1 file(s). "
                                      "Select which files to save:")
                                  .arg(modifiedModels.size());
        messageLabel->setText(messageText);
        messageLabel->setWordWrap(true);
        main_layout->addWidget(messageLabel);

        // Scroll area with checkboxes
        auto scrollArea = new QScrollArea(&dialog);
        auto scrollWidget = new QWidget();
        auto scrollLayout = Coco::Layout::make<QVBoxLayout*>(scrollWidget);

        QList<QCheckBox*> checkBoxes;
        for (auto model : modifiedModels) {
            auto meta = model->meta();
            auto title = meta ? meta->title() : "Untitled";
            auto checkBox = new QCheckBox(title, scrollWidget);
            checkBox->setChecked(true); // All checked by default
            scrollLayout->addWidget(checkBox);
            checkBoxes << checkBox;
        }

        scrollWidget->setLayout(scrollLayout);
        scrollArea->setWidget(scrollWidget);
        scrollArea->setWidgetResizable(true);
        scrollArea->setMaximumHeight(300); // Limit scroll area height
        main_layout->addWidget(scrollArea);

        // Button layout
        auto buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();

        auto saveButton = new QPushButton(Tr::Buttons::save(), &dialog);
        auto discardButton = new QPushButton(Tr::Buttons::discard(), &dialog);
        auto cancelButton = new QPushButton(Tr::Buttons::cancel(), &dialog);

        saveButton->setDefault(true);

        buttonLayout->addWidget(saveButton);
        buttonLayout->addWidget(discardButton);
        buttonLayout->addWidget(cancelButton);
        main_layout->addLayout(buttonLayout);

        // Connect buttons
        SaveChoice choice = SaveChoice::Cancel;

        // Should we use saveButton->connect...
        QObject::connect(saveButton, &QPushButton::clicked, [&]() {
            choice = SaveChoice::Save;
            dialog.accept();
        });

        QObject::connect(discardButton, &QPushButton::clicked, [&]() {
            choice = SaveChoice::Discard;
            dialog.accept();
        });

        QObject::connect(cancelButton, &QPushButton::clicked, [&]() {
            choice = SaveChoice::Cancel;
            dialog.reject();
        });

        // Execute dialog
        dialog.exec();

        Result result{};
        if (choice == SaveChoice::Save) {
            // Only return the checked models
            QList<IFileModel*> chosenModels;
            for (int i = 0; i < checkBoxes.size() && i < modifiedModels.size();
                 ++i) {
                if (checkBoxes[i]->isChecked()) {
                    chosenModels << modifiedModels[i];
                }
            }
            result = { SaveChoice::Save, chosenModels };
        } else if (choice == SaveChoice::Discard) {
            result = { SaveChoice::Discard, {} };
        } else {
            result = { SaveChoice::Cancel, {} };
        }

        return result;
    }

} // namespace SavePrompt

} // namespace Fernanda
