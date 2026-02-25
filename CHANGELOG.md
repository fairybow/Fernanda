# Changelog

[Skip to release content](#releases)

---

# Boilerplate

**A plain text editor for drafting long-form fiction.** Work on single files like a notepad, or organize whole projects in Notebooks (`.fnx`).

This is a soft release. For a full feature list, see [Features.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/Features.md). For past release details, see [CHANGELOG.md](https://github.com/fairybow/Fernanda/blob/main/CHANGELOG.md)

> [!WARNING]
> You should not trust your writing with any version of this software less than 1.0.0! Regardless, always make regular back-ups of your work.

## Installation

Download and run the installer below (on Windows x64 machines)!

> [!NOTE]
> This build is unsigned, so Windows Defender SmartScreen will likely show a warning before allowing the install. You can click **More info -> Run anyway** to proceed.

## Updating

Download the newest installer (below). Ensure Fernanda is closed, then run the installer and install to the same directory (default `C:/Program Files`), overwriting.

## Uninstalling

Run the uninstaller (`unins000.exe`) or remove via **Add or Remove Programs** as usual.

Fernanda does not automatically delete its data folders on uninstall. After running the uninstaller, you may want to remove these manually:

| Folder | Location | Contents |
|--------|----------|----------|
| User Data | `~/.fernanda/` | Settings, temp files, libraries |
| Default Docs | `~/Documents/Fernanda/` | Default location for file dialogs |

> [!TIP]
> Fernanda won't delete the `~/Documents/Fernanda/` folder, since your writing may be stored there. If you're sure you don't need it, you can remove it yourself.

## Platform

Windows (x64) only for now. Mac and Linux support is planned.

## Release note template

```markdown
<!-- Boilerplate (preamble) here -->

## What's New?

...

## Known Issues

...

<!-- Boilerplate (rest) here -->

## This Version's Dumbest Code Award :trophy:

...

:heart:
```

---

# Notes

- For release note links: use `blob/main` for evergreen links; use `blob/<tag>` for links to a specific release's snapshot!
- For diffing against previous release: `https://github.com/fairybow/Fernanda/compare/<tag>...main.diff`

---

<a id="releases"></a>

# 0.99.0-beta.2 (Testing / Soft Release)

## What's New?

- Settings pipeline overhaul: new `SettingsPanel` base class replaces per-panel boilerplate; `SettingsDialog` emits a single `settingChanged(key, value)` signal instead of around 20 individual ones; panels take `QVariantMap` instead of custom `InitialValues` structs; debouncers unified into a `QHash`
- "Prime Document" hack to allow multiple editors showing the same file to have separate layouts (which means, fundamentally, they need separate documents instead of sharing the same document)
- New documentation: `PrimeDocument.md`
- `MenuBuilder` decoupled from `Window` (takes `QWidget*` parent instead of `Window*`); gains `enabled()` builder method
- Fatal messages now always print regardless of logging toggle; `Debug::print` uses `QMessageLogger` instead of calling handler directly
- `KeyFilters` compound edit support via `MultiStepEditScope_` RAII guard and `multiStepEditBegan`/`multiStepEditEnded` signals (coordinates with prime document delta routing)
- New key filter: trailing punctuation gap closure on Enter
- Tab duplication (even for unsaved files)

## Known Issues

- Tree View root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested
- Large-document bulk operations (e.g., select-all-replace on 1M+ chars) may produce visible delay due to prime document delta routing (but this was only seen in debug)

## This Version's Dumbest Code Award :trophy:

A user toggles Line Numbers in the settings dialog. A `bool` needs to travel from a checkbox to the editor. So:

**Step 1**: The checkbox emits `toggled`. The panel catches it and emits `lineNumbersChanged`.

**Step 2**: The `SettingsDialog` catches `lineNumbersChanged` and emits `editorLineNumbersChanged`.

**Step 3**: The `SettingsService` catches `editorLineNumbersChanged`, persists the value, and emits a Bus event with a string key.

**Step 4**: The `ViewService` catches the Bus event and compares the key against 13 other keys, one `if` at a time (not `else if`, every branch, every time), until it finds the match and finally calls `setLineNumbers(true)`.

Four signal emissions, four connections, and a string-keyed dispatch resolved by brute-force linear scan. And this entire journey is copy-pasted roughly 20 times, once per setting. The `SettingsDialog` alone declares ~20 signals whose sole purpose is to hear a panel signal and re-emit it verbatim.

```cpp
// SettingsDialog.h: one of about 20 identical relays
connect(
    editorPanel_,
    &EditorPanel::lineNumbersChanged,
    this,
    [&](bool lineNumbers) {
        emit editorLineNumbersChanged(lineNumbers);
    });
```

```cpp
// ViewService.h: one of 13 if blocks
if (key == Ini::Keys::EDITOR_LINE_NUMBERS) {
    auto v = value.value<bool>();
    forEachTextFileView_(
        [&](TextFileView* view) { view->editor()->setLineNumbers(v); });
}
```

In total, the settings pipeline spans roughly 300 lines of boilerplate across four files to do what a unified `settingChanged(key, value)` signal and a hash map could do in about 40.

A solid runner-up is this (related) unnecessary cast out of `QVariant` (repeated at least one other time in another panel):

```cpp
connect(
    wrap_mode_box,
    &QComboBox::currentIndexChanged,
    this,
    [&](int index) {
        emit settingChanged(
            Ini::Keys::EDITOR_WRAP_MODE,
            wrapMode_->control()
                ->itemData(index)
                .value<QTextOption::WrapMode>());
    });
```

---

# 0.99.0-beta.1 (Testing / Soft Release)

## Highlights

### Two Workspaces

- **Notepad**: operates directly on the filesystem. Open, edit, and save any plain text file on disk. Drag a file onto Fernanda or double-click to open it.
- **Notebook**: archive-based workspace where an entire project lives inside a single, portable `.fnx` file (standard 7zip). Organize files and folders with a virtual directory tree, drag-and-drop reorganization, and a soft-delete trash system. Multiple Notebooks can be open at once, and because `.fnx` is just 7zip, your content is always recoverable outside of Fernanda.

### Editor

Line numbers, current line highlight, selection handles, undo/redo, word wrap modes, overwrite mode, center-on-scroll, and configurable tab stops.

### Key Filters (Typing Enhancements)

Auto-close punctuation pairs, smart backspace, skip-closer, barge-past-closers on double-space or Enter, trailing punctuation gap closing, and smart quote/apostrophe handling. All individually toggleable.

### Tabs & Windows

Tabbed interface with drag-and-drop between windows (or drag a tab out to create a new one). Modified-file indicators, multiple windows per workspace, and full closure management with save prompts at every level.

### Themes

JSON-based editor themes with QSS template rendering. User themes supported (drop a [`.fernanda_editor`](https://github.com/fairybow/Fernanda/blob/main/Fernanda/resources/themes/Pocket.fernanda_editor) file into `~/.fernanda/themes/` and it's available immediately with hot reload).

### Settings

Non-modal settings dialog with live preview. Tiered settings: Notebooks inherit from the base configuration and can override per-project (stored inside the archive).

### Also Included

Word counter (line/word/character counts, selection-aware, cursor position), color bar (animated gradient feedback on save/startup), update checker via GitHub Releases, and magic-byte file type detection.

### Known Issues

- Tree View root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested

## This Version's Dumbest Code Award :trophy:

When I first started working toward this project several years ago, I wrote code like this (this is real, I swear to God):

```cpp
if (checked)
{
    boolLineCount = true;
}
else
{
    boolLineCount = false;
};
```

With a semi-colon for good measure. It's amazing.

I've been at this for a while now, and though I think my code's improved a lot, I still find the occasional gems (sometimes something from last week). I wish I'd kept better track of older code from this last rewrite, as I'm sure it was full of questionable choices, but since I only thought about showcasing my worst code last minute, I had to settle for what was here now (or used to be).

Normally, I might save this section for dumb-yet-harmless code that's still in-place. Unfortunately, though, this one *had* to be removed.

So, without further ado, here's the erstwhile `wordCount_` from [`WordCounter`](https://github.com/fairybow/Fernanda/blob/main/Fernanda/src/WordCounter.h):

```cpp
int wordCount_(const QString& text) const
{
    static QRegularExpression regex(LEADING_WS_REGEX_);
    auto words = text.split(regex, Qt::SkipEmptyParts);
    return words.count();
}
```

On a document the size of Moby Dick, the old word counter allocated and destroyed, *I think*, 215,831 individual heap objects on every keystroke to produce a single number. The new one uses an int and a bool.
