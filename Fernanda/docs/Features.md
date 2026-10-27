# Features

Current version: 0.11.0-beta.0

What Fernanda does today. For what is planned, undecided, or deliberately left out, see [Future.md](Future.md). For how it is built, see [Architecture.md](Architecture.md).

## Vaults

A vault is a folder on your computer. Fernanda adds one hidden folder to it, `.fernanda/`, and otherwise leaves it alone: your files stay ordinary files that any other program can open.

- A vault can live anywhere. Opening a folder as a vault is all it takes to make one
- Several vaults can be open at once, each in its own window
- Quitting and relaunching reopens every vault that was open, with its windows, tabs, and layout
- Vaults may be nested inside one another, and may sit inside a Git repository. A project vault may even contain the Common Vault
- A file inside two open vaults is one document in every window. Renaming, moving, or deleting it from either vault carries every window's tabs with it, and a folder expanded in one window stays expanded after another window renames or moves it
- A file moved out of a vault from another vault's window has its tabs closed in that vault's windows, and keeps them wherever it is still inside the window's vault. Typing not yet saved goes with the file
- Renaming or moving a vault's folder from the file tree of a vault that contains it keeps the vault open at its new place: its window, tabs, settings, dictionary, recent files, and entry in the vault list all follow. So does renaming or moving a folder that holds other vaults' folders
- Deleting a folder that holds another open vault closes that vault's window, with a message, as when its folder is deleted outside Fernanda
- One Fernanda process holds every open vault

### The Common Vault

One vault can be shared by every project: the Common Vault, for templates, notes, and reference material you want within reach whatever you are working on.

- On first launch, Fernanda makes one at `Documents/Fernanda/Common Vault`
- Any vault can be the Common Vault: right-click it in Manage Vaults and choose Set as Common Vault. Unset Common Vault leaves you with none
- Every vault window shows it in a collapsible drawer under the vault's own file tree. Hovering over the drawer's header shows the Common Vault's name. The Common Vault's own window has no drawer
- A Common Vault file open in two windows is one document: an edit in one appears in the other as you type
- Tabs holding a Common Vault file are marked with a box icon, in the windows of vaults that don't contain the Common Vault. In a vault that does, its files are that vault's own. The icon follows its file: a file moved between the Common Vault and a window's own vault gains or loses it in place
- Words in its dictionary are accepted in every vault
- Can be opened as a window of its own, from the vault switcher
- Setting another Common Vault, or none, takes effect at once in every window: drawers appear or go, and a tab on a file that no longer belongs to either of its window's vaults closes
- Renaming it in Manage Vaults, or renaming or moving its folder from a vault that contains it, keeps it the Common Vault at the new place. Its own window, if open, stays open
- If its folder is deleted or moved outside Fernanda, or deleted from a vault that contains it, there is no Common Vault until you set one

### Manage Vaults

The vault picker. It opens on first launch, whenever there is no vault to reopen, and from the vault switcher or the File menu.

- Create new vault (a name and a location)
- Open folder as vault
- Recent vaults, most recently used first, or by name, A to Z or Z to A. The button beside the list's title moves between the three orders, and the one chosen is kept. A vault whose folder is gone is shown as missing
- Right-click a vault to rename it, reveal it in the file explorer, set it as the Common Vault, or remove it from the list. Removing never touches the folder. The Common Vault's own entry offers Unset Common Vault in place of Set and Remove
- Renaming a vault renames its folder. A vault that is open, and the Common Vault, stay open under the new name
- A vault that is open can't be removed from the list until its window is closed. The Common Vault is removed by unsetting it, then removing it
- A new name is checked as you type, by the same rules as a new vault's
- **Licenses**, under the version, lists Fernanda and everything it is built on or bundles (Qt, Hunspell, the dictionary, the fonts, and the icons), each with its license text

### Vault switcher

At the bottom of the sidebar. Lists your other vaults, most recently used first, opens the Common Vault (when there is one), and opens Manage Vaults. Choosing a vault that is already open brings its window forward.

## Saving

There is no Save command and no unsaved state.

- Every file exists on disk from the moment it is created. There are no untitled, in-memory documents
- Changes are written about a second after you stop typing, and at least every three seconds while you keep typing
- Everything is also written when Fernanda loses focus, on quit, and on logout. A window that closes writes the files it shows
- If a file can't be saved, the window refuses to close and names the files, so nothing is lost by closing
- Text is written back exactly as it was read, apart from your edits: line endings (LF or CRLF), a byte order mark, no-break spaces, and other special characters are preserved. A new file uses LF

## Changes made outside Fernanda

Files are expected to be edited elsewhere too: in another editor, by Git, by a sync client.

- A file changed outside Fernanda reloads in place. The reload is one undo step, so Ctrl+Z brings back what you had
- A file deleted, moved, or renamed outside Fernanda has its tabs closed, in every window
- Files and folders created, renamed, or removed outside Fernanda appear in the file trees on their own
- If an open vault's folder is deleted or moved, Fernanda tells you and closes that vault's window when you return to it. It never recreates the folder. If it was the Common Vault's, there is no Common Vault until you set one
- Many files changing at once (a Git checkout, a sync) are handled as one pass

## Files

### Supported types

| Kind | Extensions |
|---|---|
| Text | `.txt`, `.md`, `.markdown`, `.fountain` |
| PDF | `.pdf` |
| Image | `.png`, `.jpg`, `.jpeg`, `.gif`, `.bmp`, `.tif`, `.tiff`, `.webp` |

Other files are hidden from the file trees and from Go to File, and can't be opened. So are files and folders whose names begin with a dot.

Text files are read as UTF-8. A text file that isn't valid UTF-8 (an old Windows-1252 manuscript, say) brings up a warning before anything changes, with a preview of the affected lines. Open Anyway converts it to UTF-8 on the spot, with the unreadable characters replaced by �; Cancel leaves the file untouched.

### File tree

- Folders first, then names in natural order (`Chapter 2` before `Chapter 10`)
- File names are shown without their extension. Every type but `.txt` shows its extension at the right edge of the row (`MD`, `PDF`, `FOUNTAIN`)
- Expanded folders are remembered per vault
- Double-click a file to open it
- Right-click a file: Open in new tab, Open to the right, Open in new window, Rename, Delete
- Right-click a folder: New file, New folder, Rename, Delete
- Right-click empty space: New file, New folder (at the vault root)
- New file and New folder open the folder they're made in, and select the new entry. A new file opens in the editor too
- Click anywhere off the rows to clear the selection
- Drag a file or folder onto a folder to move it there, or onto empty space to move it to the vault root. Folders expand when hovered and the tree scrolls while dragging
- Drag a file onto an editor pane to open it there
- Delete always asks first, and moves the entry to the system trash. If the trash refuses it (a network share, for instance), nothing is deleted
- Names are checked as you type for characters and names the operating system forbids

### Go to File

Ctrl+O, or "Go to file" on a new tab. A search field over every file in the vault and the Common Vault.

- Type any words, in any order. Each must appear somewhere in the file's path
- Matches in the file name rank above matches in the folder path
- With nothing typed, the files you opened most recently come first, newest first and labeled RECENT, followed by every other file. If the top one is the file you're in, the next is selected, so Ctrl+O then Enter goes back to the previous file
- Enter opens the file in the current tab. Ctrl+Enter opens it in a new tab
- Common Vault files are labeled COMMON. A file the window's own vault also contains is listed once, as the window's own
- Recent files are remembered per vault
- To create a file, type its name: Enter creates it when nothing matches, and Shift+Enter creates it even when something does. Hold Ctrl as well to open it in a new tab. A line under the field says which file Enter or Shift+Enter would create, or what is wrong with the name
- A new file's name counts from the vault's root, and `/` puts it in folders, which are made if they're missing (`drafts/Chapter 2`). A name without `.txt`, `.md`, `.markdown`, or `.fountain` gets `.txt`. If the file is already there, it opens instead
- New files go in the window's own vault, never the Common Vault (unless that is the window's vault)

## Tabs, panes, and windows

- Opening a file replaces what is in the current tab, unless that tab is pinned
- A new tab opens empty, offering to create a file or go to one
- The same file can be open in any number of tabs, panes, and windows. All of them stay in sync and share one undo history
- Right-click a tab to close, pin, or unpin it. A pinned tab keeps its file when another is opened and shows a pin in place of its close button
- Drag a tab along the bar to reorder it, onto another pane's bar to move it there, onto the edge of a pane to split that pane, or anywhere else to pop it out into its own window
- Panes split side by side and stacked, to any depth. A pane closes when its last tab does
- Pop-out windows have panes and tabs of their own. They belong to their vault's main window and close with it, or when their last tab closes
- Window positions and sizes, the pane layout, tabs, pins, and each tab's cursor, scroll, and zoom are saved per vault and restored on the next launch

## Editor

Plain text. Fernanda does not render Markdown or Fountain; those files are edited as the text they are.

- Undo and redo
- Cut, copy, paste, delete, and select all, from the keyboard, the Edit menu, or the right-click menu. In the Edit menu they act on whatever text has the keyboard focus, so they work in the find field too
- Copying takes the text exactly as it is, as plain text: no-break spaces and other special characters come along unchanged, and nothing of the editor's font goes with it
- Line numbers (off by default)
- Line wrapping (on by default)
- Left/right margin, as a percentage of the editor's width
- Tab width, in spaces
- Center on scroll: keeps the cursor mid-view and lets the text scroll past its end
- Current line highlight
- Double-click a run of spaces or tabs to select the whole run
- Selection handles: a draggable handle under each end of a selection, for adjusting it. They appear once the selection is made: when the mouse button is let go
- Typing helpers (on by default), each with its own switch and one switch for both:
  - **Auto-pair brackets and quotes.** Typing `(`, `[`, `{`, a quote, or a backtick adds its closer, when what follows is a space, the end of the line, or another closer; before a word it types just the opener. Straight quotes don't pair after a letter or digit, so `don't` and `5'` are left alone. Curly quotes and guillemets (`“”`, `‘’`, `«»`, `‹›`) pair with their own closers. Typing a closer that was added this way steps over it, Backspace between an empty pair deletes both, and typing an opener with text selected wraps the text
  - **Step past closing punctuation.** A second space typed just before a closing bracket or quote moves past it: `(word |)` becomes `(word) |`. Enter does the same when only closers are left on the line. A comma, period, question mark, exclamation mark, semicolon, or colon typed straight after goes before the space: `(word), `. That space is the one you'd type next, so a space typed there adds nothing, and Enter drops it
  - Each change undoes in one step
- Font family, size, bold, and italic for the text. Courier Prime, Literata (the default), mononoki, and OpenDyslexic are bundled

## Find and replace

Ctrl+F opens a find bar across the top of the text, and Ctrl+H opens it with a second row to replace with. The arrow at the left of the bar shows or hides that row. Each tab has its own bar: two panes on one file can search for different things.

- Matches are highlighted as you type, and the search goes to the first one at or after the cursor. The bar shows which match it is on and how many there are ("3 of 17")
- Enter or F3 goes to the next match, Shift+Enter or Shift+F3 to the previous. Both wrap around the ends of the file
- Opening the bar with text selected on one line searches for that text
- **Match case** and **Whole word** narrow the search. Whole word leaves longer words alone: "Al" is not found in "Although"
- The search is for the text as typed, with no wildcards. A space also matches a no-break space, and the other way round
- **Replace** changes the match the search is on and moves to the next. If the cursor is not on a match, the first press only moves to one
- **Replace all** changes every match at once and says how many. One undo takes it all back
- Highlights follow the text as it changes, and stay until the bar is closed
- Esc closes the bar and leaves the last match selected
- A search covers the one file. Nothing about it is saved

## Spelling

Misspelled words are underlined with a red wave as you write. Spelling is checked against a Hunspell dictionary, the format LibreOffice uses. US English is bundled.

- The word you are typing isn't marked until the cursor leaves it
- Right-click a marked word for up to five suggestions. Choosing one replaces the word, and one undo puts it back. A suggestion takes the word's apostrophe, straight or curly
- **Add to dictionary** adds the word to the vault's own dictionary. Words in the Common Vault's dictionary are accepted in every vault
- **Ignore** accepts the word in this vault until the vault is closed
- A possessive is added or ignored without its 's: choosing either on "Pangloss's" takes "Pangloss"
- A word added in lowercase is also accepted capitalized and in capitals. One added with capitals is accepted as written and in capitals only
- An added or ignored word is accepted as a possessive too, and as a part of a hyphenated word: with "Pangloss" added, so are "Pangloss's" and "Pangloss-like"
- "don't" and "well-known" are each checked as one word. A hyphenated word is accepted when each of its parts is
- Words with a digit in them aren't checked, and neither is Chinese or Japanese text
- A vault's dictionary is `.fernanda/dictionary.txt`, one word per line, and can be edited by hand. A hand edit takes effect when the vault is next opened, or when a word is next added from the menu
- Checking can be turned off, and the language chosen, per vault in Settings
- For another language, put its dictionary's two files (`.aff` and `.dic`) in Fernanda's dictionaries folder. The Open folder button beside the language list opens it. A dictionary the vault names but the folder doesn't have is listed as not installed, and nothing is checked
- A dictionary in a text encoding Fernanda can't convert is listed as an unsupported encoding, and nothing is checked while it is chosen. UTF-8 and Latin-1 (ISO 8859-1) dictionaries always work; whether others do depends on how Qt was built

## PDFs and images

Read-only views.

- Zoom with Ctrl+wheel, Ctrl+= and Ctrl+-, or the control floating over the view. Ctrl+0 resets to fit
- PDFs: all pages in one scrolling view, fitted to width when first opened
- Images: fitted to the pane, and never enlarged past their real size by fitting. When zoomed in, drag to pan. Ctrl+wheel zooms toward the pointer
- Animated GIF and WebP images play
- Photos are shown the right way up (EXIF orientation is applied)
- A file that can't be read shows a message in place of a blank view

## Status bar

Shown in vault windows and pop-outs, for the active text tab. Empty over a PDF, an image, or a new tab.

- Word counter: words and characters, and optionally lines. With text selected it reads "12 of 1,234 words"
- Cursor position: line and column (off by default)
- Words: "don't", "well-known", "e.g.", and "1,000" each count as one. Dashes, slashes, and markup symbols are not words. Chinese and Japanese text counts one word per character
- Characters count what you see: an emoji or an accented letter is one. Spaces are included, line breaks are not
- Lines are paragraphs, not wrapped rows
- An arrow at its left appears when a newer version of Fernanda is out (see [Updates](#updates)). The status bar shows while it does, even with both items switched off

## Focus timer

A timer for writing in focus periods, with a short break after each and a longer one after several, after the Pomodoro technique. Off until you turn it on for a vault, in Settings → Focus timer.

- A button beside the gear at the bottom of the sidebar. Its icon is a stopwatch for focus and a cup for a break, and a ring around it empties as the period runs. Paused, the ring is grey
- Click it to start, pause, or resume. Right-click it to skip to the next period or to reset, and to see the time left and how many focus periods you've completed. Hovering over it shows the same
- When a period ends, the ring fills and the next period waits for a click: nothing starts on its own. If you're in another program, Fernanda's taskbar entry flashes. There is no sound
- A long break follows every fourth focus period. A skipped focus period doesn't count toward it
- Focus is 25 minutes, a short break 5, and a long break 15. All three, and how many focus periods come before a long break, can be changed in Settings. A new length applies from the next period
- Each vault's window has a timer of its own. Pop-outs don't show one
- The timer stops when its window closes, and nothing about it is saved. Turning it off resets it
- Time passes while the computer sleeps: a period that ran out meanwhile ends when it wakes

## Git

For a vault inside a Git repository, Fernanda can show the repository's changes, commit them, and work with its branches and its remote. It does what GitHub Desktop does, in Fernanda's own sidebar and tabs. It uses the Git installed on your computer. Without Git, or for a vault in no repository, none of this shows.

- When the vault is in a repository, two buttons appear at the top of the sidebar: Files and Git. Git shows the repository's page in the sidebar; Files goes back to the file tree and the Common Vault. The current one is shaded. Without a repository the buttons aren't there, and the sidebar looks as it always has
- Ctrl+Shift+G, or View → Git, switches between the two
- The rest of the window works as usual while the Git page shows
- The repository is the one the vault's folder is in, found as Git finds it, by looking upward. It may hold more than the vault (other vaults, or files Fernanda doesn't open), and the Git page shows every change in it
- Show Git, in Settings → Git, turns it off for a vault. It starts on
- The Git page reads the repository again each time it shows, whenever the window comes forward, and when a file in the repository is saved, renamed, moved, or trashed in Fernanda while the Git page or a diff shows

### Changes and diffs

- The Git page lists every changed file by name, with its folder after it in muted letters and a mark at the right in the change's color: green for added, amber for modified, red for deleted, blue for renamed, and a warning triangle for a file left conflicted by a merge
- A folder inside the repository that holds a Git repository of its own isn't listed
- Double-click a file, or press Enter, to open its diff in a tab, over the current tab unless that one is pinned, as a file opens from the file tree. Right-click for Open in new tab, Open to the right, and Open in new window
- Right-click a file for **Open file** too, which opens the file itself in a new tab (for a file in the window's vaults that Fernanda can open). If it's already the current tab, it's just shown
- A diff's tab is marked with the branch glyph, so it isn't taken for the file's own tab. Removed lines are red and added lines green, each with its line numbers in the old file and the new one. Within a changed line, the words that changed are marked more strongly. A line rewritten almost entirely gets no word marks, since nearly every word would have them. Lines wrap to the width of the tab, and the text is in the vault's font
- When a file ends without a line break, the diff says so under its last line
- A diff follows its file: it updates as the file is saved in Fernanda, and when the window comes forward after a change made elsewhere, keeping its place, and says there are no changes once the file is committed or discarded. Right-click for Copy and Select All
- Diff tabs can be pinned, split, and moved to other windows, like any tab, but aren't restored on the next launch

### Commits

- Each file has a checkbox, and the box above them checks or clears all of them and gives the count. A file unchecked stays unchecked until it's checked again, or until it's committed
- The commit box, at the bottom of the Git page, takes a summary, which is required, and a description. "Commit to *branch*", or Ctrl+Enter anywhere in the box, commits the checked files. Everything open is saved first
- Only the checked files are committed. A file staged from a terminal and left unchecked stays staged

### Discarding

- Right-click a file for **Discard changes…**. It always asks first, and Cancel is the default
- A copy of the changed file goes to the system trash, and the file goes back to how it was in the last commit. A tab showing it reloads in place, so Ctrl+Z there brings the changes back too. A file that isn't in the last commit goes to the trash itself. A renamed file goes to the trash, and its old name comes back as it was. A deleted file just comes back. A discarded copy restored from the trash comes back to a temporary folder, not to the vault
- A conflicted file can't be discarded from the Git page. Settling a conflict is left to Git

### Branches

- The branch button, at the top of the Git page, names the current branch. Click it for a menu: the local branches, with the current one checked; under **Remote branches**, the branches on the remote that have no local copy; **New branch…**; and **Create pull request**, when there's one to make
- Choosing a local branch switches to it. Choosing a remote one makes a local branch that tracks it, and switches to that
- New branch… asks for a name and makes the branch from the current one, then switches to it, taking any uncommitted changes along. The name is fixed as you type, as GitHub Desktop fixes it: spaces and characters Git refuses become hyphens, and a line under the field says what the branch will be called. Git still checks the name, and a name it refuses keeps the dialog open with Git's reason
- **Create pull request** shows for a branch published to a remote on github.com, when it isn't the remote's default branch. It opens GitHub's page for a new pull request in your browser
- Switching branches, and pulling, are refused while a file Git tracks has uncommitted changes, including any typed a moment before: commit or discard them first. New files Git doesn't track yet don't count
- Files a switch or a pull changes reload in their tabs, as any change made outside Fernanda does, and a tab on a file the other branch doesn't have closes

### Syncing

- The sync button, right of the branch button, does what GitHub Desktop's does, in the same order: it publishes a branch the remote doesn't have (or no longer has), pulls when commits are waiting, pushes when there are commits to send, or else fetches. Its glyph says which, with how many commits are waiting (↓) and to send (↑) beside it
- Hover over it to see what it will do and when the repository last fetched. The glyph turns while it runs
- The remote is the one the branch tracks, otherwise `origin`, otherwise the only remote there is. With none, the button is disabled
- Fernanda never fetches, pulls, or pushes on its own. Nothing contacts the remote until you click
- A pull only fast-forwards. If the branches have both moved on, the pull is refused, with Git's reason, and nothing is merged
- Signing in to the remote is left to Git. A credential manager Git is set up with works as it does from a terminal; one that would ask for a password in a terminal fails instead. A fetch, pull, or push that hasn't finished after two minutes is stopped

### Where it differs from GitHub Desktop

- It lives in the sidebar, and each diff opens in a tab, rather than in a window of its own
- Diff lines wrap, since a paragraph of prose is one line. Desktop's run off to the right
- Each changed word in a line is marked. Desktop marks one stretch from the first changed character to the last, and nothing in a line of 1,024 characters or more
- No fetch on a timer. Desktop fetches every hour
- The branch list has no filter
- A folder holding a repository of its own isn't listed. Desktop lists it as a new file, and committing it records only a pointer to that other repository's current commit
- No stash. Switching or pulling with changes is refused, where Desktop offers to set the changes aside, or bring them along
- No history, no merges, no amending, no discarding every change at once, and no committing part of a file

## Look

Fernanda draws its own interface, the same on every operating system: flat and quiet, with rounded corners, in the colors of the system's light or dark mode.

- It follows the system's light or dark mode, and switches with it while Fernanda is running
- The system's accent color marks the default button in a dialog, a focused text field, the selected row of a list, a slider's filled track, and a checked box
- The current tab is the color of the page below it
- Menus are rounded, with a soft shadow, and so are the drop-down lists of choices in Settings
- The current line highlight is a soft blue on a light page and a muted red on a dark one

## Settings

Ctrl+, or the gear beside the vault switcher. Settings belong to the vault and apply as you change them, apart from General's, which belong to Fernanda as a whole.

- **General**: the version, Check for updates, whether Fernanda checks on its own, and the interface language
- **Editor**: line numbers, wrap lines, left/right margin, highlight current line, selection handles, tab width, center on scroll, double-click selects whitespace, typing helpers (auto-pair brackets and quotes, step past closing punctuation), check spelling, spelling language
- **Appearance**: text font, bold, italic, font size
- **Status bar**: the word counter and the cursor position, and which parts of each to show
- **Focus timer**: whether it shows, the lengths of focus and of each break, and how many focus periods come before a long break
- **Git**: whether the Git page shows in the sidebar, for a vault in a repository

A Common Vault file opened in a project's window uses that project's settings.

## Updates

Fernanda tells you when a newer version is out. It doesn't download or install it: to update, download the installer from the release page and run it, as you installed Fernanda.

- Settings → General shows the version you have and a Check for updates button. When a newer version is out, it names it, and Open release page takes you to it
- Fernanda also checks on its own, a few seconds after it starts and then once a day while it runs. A check is one request to GitHub for the list of releases. It sends nothing from your vaults or files, but like any web request it shows GitHub your IP address, and it names Fernanda's version. The [privacy statement](../../PRIVACY.md) has the details
- "Check for updates automatically", in General, turns this off. The Windows installer asks too: its answer is the starting point for everyone who uses that install, and the switch in General changes it for you. The macOS and Linux builds have no installer, so there it starts on
- When a newer version is found, an arrow appears in the status bar of every window. Clicking it opens General. It then stays hidden until a still newer version comes out
- While Fernanda is in beta, every release counts, betas included

## Language

Fernanda's menus, settings, and messages are in English and Spanish.

- By default Fernanda uses your system's language, if it's one of these, and English otherwise
- Settings → General → Language picks one instead. The change takes effect the next time Fernanda starts, and a Relaunch button beside the setting restarts Fernanda to apply it, saving everything first as Quit does
- Standard parts of dialogs, such as their Cancel buttons, follow the same language
- Numbers and dates keep your system's regional format whatever the language

## What Fernanda keeps in a vault

| File | Holds | Meant to be committed or synced? |
|---|---|---|
| `.fernanda/settings.json` | Editor, spelling, status bar, focus timer, and Git settings | Yes |
| `.fernanda/dictionary.txt` | Words added to the vault's dictionary | Yes |
| `.fernanda/appearance.json` | Font settings | Yes |
| `.fernanda/workspace.json` | Window, pane, and tab layout | No, it is specific to one machine |
| `.fernanda/.gitignore` | Ignores `workspace.json` | Yes |

Deleting `.fernanda/` while the vault is closed resets its settings, layout, and dictionary, and nothing else.

The list of known vaults, and which of them is the Common Vault, is kept outside any vault, in Fernanda's application data folder. So are the spelling dictionaries, in its `dictionaries` folder. The bundled one is copied there on launch if it's missing, and never overwritten.

## Keyboard shortcuts

| Action | Shortcut |
|---|---|
| New file | Ctrl+N |
| New folder | Ctrl+Shift+N |
| Go to file | Ctrl+O |
| Settings | Ctrl+, |
| Undo / Redo | Ctrl+Z / Ctrl+Y |
| Cut / Copy / Paste | Ctrl+X / Ctrl+C / Ctrl+V |
| Select all | Ctrl+A |
| Find | Ctrl+F |
| Find and replace | Ctrl+H |
| Next / previous match | F3 / Shift+F3 |
| Zoom in / out / reset | Ctrl+= / Ctrl+- / Ctrl+0 |
| Switch the sidebar between the files and the Git page | Ctrl+Shift+G |
| Commit, on the Git page | Ctrl+Enter |
| Show or hide the menu bar | Ctrl+M |
| Quit | Ctrl+Q |

The menu bar is hidden until asked for. Every shortcut works without it.

## Differences from Obsidian

Fernanda follows Obsidian's behavior wherever it has no reason not to. Where it differs:

- **Plain text first.** `.txt` is the default type (Obsidian doesn't open it). There is no Markdown rendering, and no links, backlinks, tags, or metadata
- **A Common Vault shared by every vault** (see [The Common Vault](#the-common-vault)). Obsidian has no way to share files between vaults; its help advises against the symlinks people use for it
- **Vaults inside vaults are supported.** A file inside two open vaults is one document in every window, and the vaults' file operations keep every window in step. Obsidian's help has recommended against nesting vaults
- **Line endings are preserved.** Obsidian rewrites them to LF
- **A failed save refuses the window close.** Obsidian shows a notice and closes
- **A dropped file opens.** A file dragged onto an editor opens there; in Obsidian it inserts a link
- **Bold and italic font settings**, since there is no Markdown styling to carry emphasis
- **A margin setting** in place of "Readable line length", and a switch to turn wrapping off
- **Find has Match case and Whole word.** Obsidian's search in a file has neither. Fernanda's highlights also stay while you edit, until the bar is closed
- **Go to File lists every file with nothing typed,** the recent ones first. Obsidian's Quick switcher lists only the recent ones
- **Files created from Go to File are `.txt`** unless the name has another text extension. Obsidian's are Markdown. They always start at the vault's root; Obsidian follows its "Default location for new notes" setting, whose default is also the root
- **Delete and Select All in the right-click menu.** Obsidian's editor menu has only Cut, Copy, and Paste
- **One spelling language per vault.** Obsidian can check several at once
- **TIFF images** are supported. SVG and AVIF are not
- **Images zoom**, like PDFs
- **File and folder icons** in the file tree
- **Clicking off the file tree's rows clears its selection.** Obsidian keeps a selection until another entry is clicked
- **Manage Vaults can list vaults by name.** Obsidian's vault list is always most recently used first
- **No fallback trash.** If the system trash refuses an entry, Fernanda declines; Obsidian moves it to a `.trash` folder in the vault
- **A vault whose folder vanishes has its window closed**, after a message, where Obsidian leaves an empty window open. This includes a vault folder deleted from inside another vault
- **No Move Vault command.** Obsidian's vault switcher can move a vault to another folder. In Fernanda, move the folder from the file tree of a vault that contains it, and the vault follows; or move it in your file manager, then open it from its new place
- **Licenses are in the app.** Obsidian lists its third-party credits on its help site
- **No light or dark setting.** Fernanda always follows the system. Obsidian follows it by default and can be set to light or dark
- **Stepping past closing punctuation** is Fernanda's own, as is the switch over both typing helpers
- **No Markdown pairing.** Obsidian's "Auto pair Markdown syntax" pairs `*`, `_`, and the like, and applies them to selected text. Fernanda pairs brackets and quotes only
- **A focus timer is built in.** Obsidian has none; community plugins add one, and some of them log each session to a note. Fernanda's settings are kept per vault, as a plugin's are, and it writes nothing into the vault
- **Git is built in.** Obsidian has none; its community plugin Obsidian Git adds commits, pulls, and pushes, which it can also run on a schedule. Fernanda's Git page does what GitHub Desktop does, and contacts the remote only when you click
- **Updates aren't installed for you.** Fernanda says a newer version is out and links to it. Obsidian downloads updates itself and applies them when it restarts

## Known limits

- A file name with a period and no real extension (`Chapter 1. The Start`) is read as having an unsupported extension, so the file is hidden
- On Windows, while Fernanda is open, other programs can't rename or delete a folder if one of its subfolders has been expanded in a file tree or holds an open file. Fernanda's own rename, move, and delete are unaffected
- A vault folder moved or renamed outside Fernanda is a new vault to Fernanda: open it again from its new place. Its settings and layout travel with it
- A Common Vault whose folder can't be found at launch, such as one on a drive that isn't connected, is unset as if the folder were gone. Set it again from Manage Vaults
- A folder linked into a vault (a symlink or junction) works as part of the vault. A file that can be reached two ways, through a link and directly, is two documents, which can save over each other. Keep a linked folder's target outside every vault, as Obsidian also asks
- Word counts run low for Thai, Lao, Khmer, and Myanmar text
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Right-to-left layouts are not supported
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- macOS and Linux builds exist but are not well tested
