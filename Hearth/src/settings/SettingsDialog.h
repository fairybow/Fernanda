/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
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
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "settings/ColorBarPanel.h"
#include "settings/EditorPanel.h"
#include "settings/FontPanel.h"
#include "settings/Ini.h"
#include "settings/KeyFiltersPanel.h"
#include "settings/ThemesPanel.h"
#include "settings/WordCounterPanel.h"

namespace Hearth {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(
        const QString& title,
        const Ini::Map& values,
        const QList<ThemesPanel::Entry>& windowThemes,
        const QList<ThemesPanel::Entry>& editorThemes,
        QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setup_(title, values, windowThemes, editorThemes);
    }

    virtual ~SettingsDialog() override { TRACER; }

signals:
    // NB: One-way! dialog -> outside. The dialog does not listen for external
    // setting changes, so values changed elsewhere (e.g., via menu toggles)
    // while the dialog is open will not sync back into its controls. Right now,
    // no setting is exposed in both places, but if that changes, panels  will
    // need a way to accept external value updates
    void settingChanged(const QString& key, const QVariant& value);

private:
    FontPanel* fontPanel_ = nullptr;
    ThemesPanel* themesPanel_ = nullptr;
    KeyFiltersPanel* keyFiltersPanel_ = nullptr;
    EditorPanel* editorPanel_ = nullptr;
    WordCounterPanel* wordCounterPanel_ = nullptr;
    ColorBarPanel* colorBarPanel_ = nullptr;

    void setup_(
        const QString& title,
        const Ini::Map& values,
        const QList<ThemesPanel::Entry>& windowThemes,
        const QList<ThemesPanel::Entry>& editorThemes)
    {
        setModal(false);
        setWindowTitle(title);

        fontPanel_ = new FontPanel(values, this);
        themesPanel_ =
            new ThemesPanel(values, windowThemes, editorThemes, this);
        keyFiltersPanel_ = new KeyFiltersPanel(values, this);
        editorPanel_ = new EditorPanel(values, this);
        wordCounterPanel_ = new WordCounterPanel(values, this);
        colorBarPanel_ = new ColorBarPanel(values, this);

        auto main_layout = new QHBoxLayout(this);

        auto col_0 = new QVBoxLayout;
        col_0->addWidget(fontPanel_);
        col_0->addWidget(themesPanel_);
        col_0->addWidget(wordCounterPanel_);
        col_0->addStretch();

        auto col_1 = new QVBoxLayout;
        col_1->addWidget(keyFiltersPanel_);
        col_1->addWidget(editorPanel_);
        col_1->addWidget(colorBarPanel_);
        col_1->addStretch();

        main_layout->addLayout(col_0);
        main_layout->addLayout(col_1);

        for (auto& panel :
             std::initializer_list<SettingsPanel*>{ fontPanel_,
                                                    themesPanel_,
                                                    keyFiltersPanel_,
                                                    editorPanel_,
                                                    wordCounterPanel_,
                                                    colorBarPanel_ }) {
            connect(
                panel,
                &SettingsPanel::settingChanged,
                this,
                [this](const QString& key, const QVariant& value) {
                    emit settingChanged(key, value);
                });
        }
    }
};

} // namespace Hearth
