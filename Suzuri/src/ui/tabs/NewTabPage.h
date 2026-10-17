/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>

namespace Suzuri::Ui {

// The empty page shown in a fresh tab. A plain QWidget, not an AbstractFileView
// — a new tab has no file, and a NoOpModel would be exactly the off-disk buffer
// Suzuri doesn't have. It emits intent; VaultWindow performs it and replaces
// this page in place with the editor.
//
// Styling is deliberately bare: three functional buttons, centered. The
// Obsidian-style list-item look (icons, hover) isn't built
class NewTabPage : public QWidget
{
    Q_OBJECT

public:
    NewTabPage()
        : QWidget(nullptr)
    {
        setup_();
    }

    ~NewTabPage() override { TRACER; }

signals:
    void createRequested();
    void openRequested();
    void closeRequested();

private:
    void setup_()
    {
        auto* create = new QPushButton(tr("Create a new file"), this);
        auto* open = new QPushButton(tr("Go to file"), this);
        auto* close = new QPushButton(tr("Close"), this);

        connect(
            create,
            &QPushButton::clicked,
            this,
            &NewTabPage::createRequested);
        connect(open, &QPushButton::clicked, this, &NewTabPage::openRequested);
        connect(
            close,
            &QPushButton::clicked,
            this,
            &NewTabPage::closeRequested);

        auto* column = new QVBoxLayout;
        column->setSpacing(8);
        column->addWidget(create);
        column->addWidget(open);
        column->addWidget(close);

        // Center the column both ways
        auto* center_row = new QHBoxLayout;
        center_row->addStretch(1);
        center_row->addLayout(column);
        center_row->addStretch(1);

        auto* layout = new QVBoxLayout(this);
        layout->addStretch(1);
        layout->addLayout(center_row);
        layout->addStretch(1);

        // Primary action doubles as the focus target, so a freshly opened
        // new-tab page is keyboard-ready
        setFocusProxy(create);
    }
};

} // namespace Suzuri::Ui
