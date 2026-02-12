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
#include <QList>
#include <QObject>
#include <QString>
#include <QTextOption>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Path.h"

#include "Debug.h"
#include "EditorPanel.h"
#include "FontSelector.h"
#include "KeyFiltersPanel.h"
#include "ThemeSelector.h"

namespace Fernanda {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    struct InitialValues
    {
        QFont font;
        int fontSizeMin;
        int fontSizeMax;

        QList<ThemeSelector::Entry> windowThemes;
        Coco::Path currentWindowTheme;
        QList<ThemeSelector::Entry> editorThemes;
        Coco::Path currentEditorTheme;

        /// TODO KFS
        bool keyFiltersActive;
        bool keyFiltersAutoClose;
        bool keyFiltersBarging;

        /// TODO ES
        bool editorCenterOnScroll;
        bool editorOverwrite;
        int editorTabStopDistance;
        QTextOption::WrapMode editorWrapMode;
        bool editorDblClickWhitespace;
        bool editorLineNumbers;
        bool editorLineHighlight;
        bool editorSelectionHandles;

        //...
    };

    explicit SettingsDialog(
        const QString& title,
        const InitialValues& initialValues,
        QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setup_(title, initialValues);
    }

    virtual ~SettingsDialog() override { TRACER; }

signals:
    void fontChanged(const QFont& font);
    void windowThemeChanged(const Coco::Path& path);
    void editorThemeChanged(const Coco::Path& path);

    /// TODO KFS
    void keyFiltersActiveChanged(bool active);
    void keyFiltersAutoCloseChanged(bool autoClose);
    void keyFiltersBargingChanged(bool barging);

    /// TODO ES
    void editorCenterOnScrollChanged(bool centerOnScroll);
    void editorOverwriteChanged(bool overwrite);
    void editorTabStopDistanceChanged(int tabStopDistance);
    void editorWrapModeChanged(QTextOption::WrapMode wrapMode);
    void editorDblClickWhitespaceChanged(bool dblClickWhitespace);
    void editorLineNumbersChanged(bool lineNumbers);
    void editorLineHighlightChanged(bool lineHighlight);
    void editorSelectionHandlesChanged(bool selectionHandles);

private:
    FontSelector* fontSelector_ = nullptr;
    ThemeSelector* themeSelector_ = nullptr;
    KeyFiltersPanel* keyFiltersPanel_ = nullptr;
    EditorPanel* editorPanel_ = nullptr;

    void setup_(const QString& title, const InitialValues& initialValues)
    {
        setModal(false);
        setWindowTitle(title);

        fontSelector_ = new FontSelector(
            initialValues.font,
            initialValues.fontSizeMin,
            initialValues.fontSizeMax,
            this);

        themeSelector_ = new ThemeSelector(
            ThemeSelector::InitialValues{
                .windowThemes = initialValues.windowThemes,
                .currentWindowTheme = initialValues.currentWindowTheme,
                .editorThemes = initialValues.editorThemes,
                .currentEditorTheme = initialValues.currentEditorTheme },
            this);

        /// TODO KFS
        keyFiltersPanel_ = new KeyFiltersPanel(
            KeyFiltersPanel::InitialValues{
                .active = initialValues.keyFiltersActive,
                .autoClose = initialValues.keyFiltersAutoClose,
                .barging = initialValues.keyFiltersBarging },
            this);

        /// TODO ES
        editorPanel_ = new EditorPanel(
            EditorPanel::InitialValues{
                .centerOnScroll = initialValues.editorCenterOnScroll,
                .overwrite = initialValues.editorOverwrite,
                .tabStopDistance = initialValues.editorTabStopDistance,
                .wrapMode = initialValues.editorWrapMode,
                .doubleClickWhitespace = initialValues.editorDblClickWhitespace,
                .lineNumbers = initialValues.editorLineNumbers,
                .lineHighlight = initialValues.editorLineHighlight,
                .selectionHandles = initialValues.editorSelectionHandles },
            this);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(fontSelector_);
        layout->addWidget(themeSelector_);
        layout->addWidget(keyFiltersPanel_);
        layout->addWidget(editorPanel_);

        connect(
            fontSelector_,
            &FontSelector::currentChanged,
            this,
            [&](const QFont& font) { emit fontChanged(font); });

        connect(
            themeSelector_,
            &ThemeSelector::windowThemeChanged,
            this,
            [&](const Coco::Path& path) { emit windowThemeChanged(path); });

        connect(
            themeSelector_,
            &ThemeSelector::editorThemeChanged,
            this,
            [&](const Coco::Path& path) { emit editorThemeChanged(path); });

        /// TODO KFS
        connect(
            keyFiltersPanel_,
            &KeyFiltersPanel::activeChanged,
            this,
            [&](bool active) { emit keyFiltersActiveChanged(active); });

        /// TODO KFS
        connect(
            keyFiltersPanel_,
            &KeyFiltersPanel::autoCloseChanged,
            this,
            [&](bool autoClose) {
                emit keyFiltersAutoCloseChanged(autoClose);
            });

        /// TODO KFS
        connect(
            keyFiltersPanel_,
            &KeyFiltersPanel::bargingChanged,
            this,
            [&](bool barging) { emit keyFiltersBargingChanged(barging); });

        /// TODO ES
        connect(
            editorPanel_,
            &EditorPanel::centerOnScrollChanged,
            this,
            [&](bool centerOnScroll) {
                emit editorCenterOnScrollChanged(centerOnScroll);
            });

        /// TODO ES
        connect(
            editorPanel_,
            &EditorPanel::overwriteChanged,
            this,
            [&](bool overwrite) { emit editorOverwriteChanged(overwrite); });

        /// TODO ES
        connect(
            editorPanel_,
            &EditorPanel::tabStopDistanceChanged,
            this,
            [&](int tabStopDistance) {
                emit editorTabStopDistanceChanged(tabStopDistance);
            });

        /// TODO ES
        connect(
            editorPanel_,
            &EditorPanel::wrapModeChanged,
            this,
            [&](QTextOption::WrapMode wrapMode) {
                emit editorWrapModeChanged(wrapMode);
            });

        connect(
            editorPanel_,
            &EditorPanel::doubleClickWhitespaceChanged,
            this,
            [&](bool dblClickWhitespace) {
                emit editorDblClickWhitespaceChanged(dblClickWhitespace);
            });

        connect(
            editorPanel_,
            &EditorPanel::lineNumbersChanged,
            this,
            [&](bool lineNumbers) {
                emit editorLineNumbersChanged(lineNumbers);
            });

        connect(
            editorPanel_,
            &EditorPanel::lineHighlightChanged,
            this,
            [&](bool lineHighlight) {
                emit editorLineHighlightChanged(lineHighlight);
            });

        connect(
            editorPanel_,
            &EditorPanel::selectionHandlesChanged,
            this,
            [&](bool selectionHandles) {
                emit editorSelectionHandlesChanged(selectionHandles);
            });
    }
};

} // namespace Fernanda
