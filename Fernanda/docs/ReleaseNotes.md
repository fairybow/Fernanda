# v0.99.0-beta.1

**A plain text editor for drafting long-form fiction.** Work on single files like a notepad, or organize whole projects in Notebooks (`.fnx`).

This is a soft release. For a full feature list, see [Features.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/Features.md).

> [!WARNING]
> You should not trust your writing with any version of this software less than 1.0.0! Regardless, always make regular back-ups of your work.

---

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

---

## Installation

> [!NOTE]
> This build is unsigned, so Windows Defender SmartScreen will likely show a warning before allowing the install. You can click **More info -> Run anyway** to proceed.

---

## Uninstalling

Run the uninstaller (`unins000.exe`) or remove via **Add or Remove Programs** as usual.

Fernanda does not automatically delete its data folders on uninstall. After running the uninstaller, you may want to remove these manually:

| Folder | Location | Contents |
|--------|----------|----------|
| User Data | `~/.fernanda/` | Settings, temp files, libraries |
| Default Docs | `~/Documents/Fernanda/` | Default location for file dialogs |

> [!NOTE]
> Fernanda won't delete the `~/Documents/Fernanda/` folder, since your writing may be stored there. If you're sure you don't need it, you can remove it yourself.

---

## Known Issues

- Tree View root directory is locked in-place for now (Notepad)
- Window themes not yet implemented
- Notebook settings won't persist unless the Notebook itself is saved
- System shutdown handling is implemented but untested

---

## Platform

Windows (x64) only for now. Mac and Linux support is planned.

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

:heart:
