/*
 * Suzuri — A plain-text editor for creative writing
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

#include <QAbstractButton>
#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QShowEvent>
#include <QString>
#include <QUrl>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Vault.h"
#include "core/VaultConfig.h"
#include "core/spell/SpellCheckers.h"
#include "ui/settings/SettingRow.h"
#include "ui/settings/SettingsPage.h"
#include "ui/widgets/DisplaySlider.h"
#include "ui/widgets/ToggleSwitch.h"

namespace Suzuri::Ui {

using namespace Qt::StringLiterals;

// The settings dialog's Editor page, Obsidian's Editor tab: how the text
// editor behaves and what it shows besides the text. The column, and how a
// page talks to its Vault, are SettingsPage's.
//
// The first two sections are Obsidian's. Display is what the editor shows: line
// numbers sit in it there too ("Show line numbers"), and the left/right margin
// stands where Obsidian has "Readable line length". Behavior is how it
// responds: the tab width is Obsidian's "Indent visual width", which it keeps
// under Behavior as well, and auto-pairing is its "Auto pair brackets". The
// rest — wrapping, the current-line highlight, selection handles, center on
// scroll, double-click whitespace, the typing helpers' switch — are Suzuri's
// own, which Obsidian doesn't have.
//
// The typing helpers' rows hang on their switch: disabled while it's off,
// each keeping its own value.
//
// Spelling is a section of its own; Obsidian's Editor tab has a "Spellcheck"
// switch too. Its language list offers the dictionaries in the dictionaries
// folder (SpellCheckers), read again each time the page is shown, so one put
// there while the dialog was closed is offered when it next opens. Obsidian
// can check several languages at once; a vault here checks one.
class SettingsEditorPage : public SettingsPage
{
    Q_OBJECT

public:
    // spellCheckers is App's, borrowed for its list of languages
    SettingsEditorPage(
        Vault* vault,
        SpellCheckers* spellCheckers,
        QWidget* parentSettingsDialog)
        : SettingsPage(vault, parentSettingsDialog)
        , spellCheckers_(spellCheckers)
    {
        setup_();
    }

    ~SettingsEditorPage() override { TRACER; }

protected:
    void showEvent(QShowEvent* event) override
    {
        SettingsPage::showEvent(event);
        fillLanguages_();
    }

private:
    SpellCheckers* spellCheckers_ = nullptr;
    QComboBox* language_ = nullptr;

    void setup_()
    {
        setupDisplaySection_();
        setupBehaviorSection_();
        setupSpellingSection_();
    }

    void setupDisplaySection_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Display"));

        addSwitch(
            tr("Show line numbers"),
            tr("Show line numbers in the gutter."),
            config.lineNumbers(),
            &VaultConfig::setLineNumbers);

        addSwitch(
            tr("Wrap lines"),
            tr("Wrap long lines to the editor's width. Turn this off to "
               "scroll sideways instead."),
            config.wrapLines(),
            &VaultConfig::setWrapLines);

        // Applies live while dragging; the Vault writes once it settles
        auto* margin = new DisplaySlider(
            VaultConfig::MIN_LEFT_RIGHT_MARGIN,
            VaultConfig::MAX_LEFT_RIGHT_MARGIN,
            config.leftRightMargin(),
            content());
        connect(margin, &DisplaySlider::valueChanged, this, [this](int value) {
            vault()->setConfig(&VaultConfig::setLeftRightMargin, value);
        });
        addRow(
            tr("Left/right margin"),
            tr("Space kept clear on each side of the text, as a percentage "
               "of the editor's width."),
            margin);

        addSwitch(
            tr("Highlight current line"),
            tr("Show a band behind the line the text cursor is on."),
            config.lineHighlight(),
            &VaultConfig::setLineHighlight);

        addSwitch(
            tr("Selection handles"),
            tr("Show a handle under each end of a selection. Drag one to "
               "move that end."),
            config.selectionHandles(),
            &VaultConfig::setSelectionHandles);
    }

    void setupBehaviorSection_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Behavior"));

        auto* tab_width = new DisplaySlider(
            VaultConfig::MIN_TAB_WIDTH,
            VaultConfig::MAX_TAB_WIDTH,
            config.tabWidth(),
            content());
        connect(
            tab_width,
            &DisplaySlider::valueChanged,
            this,
            [this](int value) {
                vault()->setConfig(&VaultConfig::setTabWidth, value);
            });
        addRow(
            tr("Tab width"),
            tr("Number of spaces a tab character will render as."),
            tab_width);

        addSwitch(
            tr("Center on scroll"),
            tr("Keep the text cursor in the middle of the editor when it "
               "would scroll out of view, and let the text scroll past its "
               "end."),
            config.centerOnScroll(),
            &VaultConfig::setCenterOnScroll);

        addSwitch(
            tr("Double-click selects whitespace"),
            tr("Double-click a run of spaces or tabs to select all of it."),
            config.doubleClickWhitespace(),
            &VaultConfig::setDoubleClickWhitespace);

        setupTypingHelpers_();
    }

    void setupTypingHelpers_()
    {
        const auto& config = vault()->config();

        auto helpers = addSwitch(
            tr("Typing helpers"),
            tr("Help with brackets and quotes as you type. Each helper below "
               "can be turned off on its own."),
            config.typingHelpers(),
            &VaultConfig::setTypingHelpers);

        auto pairs = addSwitch(
            tr("Auto-pair brackets and quotes"),
            tr("Typing an opening bracket or quote adds its closing one. "
               "Typing the closing one steps past it, Backspace between an "
               "empty pair deletes both, and typing an opening one with text "
               "selected wraps the text."),
            config.autoPairs(),
            &VaultConfig::setAutoPairs);

        hangOn_(helpers.toggle, pairs.row);
    }

    // A row enabled only while a switch is on
    void hangOn_(ToggleSwitch* toggle, SettingRow* row)
    {
        row->setEnabled(toggle->isChecked());
        connect(toggle, &QAbstractButton::toggled, row, &QWidget::setEnabled);
    }

    void setupSpellingSection_()
    {
        const auto& config = vault()->config();

        addHeading(tr("Spelling"));

        auto check = addSwitch(
            tr("Check spelling"),
            tr("Underline words that aren't in the dictionary."),
            config.spellcheck(),
            &VaultConfig::setSpellcheck);

        // A drop-down of the dictionaries, and a button to open their folder.
        // activated is the user's choice only, so refilling the list sets
        // nothing
        auto* control = new QWidget(content());
        auto* control_layout = new QHBoxLayout(control);
        control_layout->setContentsMargins(0, 0, 0, 0);

        language_ = new QComboBox(control);
        language_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        control_layout->addWidget(language_);

        auto* open_folder = new QPushButton(tr("Open folder"), control);
        control_layout->addWidget(open_folder);

        connect(language_, &QComboBox::activated, this, [this](int index) {
            vault()->setConfig(
                &VaultConfig::setSpellcheckLanguage,
                language_->itemData(index).toString());
        });

        connect(open_folder, &QPushButton::clicked, this, [this] {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(spellCheckers_->folder().toQString()));
        });

        fillLanguages_();

        auto* row = addRow(
            tr("Language"),
            tr("The dictionary to check against. To add a language, place "
               "its dictionary's .aff and .dic files in the dictionaries "
               "folder."),
            control);

        hangOn_(check.toggle, row);
    }

    // List the installed dictionaries, each under its language's name, and
    // select the vault's. One in an encoding that can't be converted here is
    // marked as such, since choosing it checks nothing. A language the vault
    // names but the folder lacks is listed too, as not installed, so the
    // setting shows as it is rather than as some other language. Each item
    // carries the dictionary's file name
    void fillLanguages_()
    {
        if (!language_) {
            return;
        }

        auto current = vault()->config().spellcheckLanguage();
        auto languages = spellCheckers_->languages();

        language_->clear();

        for (const auto& language : languages) {
            auto name = languageName_(language);

            language_->addItem(
                spellCheckers_->isUsable(language)
                    ? name
                    : tr("%1 (unsupported encoding)").arg(name),
                language);
        }

        if (!languages.contains(current)) {
            language_->addItem(
                tr("%1 (not installed)").arg(languageName_(current)),
                current);
        }

        language_->setCurrentIndex(language_->findData(current));
    }

    // A dictionary's file name as a language: "en_US" is "English (United
    // States)" and "en" is "English". A name that is more than a language and
    // territory, such as "de_DE_frami", is shown as it is, since its locale
    // alone wouldn't tell it from "de_DE"
    [[nodiscard]] static QString languageName_(const QString& fileName)
    {
        QLocale locale(fileName);

        if (locale.language() == QLocale::C) {
            return fileName;
        }

        auto language = QLocale::languageToString(locale.language());

        if (locale.name() == fileName) {
            return u"%1 (%2)"_s.arg(
                language,
                QLocale::territoryToString(locale.territory()));
        }

        if (QLocale::languageToCode(locale.language()) == fileName) {
            return language;
        }

        return fileName;
    }
};

} // namespace Suzuri::Ui
