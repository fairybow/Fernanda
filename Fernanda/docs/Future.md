# Future

What Fernanda doesn't do yet, might do, and won't do. For what it does today, see [Features.md](Features.md). Bugs and code chores are in [TODO.md](TODO.md).

Nothing here is a promise or a schedule.

- **Planned**: intended, not yet built
- **Maybe**: worth recording, and may never be built. Each entry says what would justify it
- **Not planned**: decided against, with the reason, so the question isn't reopened without new information
- **Open questions**: undecided

## Planned

### Go to File

- Fuzzy matching

### Command palette and hotkeys

- A command palette over the actions each window already registers. Every action has a stable id named for the thing it acts on, so the palette is a list and a filter with no changes to the actions themselves
- Rebindable hotkeys, saved per vault in `.fernanda/hotkeys.json`. An override has to carry a list of key sequences, since standard shortcuts expand to more than one (Redo is Ctrl+Y and Ctrl+Shift+Z)
- One header holding every default shortcut, with per-platform variation where a binding differs. The editor's own handling of the undo and redo keys, and its context menu's labels, would read from it too

### Files and file trees

- Delete key in the file tree
- Selecting and dragging several entries at once
- A command or menu route to move a file ("Move file to..."), so dragging isn't the only way
- Delete or rename the current file from its tab
- A "Don't ask again" option on the delete confirmation
- Show the operating system's own reason when the trash refuses an entry
- Name checks beyond forbidden characters: names differing only by case, Unicode normalization, and length limits

### Tabs, panes, and windows

- Drop a tab at the position it is dropped, not at the end of the bar
- A tab's close button shown only on the current tab and the one under the pointer, as in Obsidian
- A toggle to show or hide the sidebar, remembered per vault
- Remember whether the menu bar is shown
- Grey out Undo and Redo when there is nothing to undo or redo
- Shortcuts for splitting a pane
- On macOS, replace the menu bar toggle: the system owns the menu bar there, and Cmd+M is Minimize

### Appearance

- Themes, for the window and for the editor, saved in `.fernanda/appearance.json`
- A reset button beside the font size slider
- Font previews in the font list
- Separate interface and monospace font settings
- Hearth's color bar

### Editor details

- Emphasize the current line's number; click a line number to select its line
- Place the cursor from a click in the margin
- Scroll while a selection handle is held past the top or bottom edge
- A handle for a selection end that is scrolled out of view

### PDFs and images

- SVG and AVIF images
- PDF zoom that holds the point under the pointer in place
- A password prompt for protected PDFs

### Saving and safety

- When a save fails and can't be retried (a drive gone for good), a window showing each unsaved file's text to copy out by hand. It needs no disk, so it works when every write fails
- A status indicator that shows a failed save when it happens, not only when the window is closed

### Settings

- Pick up edits made to the settings files or the vault's dictionary, by hand or by a sync, while the vault is open
- A trash destination setting, and with it permanent delete

### Opening from outside Fernanda

- Open a file or a vault passed on the command line, in the most recently used window. This is what makes "Open with Fernanda" and dragging a file onto the app work. Wanted after beta

### Git

Committing a vault to a repository, for backup and history.

- The repository is found by walking up from the vault's folder, as Git does. One repository may hold several vaults, and a vault may have none
- Status and commit would therefore belong to the repository, not the vault. Otherwise a commit from one vault would silently include another's changes

## Maybe

- **Recovery copies of files that vanish.** When a file is deleted outside Fernanda while open, its tab closes and up to three seconds of typing can go with it. Before closing, Fernanda could write the text to a recovery folder off the vault's drive and say so once. Periodic snapshots kept outside the vault (Obsidian's File Recovery) are the larger version. Worth building if a real loss is reported
- **Refusing an outside change.** Outside changes are always adopted, undoably. A file could instead be held against them: during a merge, or when explicitly locked
- **Ignoring a format-only outside change.** When another program changes only a file's line endings or byte order mark, the reload adds an undo step that visibly changes nothing and moves every cursor. The reload could be skipped. Leaning toward leaving it alone
- **A trash folder inside the vault,** used when the system trash refuses. Its one real use is a vault on a network share, which has no system trash and so can't delete in-app at all
- **Watching a whole vault through one handle.** Would end the Windows limit where other programs can't rename a folder Fernanda has listed. It needs a separate implementation per platform. Worth building if that limit becomes a real complaint
- **Checking open files when Fernanda regains focus.** On some platforms a file whose folder was renamed outside Fernanda keeps its tab until typed into. A check on return would close it sooner
- **Dragging a file out to the desktop** as a shortcut that opens it in its vault
- **Detecting and converting older text encodings** (Windows-1252 and others), in place of the warning
- **Patterns in Find** (regular expressions). Needs a way to show a pattern that doesn't parse, and a syntax for using what was matched in the replacement. Worth building if plain text with Whole word proves too blunt
- **A default folder for new files,** like Obsidian's "Default location for new notes": the current file's folder, or a chosen one. New files from Ctrl+N and from Go to File go to the vault's root. Worth building if typing the folder in Go to File proves tedious
- **Several spelling languages at once.** A view checks against one dictionary. Several would mean asking each one about every word, merging their suggestions, and deciding which one Add to dictionary answers to. Worth building if writing in two languages in one vault proves common
- **Managing the dictionary in Fernanda.** Removing an added word means editing `.fernanda/dictionary.txt` by hand, as in Obsidian. A list in Settings would do it in place. Worth building if hand editing proves awkward
- **Searching the whole vault.** A separate feature from Find, with its own list of results. Obsidian's is a plugin of its own
- **An overwrite toggle on the Insert key,** per editor, with a status bar indicator
- **Drop zones that move.** While a tab is dragged over a pane's edge, the pane would shrink aside to show where the new pane will land
- **Highlights and bookmarks** kept beside a file, not in it
- **Grouping or nesting files** under one another in the tree
- **A corkboard**
- **Converting files from the command line** (text to PDF, for instance)
- **Style sheet snippets,** like Obsidian's CSS snippets: a vault's own Qt style sheet laid over the look. A style sheet takes over drawing from Fernanda's style for whatever it touches, so the two would be hard to keep in step. Worth considering once themes exist
- **Installing updates automatically.** Fernanda says a newer version is out and links to it. Every update is a whole new installer (there is no smaller part to swap, as Obsidian swaps its app files), so installing for the user means running the installer silently. On Windows that needs a per-user install, since the current all-users installer asks for administrator rights even when silent, and with it the C++ runtime shipped beside the app in place of its installer, and a one-time move off Program Files that would leave an old copy behind. Releases would also need a way to say that a version must be installed by hand. Worth building if downloading the installer proves a real burden
- **A setting for receiving betas,** like Obsidian's early access. Today `STABLE_BUILDS_SEE_PRERELEASES` decides, for every user, whether a stable build is offered prereleases. Worth building once there are stable releases
- **Pairing Markdown syntax,** like Obsidian's "Auto pair Markdown syntax": `*`, `_`, `==`, and the like, paired as brackets are and wrapped around a selection. Most likely for Markdown files only, or Markdown and Fountain, and not plain text, where an asterisk is just an asterisk
- **Smart quotes:** straight quotes typed as curly ones, opening or closing by what's before them
- **A confirmation before a folder becomes a vault.** Opening any folder as a vault silently adds `.fernanda/` to it; Obsidian asks first

## Not planned

- **Links, backlinks, and tags.** Fernanda is for plain text. Without links there is nothing to index and nothing to rewrite when a file is renamed
- **Unsaved documents and save prompts.** Every file is on disk and always saved. An untitled, in-memory document would bring back the prompts, the modified markers, and the recovery machinery this design removes
- **A Move Vault command.** Built once, for the Common Vault, then removed. On Windows other programs (the folder picker, the shell, the indexer, antivirus) hold handles on the folders being moved, and the old folders can't be deleted while they do. Fernanda can't release handles it doesn't own. Moving the folder from the file tree of a vault that contains it, which the vault follows, or in a file manager and opening it again, does the same job. Any vault can now be set as the Common Vault, which was what the command was for
- **Deleting a vault's folder from Manage Vaults.** It is someone's writing. "Remove from list" forgets the vault and leaves the folder alone
- **Showing unsupported file types.** Obsidian has a setting for this. Fernanda hides them: a file it can't open only adds noise to the tree
- **Readable line length.** The left/right margin setting does this job and leaves the width to the writer
- **An overwrite mode setting.** Overwrite is a typing mode, not a preference. Saved as a setting it would silently eat text
- **A conflict prompt when a file changes outside Fernanda.** Built once, then removed. With saves this frequent, an open file almost never differs from disk when an outside change arrives, and the reload can be undone
- **Go to File (Ctrl+O) in pop-out windows.** A pop-out's new tab page already offers it. A shortcut would need a copy of the action for every pop-out, which wasn't worth it
- **Renaming in place in the file tree** (F2). Rename uses a dialog, which checks the name as it is typed
- **File associations.** Fernanda doesn't register itself for any file type
- **Right-to-left layouts**
- **Telling files apart by following symlinks.** A file is named by the path it was reached by, so a folder linked into a vault stays part of that vault. Following links would name it by its target, outside the vault, and break it there. A file reachable two ways through a link is two documents; Obsidian asks for link targets to be kept apart in the same way. Only a vault's own folder is followed through a link, so one folder opened two ways is one vault
- **Switching language without a restart.** Qt can swap translations while running, but every widget would have to set its text again when told, and Fernanda sets its text once, as each widget is built. Every class with text would need a second path for it, and a string missed in one would leave a window half in each language. Relaunch, beside the Language setting, does the job as Obsidian's does

## Open questions

- **The word "vault".** The per-project container might become a Project or a Notebook, and the shared one simply the Vault
- **Autosave timing.** One second after typing stops and three seconds at most are working defaults, not settled numbers
- **Git: bundle a library or call the installed `git`?** Calling it adds no dependency but requires Git on the machine and makes errors harder to read
- **Sort order with hidden extensions.** Files sort by full name, so `a b.md` comes before `a.md` and the tree reads "a b", then "a". Whether to sort by the name shown is undecided
- **Markdown rendering.** Not ruled out, and probably not the way Obsidian does it. What form it would take is undecided: styling in place with the markup left visible, a separate read-only preview, or something else. Whatever it is, the file stays plain text and is written back unchanged
- **Folder links in the file tree.** The tree shows and expands symlinked folders; Go to File skips them. One of the two should change
