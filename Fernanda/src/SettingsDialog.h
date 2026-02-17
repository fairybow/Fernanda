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
#include <QObject>
#include <QString>
#include <QTextOption>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Path.h"

#include "ColorBar.h"
#include "ColorBarPanel.h"
#include "Debug.h"
#include "EditorPanel.h"
#include "FontSelector.h"
#include "KeyFiltersPanel.h"
#include "ThemeSelector.h"
#include "WordCounterPanel.h"

namespace Fernanda {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    struct InitialValues
    {
        QFont font;

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

        bool wordCounterActive;
        bool wordCounterLineCount;
        bool wordCounterWordCount;
        bool wordCounterCharCount;
        bool wordCounterSelection;
        bool wordCounterSelReplace;
        bool wordCounterLinePos;
        bool wordCounterColPos;

        bool colorBarActive;
        ColorBar::Position colorBarPosition;
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

    void wordCounterActiveChanged(bool active);
    void wordCounterLineCountChanged(bool lineCount);
    void wordCounterWordCountChanged(bool wordCount);
    void wordCounterCharCountChanged(bool charCount);
    void wordCounterSelectionChanged(bool selection);
    void wordCounterSelReplaceChanged(bool selReplace);
    void wordCounterLinePosChanged(bool linePos);
    void wordCounterColPosChanged(bool colPos);

    void colorBarActiveChanged(bool active);
    void colorBarPositionChanged(ColorBar::Position position);

private:
    FontSelector* fontSelector_ = nullptr;
    ThemeSelector* themeSelector_ = nullptr;
    KeyFiltersPanel* keyFiltersPanel_ = nullptr;
    EditorPanel* editorPanel_ = nullptr;
    WordCounterPanel* wordCounterPanel_ = nullptr;
    ColorBarPanel* colorBarPanel_ = nullptr;

    void setup_(const QString& title, const InitialValues& initialValues)
    {
        setModal(false);
        setWindowTitle(title);

        fontSelector_ = new FontSelector(initialValues.font, this);

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

        wordCounterPanel_ = new WordCounterPanel(
            WordCounterPanel::InitialValues{
                .active = initialValues.wordCounterActive,
                .lineCount = initialValues.wordCounterLineCount,
                .wordCount = initialValues.wordCounterWordCount,
                .charCount = initialValues.wordCounterCharCount,
                .selection = initialValues.wordCounterSelection,
                .selReplace = initialValues.wordCounterSelReplace,
                .linePos = initialValues.wordCounterLinePos,
                .colPos = initialValues.wordCounterColPos },
            this);

        colorBarPanel_ = new ColorBarPanel(
            ColorBarPanel::InitialValues{
                .active = initialValues.colorBarActive,
                .position = initialValues.colorBarPosition },
            this);

        auto main_layout = new QHBoxLayout(this);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);

        auto col_0 = new QVBoxLayout;
        col_0->setSpacing(0);
        col_0->addWidget(fontSelector_);
        col_0->addWidget(themeSelector_);
        col_0->addWidget(wordCounterPanel_);
        col_0->addStretch();

        auto col_1 = new QVBoxLayout;
        col_1->setSpacing(0);
        col_1->addWidget(keyFiltersPanel_);
        col_1->addWidget(editorPanel_);
        col_1->addWidget(colorBarPanel_);
        col_1->addStretch();

        main_layout->addLayout(col_0);
        main_layout->addLayout(col_1);

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

        connect(
            wordCounterPanel_,
            &WordCounterPanel::activeChanged,
            this,
            [&](bool active) { emit wordCounterActiveChanged(active); });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::lineCountChanged,
            this,
            [&](bool lineCount) {
                emit wordCounterLineCountChanged(lineCount);
            });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::wordCountChanged,
            this,
            [&](bool wordCount) {
                emit wordCounterWordCountChanged(wordCount);
            });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::charCountChanged,
            this,
            [&](bool charCount) {
                emit wordCounterCharCountChanged(charCount);
            });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::selectionChanged,
            this,
            [&](bool selection) {
                emit wordCounterSelectionChanged(selection);
            });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::selReplaceChanged,
            this,
            [&](bool selReplace) {
                emit wordCounterSelReplaceChanged(selReplace);
            });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::linePosChanged,
            this,
            [&](bool linePos) { emit wordCounterLinePosChanged(linePos); });

        connect(
            wordCounterPanel_,
            &WordCounterPanel::colPosChanged,
            this,
            [&](bool colPos) { emit wordCounterColPosChanged(colPos); });

        connect(
            colorBarPanel_,
            &ColorBarPanel::activeChanged,
            this,
            [&](bool active) { emit colorBarActiveChanged(active); });

        connect(
            colorBarPanel_,
            &ColorBarPanel::positionChanged,
            this,
            [&](ColorBar::Position position) {
                emit colorBarPositionChanged(position);
            });
    }
};

} // namespace Fernanda
