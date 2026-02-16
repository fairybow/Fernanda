/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QComboBox>
#include <QList>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Path.h"

#include "ControlField.h"
#include "Debug.h"
#include "Tr.h"

namespace Fernanda {

// Theme selection widget with editor and window theme dropdowns.
// Emits signals on every user interaction for live update (write should be
// debounced, but that isn't ThemeSelector's concern)
//
// TODO: Double-check this!
class ThemeSelector : public QWidget
{
    Q_OBJECT

public:
    struct Entry
    {
        QString name{};
        Coco::Path path{};
    };

    struct InitialValues
    {
        QList<Entry> windowThemes{};
        Coco::Path currentWindowTheme{};
        QList<Entry> editorThemes{};
        Coco::Path currentEditorTheme{};
    };

    explicit ThemeSelector(
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QWidget(parent)
        , currentWindowTheme_(initialValues.currentWindowTheme)
        , currentEditorTheme_(initialValues.currentEditorTheme)
    {
        setup_(initialValues);
    }

    virtual ~ThemeSelector() override { TRACER; }

    Coco::Path currentWindowTheme() const noexcept
    {
        return currentWindowTheme_;
    }

    Coco::Path currentEditorTheme() const noexcept
    {
        return currentEditorTheme_;
    }

signals:
    void windowThemeChanged(const Coco::Path& path);
    void editorThemeChanged(const Coco::Path& path);

private:
    Coco::Path currentWindowTheme_{};
    Coco::Path currentEditorTheme_{};

    ControlField<QComboBox*>* windowTheme_ = new ControlField<QComboBox*>(this);
    ControlField<QComboBox*>* editorTheme_ = new ControlField<QComboBox*>(this);

    void setup_(const InitialValues& initialValues)
    {
        windowTheme_->setText(Tr::windowTheme());
        editorTheme_->setText(Tr::editorTheme());
        auto window_theme_box = windowTheme_->control();
        auto editor_theme_box = editorTheme_->control();

        /// TODO STYLE: Temporarily disable user window themes
        windowTheme_->setEnabled(false); /// SEE IF THIS WORKS

        // Populate window themes
        for (const auto& entry : initialValues.windowThemes) {
            window_theme_box->addItem(
                entry.name,
                QVariant::fromValue(entry.path));
        }

        // Populate editor themes
        for (const auto& entry : initialValues.editorThemes) {
            editor_theme_box->addItem(
                entry.name,
                QVariant::fromValue(entry.path));
        }

        // Set current selections
        selectByPath_(window_theme_box, currentWindowTheme_);
        selectByPath_(editor_theme_box, currentEditorTheme_);

        // Layout
        auto main_layout = new QVBoxLayout(this);
        main_layout->addWidget(windowTheme_);
        main_layout->addWidget(editorTheme_);

        // Connect
        connect(
            window_theme_box,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                currentWindowTheme_ = windowTheme_->control()
                                          ->itemData(index)
                                          .value<Coco::Path>();
                emit windowThemeChanged(currentWindowTheme_);
            });

        connect(
            editor_theme_box,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                currentEditorTheme_ = editorTheme_->control()
                                          ->itemData(index)
                                          .value<Coco::Path>();
                emit editorThemeChanged(currentEditorTheme_);
            });
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
