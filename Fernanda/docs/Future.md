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
- A toggle to show or hide the sidebar, remembered per vault. A hidden sidebar hides the focus timer's button with it, so the timer would need somewhere else to show
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

What GitHub Desktop does that the Git page doesn't yet, in the order it's planned. The Git page ships first as it is. Each of these follows the page's shape: a list in the sidebar, and what it shows in a tab. Where an item is planned in detail, the detail follows GitHub Desktop's own code, and is a proposal until the item is built: each point is settled then.

1. **History.** The current branch's commits, newest first, as in GitHub Desktop's History tab, with undo, amend, and revert
    - **Where.** Changes and History as two tabs in the Git page, under the branch and sync row, as in Desktop. The commit box shows under Changes only
    - **The list.** The commits reachable from the branch's tip (`log HEAD`), 100 at a time, more as the list scrolls, as Desktop loads them. Each row: the summary, then the author and how long ago, muted. Commits not yet pushed are marked, as Desktop marks them: those in `upstream..HEAD`, or with no upstream, `HEAD --not --remotes`. No avatars, which Desktop fetches from GitHub or Gravatar, since only the sync button reaches the network. No tag marks
    - **When it's read.** Only when the branch's tip moves. Porcelain v2 status already prints the tip (`# branch.oid`), which `GitStatus` reads today only to tell an unborn branch; kept, it lets a commit, undo, amend, revert, pull, or switch read the history again, and a save not. The unpushed marks follow the ahead count
    - **A commit's tab.** Clicking a commit opens a tab, placed as a diff tab is (over the active tab unless it's pinned), so commits can be stepped through. A header with the summary, description, author, date, and short hash; below it, the commit's files on the left and the selected file's diff on the right, as Desktop's right pane is. The files are read as Desktop reads them (`log <sha> -1 -m --first-parent -C -M --raw -z`), so a merge shows against its first parent, and the diff the same way for one file. Not restored on relaunch, as diff tabs aren't. The other way, the commit's files replacing the history list in the sidebar and each opening a diff tab, makes every commit a step in and out
    - **Undo.** For the newest commit, while it isn't pushed: from its right-click menu, and from Desktop's "Committed 2 minutes ago · Undo" line under the commit box, which is how Desktop's Undo is most often reached. A mixed reset to its parent, as Desktop does, so its changes come back uncommitted and nothing on disk is touched (no autosave hold needed). The first commit is undone by deleting the branch's ref, leaving it unborn. The summary and description go back into the commit box. Asks first when there are uncommitted changes, which the undone ones would mix with, or when it's a merge, as Desktop asks
    - **Amend.** For the newest commit, while it isn't pushed. The commit box takes the commit's message, a line says it's amending with a Cancel, and the button reads "Amend last commit". `commit --amend --only` with the checked files; with none checked, only the message changes, so the button stays enabled while amending. Desktop also amends a pushed commit, after a warning, and the next push must then be forced. The sync button doesn't force a push, which replaces the remote's branch and, for someone writing on two computers, can drop the other's commits; amending a pushed commit waits for merging (4), which settles diverged branches. Until then its menu item is disabled, with a tooltip saying why
    - **Revert.** A new commit that undoes one, leaving history as it was (`revert`, with `-m 1` for a merge, as Desktop). It rewrites files, so it's held as a switch or a pull is. Git refuses it over uncommitted changes to the same files. One that conflicts would leave conflict markers in the files and the repository mid-revert, which can't be settled until merging (4): it's aborted at once (`revert --abort`, which puts every file back), with a message that the commit can't be reverted cleanly yet
    - **Also on a commit's menu.** Create branch from commit (the New Branch dialog, `switch --create <name> <sha>`) and Copy SHA. View on GitHub is a convenience (6). Desktop's Reset to commit, Checkout commit, reorder, squash, and cherry-pick are left out (Not planned), and so are tags, which mark releases of code
2. **A file's own history.** The commits that changed one file, from its right-click menu in the file tree, and its text at any of them, read-only, with a way to bring that version back as an ordinary, undoable change. GitHub Desktop's history is by branch only; a writer more often asks what one chapter looked like last week
3. **Stashing.** Setting the uncommitted changes aside and bringing them back later, as GitHub Desktop does. The switch refusal is a stand-in until then
    - **One stash per branch,** as Desktop keeps them. Desktop marks its stashes by their message (`!!GitHub_Desktop<branch>`); using the same mark would let a stash made in either show in both. Untracked files are stashed too (`stash push --include-untracked`). Before the first commit there's nothing to stash against, and Git says so
    - **Stash all changes,** from the changes list's right-click menu, not offered while a file is conflicted. Stashing again on a branch that has one asks first. Desktop then drops the old stash, which loses its changes for good: whether Fernanda drops it or keeps both is settled when it's built
    - **Switching with changes** asks, as Desktop does: leave the changes on the current branch (stash, then switch), or bring them to the new one (stash, switch, restore). A pull that Git refuses because it would change a file with changes offers "Stash changes and continue", as Desktop's does
    - **The stash** shows as a "Stashed changes" row below the changes list while the branch has one. It opens a tab with its files and their diffs, as a commit's tab shows them, with Restore (`stash pop`) and Discard (`stash drop`, asking first). Restore is refused while there are uncommitted changes, as Desktop refuses it. One that conflicts, possible when the branch has moved since, waits for merging (4)'s conflict handling; until then it's undone and the stash kept
    - Stashing and restoring rewrite files, so they're held as a switch or a pull is
4. **Merging.** A pull only fast-forwards, so committing on two computers leaves the next pull refused. A pull would merge branches that have both moved on, as GitHub Desktop's does, and another branch could be merged into the current one. A merge that conflicts lists the conflicted files with each side's version to keep, and commits once they're settled. Git refuses to commit only some files mid-merge, so the commit box would say so
5. **Discarding more, and managing branches**
    - **Discard all changes,** from the changes list's right-click menu, as Desktop's Discard All Changes, and **discard the selected files** once the list takes more than one selection. One question for all of them, with Cancel the default, as one file's is. Each file goes as one file's discard goes: a file in the last commit has its copy trashed and is restored, one not in it is trashed itself. Every file is trashed before Git restores any, and if one can't be, nothing is discarded. Git then restores them in one step (`restore --pathspec-from-file`), held as a single file's restore is. Conflicted files are left out
    - **Rename and delete a branch,** from the branch menu. Deleting asks first, and is refused for the current branch
6. **Smaller conveniences.** A filter in the branch list. View on GitHub on a commit, a browser link made from the remote's address. Ignore a file or every file with its extension, from the changes list's right-click menu, by adding it to `.gitignore`. Copy a changed file's path, and show it in the system's file manager
7. **Starting a repository for a vault that isn't in one** (`git init`), from the Git settings page, so the Git page isn't only for vaults already in a repository. Publishing it is then the sync button's Publish, once a remote is added; making a repository on GitHub needs a sign-in, which is left to GitHub's website
8. **Committing part of a file**, by clicking lines or hunks in the diff's gutter as in GitHub Desktop, and showing more of the file around a change. Last, since a paragraph of prose is one line, so a hunk is already about as small as a change gets

### Releases

- A signed Windows installer, through the SignPath Foundation's free code signing for open source. The installer already shows the privacy statement and offers to turn off the update check, as its conditions ask. Once the project is accepted, the README and every release page need a section titled "Code signing policy", naming who signs and the team's roles and linking the privacy statement, and the release notes' warning about an unsigned build goes. Until then, that section would claim something untrue

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
- **Restoring diff tabs, and Find in a diff.** Diff tabs aren't restored when a vault opens again, since a diff is read from Git and may have changed or gone by then. A diff tab could be saved in the workspace as its file and scroll position, and reopened if the file still has changes. Find works in the editor only; a diff tab could take Ctrl+F too. Worth building if diff tabs prove to be something kept open
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
- **The focus timer in one window only.** Every vault window shows its own timer's button. It could show only in the window used last, following activation through that window's pop-outs. Worth building if the button in several windows proves distracting
- **One focus timer for the whole app.** Each vault window runs its own timer, so two can run at once, and moving to another vault's window doesn't carry a running period with it. Starting one timer could reset the others, but that would make several timers act as one without being one. If one timer is what's wanted, it should be one timer, with the question of whose lengths it uses settled. Worth considering if two timers running at once proves confusing
- **More from the focus timer:** starting the next period on its own, a sound when a period ends (which needs Qt Multimedia), and a system notification. Each is a setting Obsidian's timer plugins have. Worth building if the taskbar flash proves too easy to miss
- **The focus timer's time over the text:** a small box at the top of the pane area showing the countdown, which clicks pass through. The ring shows the time only roughly, on purpose. Worth building if that proves too little. A box floating over the whole desktop was considered and set aside: Wayland can't place one or keep it on top, macOS hides one while Fernanda isn't active, and on Windows it covers other programs
- **Commands for the focus timer** (start or pause, skip, reset), as `timer.` actions, once there is a command palette to reach them from
- **Two tools from the original Fernanda:** keeping a window on top of other programs, and keeping the computer awake while writing. Staying awake needs code for each platform
- **Importing Word documents** as plain text into a vault folder, as Hearth did. Hearth's converter needs work before it's brought over: it takes field codes and deleted tracked changes for text, writes text boxes twice, and drops tabs and line breaks. It needs a zip reader (miniz, under the MIT license). Footnotes, list numbering, and italics would be lost in plain text. Hearth's RTF import could follow the same path. Worth building if drafts often come in from Word

## Not planned

- **GitHub account features in the Git page.** Viewing pull requests and their checks, notifications, issues, warnings about protected branches and a repository's rules, making a repository on GitHub, and AI-written commit messages all need Fernanda signed in to GitHub's servers, which Git's own sign-in doesn't cover. They would also break the promise that only the sync button reaches the network. Create pull request opens GitHub's own page instead, and the rest is there too
- **Rewriting history:** reordering, squashing, cherry-picking, rebasing, and resetting to an older commit. They tidy shared code history and are the easiest way to lose work, and a writer has little reason to reshape past commits. Undoing the last commit and amending cover the common mistakes, and reverting undoes an old commit without rewriting anything
- **A side-by-side diff, hiding whitespace, and a diff tab size.** A paragraph is one line, so two half-width columns would wrap each into a tall narrow block, and hiding whitespace could hide a real change in prose, such as a double space. A diff has no tabs to size: it's in the vault's font
- **Checking out an old commit.** It leaves the whole repository on no branch ("detached HEAD"), an easy state to get lost in. A file's own history shows an old version of one file, which is what a writer usually wants
- **A list of repositories, cloning, and adding repositories,** as GitHub Desktop has. Fernanda opens vaults, and the repository is the one the vault is in. A clone is a folder to open as a vault
- **Co-authors, skipping commit hooks, forks, and settings for large files (Git LFS) or worktrees.** Rare in solo writing. Git handles large files and worktrees without any interface, and Fernanda already follows a worktree's `.git` file

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
- **Fetching on a timer.** GitHub Desktop fetches every hour. Fernanda's Git page contacts the repository's remote only when its sync button is clicked, so opening a vault never reaches the network on its own. The sync button's tooltip says when the repository last fetched, so a stale page is visible
- **Committing or discarding a folder that holds its own repository.** Git lists one as a single entry without looking inside. Committing it records only a pointer to that repository's commit, so a clone gets an empty folder, and discarding it would trash the repository whole, its history included. The Git page leaves it out
- **Diffs in a monospace font.** A diff is in the vault's text font, so prose reads as it does in the editor
- **Telling files apart by following symlinks.** A file is named by the path it was reached by, so a folder linked into a vault stays part of that vault. Following links would name it by its target, outside the vault, and break it there. A file reachable two ways through a link is two documents; Obsidian asks for link targets to be kept apart in the same way. Only a vault's own folder is followed through a link, so one folder opened two ways is one vault
- **Switching language without a restart.** Qt can swap translations while running, but every widget would have to set its text again when told, and Fernanda sets its text once, as each widget is built. Every class with text would need a second path for it, and a string missed in one would leave a window half in each language. Relaunch, beside the Language setting, does the job as Obsidian's does

## Open questions

- **The word "vault".** The per-project container might become a Project or a Notebook, and the shared one simply the Vault
- **Autosave timing.** One second after typing stops and three seconds at most are working defaults, not settled numbers
- **Sort order with hidden extensions.** Files sort by full name, so `a b.md` comes before `a.md` and the tree reads "a b", then "a". Whether to sort by the name shown is undecided
- **Markdown rendering.** Not ruled out, and probably not the way Obsidian does it. What form it would take is undecided: styling in place with the markup left visible, a separate read-only preview, or something else. Whatever it is, the file stays plain text and is written back unchanged
- **Folder links in the file tree.** The tree shows and expands symlinked folders; Go to File skips them. One of the two should change
