#pragma once

#include <QDialog>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "Debouncer.h"
#include "EditorSwitchboard.h"
#include "FontSelector.h"
#include "Ini.h"
#include "Settings.h"
#include "Tr.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(Settings* settings, QWidget* parent = nullptr)
        : QDialog(parent), settings_(settings)
    {
        initialize_();
    }

    virtual ~SettingsDialog() override { COCO_TRACER; }

signals:
    void editorValuesChanged(const Ini::Editor::Values& values);
    void fontChanged(const QFont& font);

private:
    Settings* settings_;

    static constexpr auto DEBOUCE_MS_ = 500;

    // Editor
    EditorSwitchboard* editorSwitchboard_ = new EditorSwitchboard(Ini::Editor::load(settings_), this);
    Debouncer* editorWriteDebouncer_ = new Debouncer(DEBOUCE_MS_, this, &SettingsDialog::onEditorWriteDebouncerTimeout_);
    Ini::Editor::Values pendingEditorValues_{};
    bool hasPendingEditorValues_ = false;

    // Font
    FontSelector* fontSelector_ = new FontSelector(Ini::EditorFont::load(settings_), this);
    Debouncer* fontWriteDebouncer_ = new Debouncer(DEBOUCE_MS_, this, &SettingsDialog::onFontWriteDebouncerTimeout_);
    QFont pendingFont_{};
    bool hasPendingFont_ = false;

    void initialize_()
    {
        setModal(false);

        // Figure out good layout later lol
        auto layout = Coco::Layout::makeDense<QVBoxLayout*>(this);
        layout->addWidget(editorSwitchboard_);
        layout->addWidget(fontSelector_);

        // Connect
        connect(editorSwitchboard_, &EditorSwitchboard::currentChanged, this, &SettingsDialog::onEditorSwitchboardCurrentChanged_);
        connect(fontSelector_, &FontSelector::currentChanged, this, &SettingsDialog::onFontSelectorCurrentChanged_);
    }

    void onEditorWriteDebouncerTimeout_()
    {
        if (!settings_ || !hasPendingEditorValues_) return;
        Ini::Editor::save(pendingEditorValues_, settings_);
        hasPendingEditorValues_ = false;
    }

    void onEditorSwitchboardCurrentChanged_(const Ini::Editor::Values& values)
    {
        // Always emit for immediate visual feedback but debounce the write
        emit editorValuesChanged(values);
        pendingEditorValues_ = values;
        hasPendingEditorValues_ = true;
        editorWriteDebouncer_->start();
    }

    void onFontWriteDebouncerTimeout_()
    {
        if (!settings_ || !hasPendingFont_) return;
        Ini::EditorFont::save(pendingFont_, settings_);
        hasPendingFont_ = false;
    }

    void onFontSelectorCurrentChanged_(const QFont& font)
    {
        // Always emit for immediate visual feedback but debounce the write
        emit fontChanged(font);
        pendingFont_ = font;
        hasPendingFont_ = true;
        fontWriteDebouncer_->start();
    }
};
