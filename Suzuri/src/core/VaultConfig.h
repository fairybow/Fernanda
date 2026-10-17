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

#include <optional>

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtMinMax>

#include <Coco/Path.h>

#include "core/Io.h"
#include "core/JsonIo.h"
#include "core/VaultDotDir.h"

// Per-vault configuration — the committed files in <vaultRoot>/.suzuri/, the
// vault-local counterpart of AppConfig. Owned by Vault as a plain by-value
// member; the settings dialog edits it through Vault::setConfig, never
// directly, so every change is announced and saved.
//
// The same shape as AppConfig: a plain value type, not a QObject, whose getters
// fall back to defaults when a key is unset, so no caller has to know whether a
// value came from disk. And like AppConfig it houses its own defaults —
// AppConfig's are AppDirs paths resolved at runtime, these are the compile-time
// constants below. There is no separate defaults header and no runtime default
// layer.
//
// Only set keys are written, so "unset ⇒ absent ⇒ default" round-trips and a
// fresh vault's files stay minimal (Obsidian writes its config files the same
// way). A setter that leaves the resolved value unchanged stores nothing —
// choosing the default on an unset key doesn't write it — and reports false,
// so the Vault announces and saves only real changes.
//
// Two files, one type. appearance.json holds the text font (Obsidian keeps
// its fonts in its own appearance.json). settings.json — the file Obsidian
// calls app.json — holds everything that isn't how text looks: what the
// status bar shows, and how the editor behaves and what it shows besides the
// text. load reads both and save writes both.
//
// The status bar's settings are two groups, one per item (ui/WordCounter.h,
// ui/CursorPosition.h): a switch for the item itself, and one for each thing
// it can show. An item's parts keep their values while the item is off, so
// turning it back on restores what it showed.
//
// QtCore-only, like the Vault: it stores the font's parts, and the view that
// applies them builds the QFont (TextFileView::applyConfig)
namespace Suzuri {

using namespace Qt::StringLiterals;

class VaultConfig
{
public:
    // --- Defaults ------------------------------------------------------------

    // Literata is bundled (BundledFonts), so the default renders the same on
    // every machine. Sizes are in points, as QFont takes them, so they scale
    // with the display like the rest of the UI. The MIN/MAX bounds are the
    // size slider's range too, and clamp a hand-edited file on load
    static constexpr auto DEFAULT_TEXT_FONT_FAMILY = "Literata";
    static constexpr int DEFAULT_TEXT_FONT_SIZE = 12;
    static constexpr bool DEFAULT_TEXT_FONT_BOLD = false;
    static constexpr bool DEFAULT_TEXT_FONT_ITALIC = false;

    static constexpr int MIN_TEXT_FONT_SIZE = 8;
    static constexpr int MAX_TEXT_FONT_SIZE = 48;

    // The status bar. The word counter is on, showing what Obsidian's shows
    // (words and characters) plus selection counts; the cursor position is
    // off, as Obsidian has none, and shows both parts once switched on
    static constexpr bool DEFAULT_WORD_COUNTER_ENABLED = true;
    static constexpr bool DEFAULT_WORD_COUNTER_WORDS = true;
    static constexpr bool DEFAULT_WORD_COUNTER_CHARACTERS = true;
    static constexpr bool DEFAULT_WORD_COUNTER_LINES = false;
    static constexpr bool DEFAULT_WORD_COUNTER_SELECTION = true;

    static constexpr bool DEFAULT_CURSOR_POSITION_ENABLED = false;
    static constexpr bool DEFAULT_CURSOR_POSITION_LINE = true;
    static constexpr bool DEFAULT_CURSOR_POSITION_COLUMN = true;

    // The editor's line-number gutter (views/TextEditor.h). Off: Suzuri is for
    // prose, where a line is a paragraph
    static constexpr bool DEFAULT_LINE_NUMBERS = false;

    // The rest of the editor's settings (views/TextEditor.h). Wrapping on is
    // how QPlainTextEdit wraps by default: at a word boundary, or anywhere in a
    // word wider than the view. The left/right margin is a percentage of the
    // editor's width, kept clear on each side of the text; the tab width is in
    // spaces of the text font. The MIN/MAX bounds are the sliders' ranges and
    // clamp a hand-edited file on load, as the font size's do
    static constexpr bool DEFAULT_WRAP_LINES = true;
    static constexpr int DEFAULT_LEFT_RIGHT_MARGIN = 0;
    static constexpr int DEFAULT_TAB_WIDTH = 4;
    static constexpr bool DEFAULT_CENTER_ON_SCROLL = false;
    static constexpr bool DEFAULT_LINE_HIGHLIGHT = false;
    static constexpr bool DEFAULT_DOUBLE_CLICK_WHITESPACE = true;
    static constexpr bool DEFAULT_SELECTION_HANDLES = false;

    // The typing helpers (views/TypingHelpers.h): a switch for all of them,
    // and one for each. A helper keeps its own value while the switch is off,
    // as the status bar's parts do. Auto-pairing is on, as Obsidian's is
    static constexpr bool DEFAULT_TYPING_HELPERS = true;
    static constexpr bool DEFAULT_AUTO_PAIRS = true;

    // Spellcheck (core/spell/SpellChecker.h). The language is a dictionary's
    // name, the stem of its .aff and .dic files (core/spell/SpellCheckers.h)
    static constexpr bool DEFAULT_SPELLCHECK = true;
    static constexpr auto DEFAULT_SPELLCHECK_LANGUAGE = "en_US";

    static constexpr int MIN_LEFT_RIGHT_MARGIN = 0;
    static constexpr int MAX_LEFT_RIGHT_MARGIN = 40;

    static constexpr int MIN_TAB_WIDTH = 1;
    static constexpr int MAX_TAB_WIDTH = 16;

    VaultConfig() = default;

    // --- Getters -------------------------------------------------------------

    [[nodiscard]] QString textFontFamily() const
    {
        return textFontFamily_.isEmpty()
                   ? QString::fromLatin1(DEFAULT_TEXT_FONT_FAMILY)
                   : textFontFamily_;
    }

    [[nodiscard]] int textFontSize() const
    {
        return textFontSize_.value_or(DEFAULT_TEXT_FONT_SIZE);
    }

    [[nodiscard]] bool textFontBold() const
    {
        return textFontBold_.value_or(DEFAULT_TEXT_FONT_BOLD);
    }

    [[nodiscard]] bool textFontItalic() const
    {
        return textFontItalic_.value_or(DEFAULT_TEXT_FONT_ITALIC);
    }

    [[nodiscard]] bool wordCounterEnabled() const
    {
        return wordCounterEnabled_.value_or(DEFAULT_WORD_COUNTER_ENABLED);
    }

    [[nodiscard]] bool wordCounterWords() const
    {
        return wordCounterWords_.value_or(DEFAULT_WORD_COUNTER_WORDS);
    }

    [[nodiscard]] bool wordCounterCharacters() const
    {
        return wordCounterCharacters_.value_or(DEFAULT_WORD_COUNTER_CHARACTERS);
    }

    [[nodiscard]] bool wordCounterLines() const
    {
        return wordCounterLines_.value_or(DEFAULT_WORD_COUNTER_LINES);
    }

    // Whether a selection's counts are shown against the document's
    [[nodiscard]] bool wordCounterSelection() const
    {
        return wordCounterSelection_.value_or(DEFAULT_WORD_COUNTER_SELECTION);
    }

    [[nodiscard]] bool cursorPositionEnabled() const
    {
        return cursorPositionEnabled_.value_or(DEFAULT_CURSOR_POSITION_ENABLED);
    }

    [[nodiscard]] bool cursorPositionLine() const
    {
        return cursorPositionLine_.value_or(DEFAULT_CURSOR_POSITION_LINE);
    }

    [[nodiscard]] bool cursorPositionColumn() const
    {
        return cursorPositionColumn_.value_or(DEFAULT_CURSOR_POSITION_COLUMN);
    }

    [[nodiscard]] bool lineNumbers() const
    {
        return lineNumbers_.value_or(DEFAULT_LINE_NUMBERS);
    }

    [[nodiscard]] bool wrapLines() const
    {
        return wrapLines_.value_or(DEFAULT_WRAP_LINES);
    }

    // Percent of the editor's width, per side
    [[nodiscard]] int leftRightMargin() const
    {
        return leftRightMargin_.value_or(DEFAULT_LEFT_RIGHT_MARGIN);
    }

    // In spaces of the text font
    [[nodiscard]] int tabWidth() const
    {
        return tabWidth_.value_or(DEFAULT_TAB_WIDTH);
    }

    [[nodiscard]] bool centerOnScroll() const
    {
        return centerOnScroll_.value_or(DEFAULT_CENTER_ON_SCROLL);
    }

    [[nodiscard]] bool lineHighlight() const
    {
        return lineHighlight_.value_or(DEFAULT_LINE_HIGHLIGHT);
    }

    [[nodiscard]] bool doubleClickWhitespace() const
    {
        return doubleClickWhitespace_.value_or(DEFAULT_DOUBLE_CLICK_WHITESPACE);
    }

    [[nodiscard]] bool selectionHandles() const
    {
        return selectionHandles_.value_or(DEFAULT_SELECTION_HANDLES);
    }

    [[nodiscard]] bool typingHelpers() const
    {
        return typingHelpers_.value_or(DEFAULT_TYPING_HELPERS);
    }

    [[nodiscard]] bool autoPairs() const
    {
        return autoPairs_.value_or(DEFAULT_AUTO_PAIRS);
    }

    [[nodiscard]] bool spellcheck() const
    {
        return spellcheck_.value_or(DEFAULT_SPELLCHECK);
    }

    [[nodiscard]] QString spellcheckLanguage() const
    {
        return spellcheckLanguage_.isEmpty()
                   ? QString::fromLatin1(DEFAULT_SPELLCHECK_LANGUAGE)
                   : spellcheckLanguage_;
    }

    // --- Setters -------------------------------------------------------------

    // Each returns whether the resolved value changed (see class note). Memory
    // only; the Vault decides when to save

    [[nodiscard]] bool setTextFontFamily(const QString& family)
    {
        if (family.isEmpty() || family == textFontFamily()) {
            return false;
        }

        textFontFamily_ = family;
        return true;
    }

    [[nodiscard]] bool setTextFontSize(int size)
    {
        return setInt_(
            textFontSize_,
            textFontSize(),
            size,
            MIN_TEXT_FONT_SIZE,
            MAX_TEXT_FONT_SIZE);
    }

    [[nodiscard]] bool setTextFontBold(bool bold)
    {
        return setBool_(textFontBold_, textFontBold(), bold);
    }

    [[nodiscard]] bool setTextFontItalic(bool italic)
    {
        return setBool_(textFontItalic_, textFontItalic(), italic);
    }

    [[nodiscard]] bool setWordCounterEnabled(bool enabled)
    {
        return setBool_(wordCounterEnabled_, wordCounterEnabled(), enabled);
    }

    [[nodiscard]] bool setWordCounterWords(bool shown)
    {
        return setBool_(wordCounterWords_, wordCounterWords(), shown);
    }

    [[nodiscard]] bool setWordCounterCharacters(bool shown)
    {
        return setBool_(wordCounterCharacters_, wordCounterCharacters(), shown);
    }

    [[nodiscard]] bool setWordCounterLines(bool shown)
    {
        return setBool_(wordCounterLines_, wordCounterLines(), shown);
    }

    [[nodiscard]] bool setWordCounterSelection(bool shown)
    {
        return setBool_(wordCounterSelection_, wordCounterSelection(), shown);
    }

    [[nodiscard]] bool setCursorPositionEnabled(bool enabled)
    {
        return setBool_(
            cursorPositionEnabled_,
            cursorPositionEnabled(),
            enabled);
    }

    [[nodiscard]] bool setCursorPositionLine(bool shown)
    {
        return setBool_(cursorPositionLine_, cursorPositionLine(), shown);
    }

    [[nodiscard]] bool setCursorPositionColumn(bool shown)
    {
        return setBool_(cursorPositionColumn_, cursorPositionColumn(), shown);
    }

    [[nodiscard]] bool setLineNumbers(bool shown)
    {
        return setBool_(lineNumbers_, lineNumbers(), shown);
    }

    [[nodiscard]] bool setWrapLines(bool wrapped)
    {
        return setBool_(wrapLines_, wrapLines(), wrapped);
    }

    [[nodiscard]] bool setLeftRightMargin(int percent)
    {
        return setInt_(
            leftRightMargin_,
            leftRightMargin(),
            percent,
            MIN_LEFT_RIGHT_MARGIN,
            MAX_LEFT_RIGHT_MARGIN);
    }

    [[nodiscard]] bool setTabWidth(int spaces)
    {
        return setInt_(
            tabWidth_,
            tabWidth(),
            spaces,
            MIN_TAB_WIDTH,
            MAX_TAB_WIDTH);
    }

    [[nodiscard]] bool setCenterOnScroll(bool enabled)
    {
        return setBool_(centerOnScroll_, centerOnScroll(), enabled);
    }

    [[nodiscard]] bool setLineHighlight(bool shown)
    {
        return setBool_(lineHighlight_, lineHighlight(), shown);
    }

    [[nodiscard]] bool setDoubleClickWhitespace(bool enabled)
    {
        return setBool_(
            doubleClickWhitespace_,
            doubleClickWhitespace(),
            enabled);
    }

    [[nodiscard]] bool setSelectionHandles(bool shown)
    {
        return setBool_(selectionHandles_, selectionHandles(), shown);
    }

    [[nodiscard]] bool setTypingHelpers(bool enabled)
    {
        return setBool_(typingHelpers_, typingHelpers(), enabled);
    }

    [[nodiscard]] bool setAutoPairs(bool enabled)
    {
        return setBool_(autoPairs_, autoPairs(), enabled);
    }

    [[nodiscard]] bool setSpellcheck(bool enabled)
    {
        return setBool_(spellcheck_, spellcheck(), enabled);
    }

    [[nodiscard]] bool setSpellcheckLanguage(const QString& language)
    {
        if (language.isEmpty() || language == spellcheckLanguage()) {
            return false;
        }

        spellcheckLanguage_ = language;
        return true;
    }

    // --- IO ------------------------------------------------------------------

    // Read the vault's config files into memory. A missing file (a new vault,
    // or a deleted .suzuri/) or a missing key leaves that value unset, so its
    // getter returns the default. A value of the wrong JSON type is ignored the
    // same way; an out-of-range number is clamped. JsonIo has already warned
    // about a genuinely corrupt file
    void load(const Coco::Path& vaultRoot)
    {
        auto appearance = JsonIo::read(appearancePath_(vaultRoot));

        if (auto value = appearance.value(TEXT_FONT_FAMILY_KEY_);
            value.isString() && !value.toString().isEmpty()) {
            textFontFamily_ = value.toString();
        }

        readInt_(
            appearance,
            TEXT_FONT_SIZE_KEY_,
            textFontSize_,
            MIN_TEXT_FONT_SIZE,
            MAX_TEXT_FONT_SIZE);

        readBool_(appearance, TEXT_FONT_BOLD_KEY_, textFontBold_);
        readBool_(appearance, TEXT_FONT_ITALIC_KEY_, textFontItalic_);

        auto settings = JsonIo::read(settingsPath_(vaultRoot));

        readBool_(settings, WORD_COUNTER_ENABLED_KEY_, wordCounterEnabled_);
        readBool_(settings, WORD_COUNTER_WORDS_KEY_, wordCounterWords_);
        readBool_(
            settings,
            WORD_COUNTER_CHARACTERS_KEY_,
            wordCounterCharacters_);
        readBool_(settings, WORD_COUNTER_LINES_KEY_, wordCounterLines_);
        readBool_(settings, WORD_COUNTER_SELECTION_KEY_, wordCounterSelection_);
        readBool_(
            settings,
            CURSOR_POSITION_ENABLED_KEY_,
            cursorPositionEnabled_);
        readBool_(settings, CURSOR_POSITION_LINE_KEY_, cursorPositionLine_);
        readBool_(settings, CURSOR_POSITION_COLUMN_KEY_, cursorPositionColumn_);
        readBool_(settings, LINE_NUMBERS_KEY_, lineNumbers_);
        readBool_(settings, WRAP_LINES_KEY_, wrapLines_);
        readInt_(
            settings,
            LEFT_RIGHT_MARGIN_KEY_,
            leftRightMargin_,
            MIN_LEFT_RIGHT_MARGIN,
            MAX_LEFT_RIGHT_MARGIN);
        readInt_(
            settings,
            TAB_WIDTH_KEY_,
            tabWidth_,
            MIN_TAB_WIDTH,
            MAX_TAB_WIDTH);
        readBool_(settings, CENTER_ON_SCROLL_KEY_, centerOnScroll_);
        readBool_(settings, LINE_HIGHLIGHT_KEY_, lineHighlight_);
        readBool_(
            settings,
            DOUBLE_CLICK_WHITESPACE_KEY_,
            doubleClickWhitespace_);
        readBool_(settings, SELECTION_HANDLES_KEY_, selectionHandles_);
        readBool_(settings, TYPING_HELPERS_KEY_, typingHelpers_);
        readBool_(settings, AUTO_PAIRS_KEY_, autoPairs_);
        readBool_(settings, SPELLCHECK_KEY_, spellcheck_);

        if (auto value = settings.value(SPELLCHECK_LANGUAGE_KEY_);
            value.isString() && !value.toString().isEmpty()) {
            spellcheckLanguage_ = value.toString();
        }
    }

    // Serialize the set keys and write both files, each atomically. Both are
    // attempted even if the first fails, so one bad file doesn't cost the other
    // its changes. Never recreates a vault folder deleted outside Suzuri:
    // ensureVaultDotDir refuses a missing root, and the writes themselves make
    // no directories. Returns false on that refusal or if either write failed —
    // already logged
    bool save(const Coco::Path& vaultRoot) const
    {
        if (!ensureVaultDotDir(vaultRoot)) {
            return false;
        }

        QJsonObject appearance{};

        if (!textFontFamily_.isEmpty()) {
            appearance[TEXT_FONT_FAMILY_KEY_] = textFontFamily_;
        }

        writeInt_(appearance, TEXT_FONT_SIZE_KEY_, textFontSize_);

        writeBool_(appearance, TEXT_FONT_BOLD_KEY_, textFontBold_);
        writeBool_(appearance, TEXT_FONT_ITALIC_KEY_, textFontItalic_);

        QJsonObject settings{};

        writeBool_(settings, WORD_COUNTER_ENABLED_KEY_, wordCounterEnabled_);
        writeBool_(settings, WORD_COUNTER_WORDS_KEY_, wordCounterWords_);
        writeBool_(
            settings,
            WORD_COUNTER_CHARACTERS_KEY_,
            wordCounterCharacters_);
        writeBool_(settings, WORD_COUNTER_LINES_KEY_, wordCounterLines_);
        writeBool_(
            settings,
            WORD_COUNTER_SELECTION_KEY_,
            wordCounterSelection_);
        writeBool_(
            settings,
            CURSOR_POSITION_ENABLED_KEY_,
            cursorPositionEnabled_);
        writeBool_(settings, CURSOR_POSITION_LINE_KEY_, cursorPositionLine_);
        writeBool_(
            settings,
            CURSOR_POSITION_COLUMN_KEY_,
            cursorPositionColumn_);
        writeBool_(settings, LINE_NUMBERS_KEY_, lineNumbers_);
        writeBool_(settings, WRAP_LINES_KEY_, wrapLines_);
        writeInt_(settings, LEFT_RIGHT_MARGIN_KEY_, leftRightMargin_);
        writeInt_(settings, TAB_WIDTH_KEY_, tabWidth_);
        writeBool_(settings, CENTER_ON_SCROLL_KEY_, centerOnScroll_);
        writeBool_(settings, LINE_HIGHLIGHT_KEY_, lineHighlight_);
        writeBool_(
            settings,
            DOUBLE_CLICK_WHITESPACE_KEY_,
            doubleClickWhitespace_);
        writeBool_(settings, SELECTION_HANDLES_KEY_, selectionHandles_);
        writeBool_(settings, TYPING_HELPERS_KEY_, typingHelpers_);
        writeBool_(settings, AUTO_PAIRS_KEY_, autoPairs_);
        writeBool_(settings, SPELLCHECK_KEY_, spellcheck_);

        if (!spellcheckLanguage_.isEmpty()) {
            settings[SPELLCHECK_LANGUAGE_KEY_] = spellcheckLanguage_;
        }

        auto appearance_saved = JsonIo::write(
            appearance,
            appearancePath_(vaultRoot),
            Io::CreateDirs::No);

        auto settings_saved = JsonIo::write(
            settings,
            settingsPath_(vaultRoot),
            Io::CreateDirs::No);

        return appearance_saved && settings_saved;
    }

private:
    // Named once because load and save both use each one. Obsidian's own key
    // for the family is textFontFamily; its size is baseFontSize (in px), not
    // mirrored here since ours is points
    static inline const QString APPEARANCE_FILE_NAME_ = u"appearance.json"_s;
    static inline const QString TEXT_FONT_FAMILY_KEY_ = u"textFontFamily"_s;
    static inline const QString TEXT_FONT_SIZE_KEY_ = u"textFontSize"_s;
    static inline const QString TEXT_FONT_BOLD_KEY_ = u"textFontBold"_s;
    static inline const QString TEXT_FONT_ITALIC_KEY_ = u"textFontItalic"_s;

    // settings.json. The bare item name is the item's own switch; its parts
    // carry the name as a prefix
    static inline const QString SETTINGS_FILE_NAME_ = u"settings.json"_s;
    static inline const QString WORD_COUNTER_ENABLED_KEY_ = u"wordCounter"_s;
    static inline const QString WORD_COUNTER_WORDS_KEY_ = u"wordCounterWords"_s;
    static inline const QString WORD_COUNTER_CHARACTERS_KEY_ =
        u"wordCounterCharacters"_s;
    static inline const QString WORD_COUNTER_LINES_KEY_ = u"wordCounterLines"_s;
    static inline const QString WORD_COUNTER_SELECTION_KEY_ =
        u"wordCounterSelection"_s;
    static inline const QString CURSOR_POSITION_ENABLED_KEY_ =
        u"cursorPosition"_s;
    static inline const QString CURSOR_POSITION_LINE_KEY_ =
        u"cursorPositionLine"_s;
    static inline const QString CURSOR_POSITION_COLUMN_KEY_ =
        u"cursorPositionColumn"_s;
    static inline const QString LINE_NUMBERS_KEY_ = u"lineNumbers"_s;
    static inline const QString WRAP_LINES_KEY_ = u"wrapLines"_s;
    static inline const QString LEFT_RIGHT_MARGIN_KEY_ = u"leftRightMargin"_s;
    static inline const QString TAB_WIDTH_KEY_ = u"tabWidth"_s;
    static inline const QString CENTER_ON_SCROLL_KEY_ = u"centerOnScroll"_s;
    static inline const QString LINE_HIGHLIGHT_KEY_ = u"lineHighlight"_s;
    static inline const QString DOUBLE_CLICK_WHITESPACE_KEY_ =
        u"doubleClickWhitespace"_s;
    static inline const QString SELECTION_HANDLES_KEY_ = u"selectionHandles"_s;
    static inline const QString TYPING_HELPERS_KEY_ = u"typingHelpers"_s;
    static inline const QString AUTO_PAIRS_KEY_ = u"autoPairs"_s;
    static inline const QString SPELLCHECK_KEY_ = u"spellcheck"_s;
    static inline const QString SPELLCHECK_LANGUAGE_KEY_ =
        u"spellcheckLanguage"_s;

    // Empty / nullopt == unset; the getter substitutes the default
    QString textFontFamily_{};
    std::optional<int> textFontSize_{};
    std::optional<bool> textFontBold_{};
    std::optional<bool> textFontItalic_{};

    std::optional<bool> wordCounterEnabled_{};
    std::optional<bool> wordCounterWords_{};
    std::optional<bool> wordCounterCharacters_{};
    std::optional<bool> wordCounterLines_{};
    std::optional<bool> wordCounterSelection_{};

    std::optional<bool> cursorPositionEnabled_{};
    std::optional<bool> cursorPositionLine_{};
    std::optional<bool> cursorPositionColumn_{};

    std::optional<bool> lineNumbers_{};
    std::optional<bool> wrapLines_{};
    std::optional<int> leftRightMargin_{};
    std::optional<int> tabWidth_{};
    std::optional<bool> centerOnScroll_{};
    std::optional<bool> lineHighlight_{};
    std::optional<bool> doubleClickWhitespace_{};
    std::optional<bool> selectionHandles_{};
    std::optional<bool> typingHelpers_{};
    std::optional<bool> autoPairs_{};

    std::optional<bool> spellcheck_{};
    QString spellcheckLanguage_{};

    static Coco::Path appearancePath_(const Coco::Path& vaultRoot)
    {
        return vaultDotDir(vaultRoot) / APPEARANCE_FILE_NAME_;
    }

    static Coco::Path settingsPath_(const Coco::Path& vaultRoot)
    {
        return vaultDotDir(vaultRoot) / SETTINGS_FILE_NAME_;
    }

    // The three steps every on/off setting shares, so each one's getter,
    // setter, and key are all that's written out per setting.
    //
    // setBool_: store value unless it's what the getter already resolves to
    // (current), and report whether it changed — the setters' contract.
    // readBool_: take a key from a loaded file, leaving stored unset if the
    // key is missing or isn't a bool. writeBool_: put a set value into a
    // file being saved, and nothing for an unset one
    [[nodiscard]] static bool
    setBool_(std::optional<bool>& stored, bool current, bool value)
    {
        if (value == current) {
            return false;
        }

        stored = value;
        return true;
    }

    static void readBool_(
        const QJsonObject& file,
        const QString& key,
        std::optional<bool>& stored)
    {
        if (auto value = file.value(key); value.isBool()) {
            stored = value.toBool();
        }
    }

    static void writeBool_(
        QJsonObject& file,
        const QString& key,
        const std::optional<bool>& stored)
    {
        if (stored.has_value()) {
            file[key] = *stored;
        }
    }

    // The same three steps for a number kept within [minimum, maximum].
    //
    // setInt_ clamps before comparing, so an out-of-range request that lands
    // on the current value is no change. readInt_ clamps what it reads, so a
    // hand-edited file can't put a value outside its slider's range
    [[nodiscard]] static bool setInt_(
        std::optional<int>& stored,
        int current,
        int value,
        int minimum,
        int maximum)
    {
        auto clamped = qBound(minimum, value, maximum);
        if (clamped == current) {
            return false;
        }

        stored = clamped;
        return true;
    }

    static void readInt_(
        const QJsonObject& file,
        const QString& key,
        std::optional<int>& stored,
        int minimum,
        int maximum)
    {
        if (auto value = file.value(key); value.isDouble()) {
            stored = qBound(minimum, value.toInt(), maximum);
        }
    }

    static void writeInt_(
        QJsonObject& file,
        const QString& key,
        const std::optional<int>& stored)
    {
        if (stored.has_value()) {
            file[key] = *stored;
        }
    }
};

} // namespace Suzuri
