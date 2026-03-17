/*
 * Fernanda is a plain text editor for fiction writing
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

#include <QComboBox>
#include <QGroupBox>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <Coco/Path.h>
#include <Coco/Utility.h>

#include "core/Debug.h"
#include "core/Tr.h"
#include "settings/Ini.h"
#include "settings/SettingsPanel.h"
#include "ui/ControlField.h"

namespace Fernanda {

// Theme selection widget with editor and window theme dropdowns.
// Emits signals on every user interaction for live update (write should be
// debounced, but that isn't ThemeSelector's concern)
//
// TODO: Double-check this!
class ThemesPanel : public SettingsPanel
{
    Q_OBJECT

public:
    struct Entry
    {
        QString name{};
        Coco::Path path{};
    };

    explicit ThemesPanel(
        const QVariantMap& values,
        const QList<Entry>& windowThemes,
        const QList<Entry>& editorThemes,
        QWidget* parent = nullptr)
        : SettingsPanel(Tr::themesPanelTitle(), parent)
    {
        setup_(values, windowThemes, editorThemes);
    }

    virtual ~ThemesPanel() override { TRACER; }

private:
    ControlField<QComboBox>* windowTheme_ =
        new ControlField<QComboBox>(FieldKind::Label, this);
    ControlField<QComboBox>* editorTheme_ =
        new ControlField<QComboBox>(FieldKind::Label, this);

    void setup_(
        const QVariantMap& values,
        const QList<Entry>& windowThemes,
        const QList<Entry>& editorThemes)
    {
        windowTheme_->setText(Tr::themesPanelWindowTheme());
        editorTheme_->setText(Tr::themesPanelEditorTheme());

        auto window_theme_box = windowTheme_->control();
        auto editor_theme_box = editorTheme_->control();

        /// TODO STYLE: Temporarily disable user window themes
        windowTheme_->setEnabled(false);

        // Populate window themes
        for (auto& entry : windowThemes) {
            window_theme_box->addItem(entry.name, qVar(entry.path));
        }

        // Populate editor themes
        for (auto& entry : editorThemes) {
            editor_theme_box->addItem(entry.name, qVar(entry.path));
        }

        // Set current selections
        selectByPath_(
            window_theme_box,
            values[Ini::Keys::WINDOW_THEME].value<Coco::Path>());
        selectByPath_(
            editor_theme_box,
            values[Ini::Keys::EDITOR_THEME].value<Coco::Path>());

        // Layout
        auto layout = groupBox()->layout();
        layout->addWidget(windowTheme_);
        layout->addWidget(editorTheme_);

        // Connect
        connectComboBox(window_theme_box, Ini::Keys::WINDOW_THEME);
        connectComboBox(editor_theme_box, Ini::Keys::EDITOR_THEME);
    }

    void selectByPath_(QComboBox* box, const Coco::Path& path)
    {
        for (auto i = 0; i < box->count(); ++i) {
            if (box->itemData(i).value<Coco::Path>() == path) {
                box->setCurrentIndex(i);
                return;
            }
        }

        // TODO: Path not found - leave at default (index 0) or could set to -1
    }
};

} // namespace Fernanda
