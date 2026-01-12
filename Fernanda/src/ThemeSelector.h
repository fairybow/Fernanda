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
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Path.h"

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

    QComboBox* windowThemeBox_ = new QComboBox(this);
    QComboBox* editorThemeBox_ = new QComboBox(this);

    void setup_(const InitialValues& initialValues)
    {
        // Populate window themes
        for (const auto& entry : initialValues.windowThemes) {
            windowThemeBox_->addItem(
                entry.name,
                QVariant::fromValue(entry.path));
        }

        // Populate editor themes
        for (const auto& entry : initialValues.editorThemes) {
            editorThemeBox_->addItem(
                entry.name,
                QVariant::fromValue(entry.path));
        }

        // Set current selections
        selectByPath_(windowThemeBox_, currentWindowTheme_);
        selectByPath_(editorThemeBox_, currentEditorTheme_);

        // Layout
        auto main_layout = new QVBoxLayout(this);

        auto window_layout = new QHBoxLayout;
        window_layout->addWidget(new QLabel(Tr::windowTheme(), this));
        window_layout->addWidget(windowThemeBox_, 1);

        auto editor_layout = new QHBoxLayout;
        editor_layout->addWidget(new QLabel(Tr::editorTheme(), this));
        editor_layout->addWidget(editorThemeBox_, 1);

        main_layout->addLayout(window_layout);
        main_layout->addLayout(editor_layout);

        // Connect
        connect(
            windowThemeBox_,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                currentWindowTheme_ =
                    windowThemeBox_->itemData(index).value<Coco::Path>();
                emit windowThemeChanged(currentWindowTheme_);
            });

        connect(
            editorThemeBox_,
            &QComboBox::currentIndexChanged,
            this,
            [&](int index) {
                currentEditorTheme_ =
                    editorThemeBox_->itemData(index).value<Coco::Path>();
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
