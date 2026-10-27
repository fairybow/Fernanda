# Changelog

[Skip to release content](#releases)

---

# Boilerplate

(See [build_release_text.py](https://github.com/fairybow/Fernanda/blob/main/.github/scripts/build_release_text.py))

<!-- release-preamble-start -->

**Fernanda is a plain-text writing app for fiction, poetry, and long projects.**

This is a soft release. For a full feature list, see [Features.md](https://github.com/fairybow/Fernanda/blob/main/Fernanda/docs/Features.md). For past release details, see [CHANGELOG.md](https://github.com/fairybow/Fernanda/blob/main/CHANGELOG.md)

> [!WARNING]
> You should not trust your writing with any version of this software less than 1.0.0! Regardless, always make regular backups of your work.

<!-- release-preamble-end -->

<!-- release-footer-start -->

## Installation

**Windows:** Download and run the installer below.

**macOS:** Download the `.dmg`, open it, and drag Fernanda to Applications.

**Linux:** Download the `.AppImage`, make it executable (`chmod +x`), and run.

> [!NOTE]
> **Windows:** This build is unsigned, so Windows Defender SmartScreen will likely show a warning. Click **More info -> Run anyway** to proceed.
>
> **macOS:** This build is unsigned. You may need to right-click and select **Open** the first time, or allow it in **System Settings -> Privacy & Security**.

## Updating

**Windows:** Download the newest installer, ensure Fernanda is closed, then run the installer and install to the same directory (default `C:/Program Files`), overwriting.

**macOS:** Replace the app in Applications with the new version from the `.dmg`.

**Linux:** Replace the old `.AppImage` with the new one.

## Uninstalling

**Windows:** Run the uninstaller (`unins000.exe`) or remove via **Add or Remove Programs** as usual.

**macOS:** Drag Fernanda from Applications to the Trash.

**Linux:** Delete the `.AppImage`.

Uninstalling leaves these folders in place. The first holds only app data and is safe to remove. Each vault's `.fernanda/` holds its settings and the words you've added to its dictionary. `Documents/Fernanda/` holds your writing: delete it only if you mean to.

| Folder | Location | Contents |
|---|---|---|
| App data | `AppData/Local/Fernanda` (Windows), `~/Library/Application Support/Fernanda` (macOS), `~/.local/share/Fernanda` (Linux) | Known vaults, session, logs, spelling dictionaries |
| Default vault location | `Documents/Fernanda/` | Your vaults and the Common Vault |
| Per-vault settings | `.fernanda/` inside each vault | Settings, layout, and the vault's dictionary |

## Platforms

Windows (x64), macOS (ARM), and Linux (x86_64).

> [!NOTE]
> macOS and Linux builds are available but not well-tested. Bug reports are welcome!

## Privacy

Fernanda never sends your writing anywhere. The only thing it sends on its own is a daily check with GitHub for a newer version, which you can turn off when installing on Windows or in Settings → General. See the [privacy statement](https://github.com/fairybow/Fernanda/blob/main/PRIVACY.md).

:heart:

<!-- release-footer-end -->

---

# Notes

- For release note links: use `blob/main` for evergreen links; use `blob/<tag>` for links to a specific release's snapshot!
- For diffing against previous release: `https://github.com/fairybow/Fernanda/compare/<tag>...main.diff`
- Release commands:

```
git tag v0.0.0-beta.0 [must match tag part of entry title (see below)]
git push origin v0.0.0-beta.0
```

---

# Release Entry Template

```
# 0.0.0-beta.0 (Testing / Soft Release) - tag v0.0.0-beta.0

## What's New?

- ...

## Known Issues

- ...
```

---

<a id="releases"></a>

# 0.12.0-beta.0 (Git and Focus Timer) - tag v0.12.0-beta.0

## What's New?

**Git.** For a vault inside a Git repository, Fernanda can now show what's changed, commit it, and work with branches and the remote. It does what GitHub Desktop does, in Fernanda's own sidebar and tabs, using the Git installed on your computer. Without Git, or for a vault in no repository, nothing changes.

- **Files** and **Git** buttons appear at the top of the sidebar when the vault is in a repository. Ctrl+Shift+G, or **View → Git**, switches between them. The rest of the window works as usual while the Git page shows
- The Git page lists every changed file, with a colored mark for added, modified, deleted, or renamed. Double-click a file to open its diff in a tab: removed lines in red, added in green, and the words that changed marked within each line. A diff follows its file as you type and save
- Check the files you want, write a summary, and **Commit to *branch*** (or Ctrl+Enter). Only the checked files are committed
- Right-click a file to **Discard changes**. It always asks first, and a copy of what's discarded goes to the system trash. A tab showing the file reloads in place, so Ctrl+Z there brings the changes back
- The branch button switches branches, makes a new one (taking your uncommitted changes along), switches to a branch from the remote, and opens GitHub's page for a new pull request
- The sync button does what GitHub Desktop's does: publishes a new branch, pulls when commits are waiting, pushes when there are commits to send, or else fetches, with how many commits are waiting (↓) and to send (↑) beside it. A pull keeps your uncommitted changes
- Typing while Git updates your files is never written over what Git brings in. If Git changes the file you're typing in, the tab shows Git's version, and Ctrl+Z brings back what you typed
- Fernanda never contacts the remote on its own: only when you click the sync button. Signing in is left to Git, through whatever credential manager or SSH agent it's set up with
- A repository owned by another user on your computer, as one on an exFAT USB drive often is, shows GitHub Desktop's **Trust repository** button instead of the changes (though note that, at this moment, Fernanda is not hardened to work with removable drive content)
- **Show Git**, in Settings → Git, turns it off for a vault

See [Features.md](https://github.com/fairybow/Fernanda/blob/v0.12.0-beta.0/Fernanda/docs/Features.md#git) for everything the Git page does, and where it differs from GitHub Desktop (our exemplar for Git UI behavior).

**Focus timer.** A timer for writing in focus periods, with a short break after each and a longer one after several. It's off until you turn it on for a vault, in Settings → Focus timer.

- A button beside the gear at the bottom of the sidebar. Click it to start, pause, or resume. A ring around it empties as the period runs
- Right-click it to skip to the next period or reset, and to see the time left and how many focus periods you've completed (though at the moment, the time left does not update in place)
- When a period ends, the next one waits for a click, and Fernanda's taskbar entry flashes if you're in another program
- Focus is 25 minutes, a short break 5, and a long break 15, after every fourth focus period. All of these can be changed in Settings

**Privacy**

- Fernanda has a [privacy statement](https://github.com/fairybow/Fernanda/blob/main/PRIVACY.md). In short: your writing never leaves your computer (aside from going to a repo, if you're using Git), and the only thing Fernanda sends on its own is the daily update check
- The Windows installer now shows the privacy statement and asks whether to check for updates automatically

## Known Issues

- Switching branches is refused while you have uncommitted changes. Commit or discard them first. Setting them aside (stashing) is planned
- A pull only fast-forwards: if you've committed on two computers, the next pull is refused and nothing is merged
- The Git page has no commit history yet, and no way to undo or amend a commit. Both are planned
- The Spanish translation is an unreviewed draft
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- Closing all windows at once from the Windows taskbar reopens only one vault on the next launch
- A file that can be reached two ways, through a symlinked folder and directly, is two documents, which can save over each other
- macOS and Linux builds are not well-tested

---

# 0.11.0-beta.0 (Common Vault Rework and Nested Vaults) - tag v0.11.0-beta.0

## What's New?

**The Common Vault is now a role any vault can have.** Right-click a vault in **Manage Vaults** and choose **Set as Common Vault**, or **Unset Common Vault** to have none. Every open window follows at once. Your current Common Vault stays the Common Vault.

- If the Common Vault's folder is deleted, or moved outside Fernanda, it's no longer recreated empty: there's no Common Vault until you set one
- Hovering over the drawer's header shows the Common Vault's name

**Nested vaults work together.** Vaults can sit inside one another, and a project vault can contain the Common Vault. Every window now shares one set of open files, so vaults that overlap stay in step:

- A file inside two open vaults is one document in every window. Before, each vault kept its own copy, so their autosaves could overwrite each other, and a rename in one could lose a few seconds of typing in the other
- Renaming, moving, or deleting a file or folder from either vault carries every window's tabs with it. A folder expanded in one window stays expanded after another window moves it
- A file moved out of a vault from another vault's window has its tabs closed in that vault's windows, and unsaved typing goes with the file. A file moved between a window's vault and the Common Vault gains or loses its Common Vault mark in place
- Renaming or moving a folder that holds another vault keeps that vault open at its new place, with its tabs, settings, dictionary, and spot in the vault list. Deleting one closes the vault's window at once, with a message

**Manage Vaults**

- **Rename** works on open vaults and the Common Vault too, which stay open under the new name. It checks the new name as you type, and allows a change of letter case alone
- A button beside **Recent vaults** lists vaults by name, A to Z or Z to A, or most recently used first. Your choice is kept
- A vault whose folder is missing no longer has a taller row

**File tree**

- **New file** and **New folder** open the folder they're made in, and select the new entry
- Clicking anywhere off a row's highlight clears the selection

**Fixed**

- A vault on a network share (`\\server\share`) showed as missing after a relaunch
- On Linux and macOS, a backslash in a file name was taken for a folder separator
- On Windows and macOS, one folder opened by two letter cases opened two windows
- Names Windows reserves, such as `COM¹`, were allowed in some places
- A rename onto the name of a file just deleted outside Fernanda could leave the deleted file open
- Opening or closing the Common Vault drawer could leave a row lit under the pointer

## Known Issues

- The Spanish translation is an unreviewed draft, and some newer text isn't translated yet
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- Closing all windows at once from the Windows taskbar reopens only one vault on the next launch
- A file that can be reached two ways, through a symlinked folder and directly, is two documents, which can save over each other
- macOS and Linux builds are not well-tested

---

# 0.10.1-beta.0 (New Icon) - tag v0.10.1-beta.0

## What's New?

- A refined icon: the ring's colors now blend with a long, soft middle, and its edges are a darker shade of the same colors

## Known Issues

- The Spanish translation is an unreviewed draft, and the typing helper settings aren't translated yet
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.10.0-beta.0 (Fernanda) - tag v0.10.0-beta.0

## What's New?

**Suzuri is now Fernanda.** Another writing app already uses the name Suzuri, so this one goes back to the name its earliest versions had. Nothing about how the app works has changed.

- A new icon
- **Fixed:** on macOS, the app's icon file held only a 16-pixel image, so the app showed a blurry or generic icon. It now has every size up to 1024 pixels
- The app's folders now carry the new name: its settings and data live in a `Fernanda` folder, new vaults go in `Documents/Fernanda/`, and each vault keeps its settings in `.fernanda/`

### Coming from Suzuri

Fernanda installs as a new app, next to Suzuri rather than over it, and doesn't pick up Suzuri's settings on its own. To carry everything over:

1. **Windows:** uninstall Suzuri, then install Fernanda. **macOS:** replace Suzuri in Applications with Fernanda. **Linux:** replace the Suzuri AppImage with the Fernanda one
2. In each vault, rename the `.suzuri` folder to `.fernanda`. It holds the vault's settings, layout, and dictionary
3. To keep your list of vaults and your other settings, copy the app data folder `Suzuri` to `Fernanda` (in `AppData/Local` on Windows, `~/Library/Application Support` on macOS, `~/.local/share` on Linux), then rename `suzuri.json` inside it to `fernanda.json`. (You may also want to rename paths within that JSON that reference the old name)

Your vaults themselves can stay where they are. If you skip step 3, open them again from **Manage Vaults**.

## Known Issues

- The Spanish translation is an unreviewed draft, and the typing helper settings aren't translated yet
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.9.1-beta.0 (Fixes) - tag v0.9.1-beta.0

## What's New?

- **Fixed:** typing a space after punctuation that had gone before a closing bracket or quote gave two spaces: `(word), ` and a space became `(word),  `. That space now counts as the one you'd type next, and Enter there no longer leaves a space at the end of the line
- **Fixed:** closing a window with tabs open ran code on part of the window that had already been taken apart. No problems were seen from it, but it was the kind of mistake that can cause a crash
- Suzuri's description, shown in the Windows file properties and the Linux app menu, now reads "A plain-text writing app for fiction, poetry, and long projects"

## Known Issues

- The Spanish translation is an unreviewed draft, and the typing helper settings aren't translated yet
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.9.0-beta.0 (Typing Helpers) - tag v0.9.0-beta.0

## What's New?

Suzuri can now help with brackets, quotes, and the punctuation after them as you type. Both helpers are on by default, and each can be turned off in **Settings → Editor → Behavior**, along with a switch for both.

- **Auto-pair brackets and quotes.** Typing `(`, `[`, `{`, a quote, or a backtick adds its closing match, when the cursor is at the end of a line or before a space or another closer. Before a word, you get just the opening character, so you can wrap text that's already there
  - Straight quotes don't pair after a letter or a number, so `don't` and `5'` are typed as they are
  - Curly quotes and guillemets (`“”`, `‘’`, `«»`, `‹›`) pair with their own closers
  - Typing the closer that was added steps over it, and Backspace between an empty pair deletes both
  - With text selected, typing an opening character wraps the text in the pair
- **Step past closing punctuation.** A second space typed just before a closing bracket or quote moves past it: `(word |)` becomes `(word) |`
  - Enter does the same when only closing characters are left on the line, so a paragraph that ends inside a bracket or quote ends after it
  - A comma, period, question mark, exclamation mark, semicolon, or colon typed straight after goes before the space: `(word), `
- Each of these undoes in one step

Also in this release:

- **Fixed:** a selection handle at the very start or end of a line was cut off by the edge of the text area
- **Fixed:** on the Editor settings page, Enter opened the dictionaries folder, as **Open folder** had become the page's default button. No button in Settings is a default now
- **Fixed:** the **Options** heading in Settings lit up on hover, as if it could be chosen
- The text in the editor now starts slightly lower on the page, leaving room for the selection handles

## Known Issues

- The Spanish translation is an unreviewed draft, and the new settings aren't translated yet
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.8.1-beta.0 (Fixes) - tag v0.8.1-beta.0

## What's New?

- **Fixed:** drop-down lists (in Settings, for example) showed a second, square edge around their rounded one, and could be too narrow for their longest choice. They're now drawn like the menus: rounded, with a soft shadow, and as wide as their box
- **Fixed:** a drop-down box's longest choice could lose its last character, as in "English (United States)"
- **Fixed:** clicking **Check for updates** moved the keyboard focus to the next setting

## Known Issues

- The Spanish translation is an unreviewed draft
- On Linux without a compositing window manager, menus and drop-down lists may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.8.0-beta.0 (Languages) - tag v0.8.0-beta.0

## What's New?

Suzuri can now be used in another language, starting with Spanish.

- **Settings → General → Language** chooses the language of Suzuri's menus, settings, and messages. **System default** uses your system's language when Suzuri has it, and English otherwise
- **A new language takes effect when Suzuri starts again.** After you choose one, a **Relaunch** button appears next to the list. It closes Suzuri as Quit does and reopens it in the new language
- Text that Qt draws itself, such as the buttons in standard dialogs, follows the language too. Numbers and dates keep your system's format
- **The Spanish translation is a first draft** and hasn't been reviewed yet. Expect some awkward or mistaken wording

Also in this release:

- **Fixed:** at 100% display scaling, the X on tabs and the arrows in the file tree were drawn lopsided

## Known Issues

- The Spanish translation is an unreviewed draft
- Drop-down lists (in Settings, for example) are square, unlike the menus
- On Linux without a compositing window manager, menus may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.7.0-beta.0 (Updates) - tag v0.7.0-beta.0

## What's New?

Suzuri can now tell you when a newer version is out.

- **Settings → General** is a new page. It shows the version you have and a **Check for updates** button. When a newer version is out, it names it, and **Open release page** takes you to the download
- **Suzuri checks on its own,** a few seconds after it starts and then once a day while it runs. To stop it, turn off **Check for updates automatically** in General. A check is one request to GitHub for the list of releases, and sends nothing about you or your files
- **A "download arrow" icon appears in the status bar** of every window when a newer version is found. Click it to see the update in General. It then stays hidden until a still newer version comes out
- Suzuri doesn't download or install updates itself. To update, download the installer from the release page and run it, just as you installed Suzuri

## Known Issues

- Drop-down lists (in Settings, for example) are square, unlike the menus
- On Linux without a compositing window manager, menus may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.6.0-beta.0 (New Look) - tag v0.6.0-beta.0

## What's New?

Suzuri has a new look: flat, quiet, and rounded, and the same on Windows, macOS, and Linux.

- **Light and dark follow your system,** including when you switch while Suzuri is running. Your system's accent color marks the default button, a focused field, and the selected row in a list
- **Tabs:** the current tab is the color of the page below it, and the others are quieter until you hover over them
- **The sidebar** is lighter in light mode. Rows in the file tree get a soft highlight on hover and when selected, and a folder's highlight takes in its arrow. The Common Vault drawer's header highlights on hover too
- **Menus** are rounded, with a soft shadow
- **Go to File** has roomier rows, and the selected one is marked with a bar in the accent color
- **The editor** keeps a little space between the text and the pane's edges. The current line highlight is a soft blue on a light page and a muted red on a dark one
- Scroll bars are thin and have no arrows. The find bar sits on the page in the page's color

Also in this release:

- **Fixed:** the Common Vault mark on a tab now changes color with a light or dark switch

## Known Issues

- Drop-down lists (in Settings, for example) are square, unlike the menus
- On Linux without a compositing window manager, menus may show dark corners outside their rounded edges
- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.5.0-beta.0 (Go to File) - tag v0.5.0-beta.0

## What's New?

Go to File (Ctrl+O) can now take you back to recent files and create new ones.

- **Recent files come first.** With nothing typed, the files you've opened most recently are listed at the top, newest first and labeled RECENT, followed by every other file. If the top one is the file you're already in, the next one is selected, so Ctrl+O then Enter takes you back to the file before. Recent files are remembered per vault
- Common Vault files are now labeled COMMON instead of marked with an icon
- **Create a file by typing its name.** Enter creates it when nothing matches; Shift+Enter creates it even when something does. Hold Ctrl as well to open it in a new tab. A line under the search field shows what will be created, or what's wrong with the name
- Type a path to put the new file in folders, which are made if they don't exist yet (`drafts/Chapter 2`). New files are `.txt` unless you name them `.md`, `.markdown`, or `.fountain`. If the file already exists, it opens instead

Also in this release:

- **Fixed:** focus now lands in the editor after restoring a session, on the first click of a tab, and after dragging a tab to another pane or window
- A spelling language in a hand-edited `settings.json` must now be a plain dictionary name. Anything else is treated as not installed

## Known Issues

- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.4.0-beta.0 (Licenses) - tag v0.4.0-beta.0

## What's New?

Suzuri now credits everything it's built with.

- **Licenses**, under the version in Manage Vaults, lists Suzuri and everything it's built on or bundles: Qt, Hunspell, the English dictionary, the four bundled fonts, and the Lucide icons. Choose one to read its full license text
- Suzuri's own license, the GNU GPL v3, is first in the list

## Known Issues

- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Focus doesn't always land in the editor after dragging a tab or restoring a session
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.3.0-beta.0 (Edit Menu) - tag v0.3.0-beta.0

## What's New?

The rest of the usual editing commands are now in the menus.

- **Cut, Copy, Paste, Delete, and Select All** are in the Edit menu (Ctrl+M shows the menu bar) and in the text editor's right-click menu
- In the Edit menu they act on whatever text you're in, so they work in the find field as well as the editor
- In the right-click menu, commands that can't apply are greyed out: Cut and Copy with nothing selected, Paste with nothing on the clipboard
- **Copying keeps your text exactly as it is.** No-break spaces and other special characters used to be changed on the way to the clipboard. Copied text is now plain text only, so when you paste into Word or an email it takes on that document's formatting instead of bringing the editor's font with it

Also in this release:

- Selection handles now only appear (if enabled) once you've finished making a selection, when you let go of the mouse

## Known Issues

- Web addresses and file paths are spellchecked piece by piece, so parts like "github" and "com" are marked
- Spelling suggestions can take a moment to appear for a long or badly garbled word
- Focus doesn't always land in the editor after dragging a tab or restoring a session
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.2.0-beta.0 (Spellcheck) - tag v0.2.0-beta.0

## What's New?

Suzuri now has spellcheck.

- Misspelled words get a red wavy underline. The word you're typing isn't marked until you move on from it
- **Right-click** a marked word for up to five suggestions. Choosing one replaces the word, and one undo puts it back. Suggestions keep your apostrophe style, straight or curly
- **Add to dictionary** saves the word to the vault's own dictionary, `.suzuri/dictionary.txt`, which travels with the vault. Words added to the Common Vault's dictionary are accepted in every vault
- **Ignore** accepts a word until the vault is closed
- Added words are accepted in their possessive and hyphenated forms too: with "Pangloss" added, so are "Pangloss's" and "Pangloss-like" accepted
- US English is included. For another language, put a Hunspell dictionary (the `.aff` and `.dic` files LibreOffice uses) in Suzuri's dictionaries folder. **Settings → Editor → Spelling** has a button that opens it
- Checking can be turned off, and the language chosen, separately for each vault

Each vault checks one language at a time right now.

Also in this release:

- **Fixed:** the text editor's right-click menu opened to the left of the pointer when line numbers or a margin were on

## Known Issues

- Web addresses and file paths are checked piece by piece, so parts like "github" and "com" are marked
- Suggestions can take a moment to appear for a long or badly garbled word
- Focus doesn't always land in the editor after dragging a tab or restoring a session
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.1.0-beta.0 (Find and Replace) - tag v0.1.0-beta.0

## What's New?

Suzuri can now find and replace text in a file.

- **Ctrl+F** opens a find bar across the top of the text. Matches are highlighted as you type, and the bar shows which one you're on and how many there are
- **Ctrl+H** opens it with a second row for replacing. The arrow at the left of the bar shows or hides that row
- Enter or F3 goes to the next match, Shift+Enter or Shift+F3 to the previous
- **Match case** and **Whole word** narrow the search. Whole word leaves longer words alone, so replacing "fart" won't touch "farthing"
- **Replace all** changes every match at once, and one undo takes it all back
- Each tab has its own find bar, so two panes on the same file can search for different things

The search is for plain text in one file. Patterns and searching a whole vault aren't built yet.

Also in this release:

- The README now has a screenshot, a feature list, and build instructions

## Known Issues

- Focus doesn't always land in the editor after dragging a tab or restoring a session
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.0.0-beta.1 (Soft Release) - tag v0.0.0-beta.1

## What's New?

A small release of fixes and safeguards.

- **Fixed:** on Windows, vaults, folders, and files with names outside the system's code page (emoji, and some accented or non-Latin letters) weren't handled correctly. Paths are now Unicode all the way through
- A vault now refuses any path that would step outside its folder, such as one containing `..`. Nothing in the app could produce one, but a hand-edited or damaged `workspace.json` could, and Suzuri would have opened and saved a file outside the vault
- If the views of a file open in more than one tab ever disagree about its text, Suzuri now notices and resets the view to the text that is being saved. No cause for this is known. It's a safety net
- Suzuri now has automated tests for its file handling, run on Linux and macOS for every change

## Known Issues

- Focus doesn't always land in the editor after dragging a tab or restoring a session
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested

---

# 0.0.0-beta.0 (Initial Release) - tag v0.0.0-beta.0

## What's New?

This is Suzuri's first public release.

Suzuri is a plain-text writing app for long-form fiction. Its design follows Obsidian: a vault is just a folder on your computer, and everything you write stays in ordinary files you own.

It's a hobby project, the writing app I've wanted for myself, and I hope to keep working on it for a long time. This release is the starting point: vaults, a file tree, tabs and split panes, pop-out windows, and autosave.

## Known Issues

- Focus doesn't always land in the editor after dragging a tab or restoring a session
- Pop-out windows can reopen behind their vault window
- macOS and Linux builds are not well-tested
