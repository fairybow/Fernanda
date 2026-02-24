/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QWidget>

#include "Coco/Path.h"

#include "ColorBarPanel.h"
#include "Debug.h"
#include "EditorPanel.h"
#include "FontPanel.h"
#include "KeyFiltersPanel.h"
#include "ThemesPanel.h"
#include "WordCounterPanel.h"

namespace Fernanda {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(
        const QString& title,
        const QVariantMap& values,
        const QList<ThemesPanel::Entry>& windowThemes,
        const QList<ThemesPanel::Entry>& editorThemes,
        QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setup_(title, values, windowThemes, editorThemes);
    }

    virtual ~SettingsDialog() override { TRACER; }

signals:
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
        const QVariantMap& values,
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
                &SettingsDialog::settingChanged);
        }
    }
};

} // namespace Fernanda
