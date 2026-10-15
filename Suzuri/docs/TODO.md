# TODO

Bugs and code chores. Features that aren't built yet are in [Future.md](Future.md); conventions are in [CodeStyle.md](CodeStyle.md).

## Bugs

- [ ] VaultSwitcher's menu should open upward
- [ ] "Go to file" on a new tab in a pop-out used to activate the main window and never return focus to the pop-out. Likely fixed now that the switcher opens over the page's own window: verify
- [ ] Pop-out Z-order isn't restored: a pop-out that was in front of its vault window comes back behind it. Vault windows restore in the right order among themselves, so this may be pop-outs only, and may not be fixable (which would be fine)
- [ ] Opening the file that is already in the active, unpinned tab should do nothing
- [ ] Minor: dragging a file quickly from near its row's bottom edge moves the selection highlight down to the next row, though that row isn't selected
- [ ] The "Options" heading in the settings dialog can be hovered or selected and probably shouldn't be
- [ ] Translations don't work. Nothing installs a `QTranslator`, so the embedded `.qm` files are never loaded, and both `.ts` files are still empty: the `update_translations` target that fills them has never been run
- [x] A window's current tab didn't have focus after restore. The tab pages were focused while the window was still hidden, which a widget can't take, so `BaseWindow` now focuses the active page on its first show
- [x] After restore, the first click on a tab didn't focus its widget. Focus was moved on `QTabBar::tabBarClicked`, which comes before the bar changes the current tab, so the tab switch then took it. `TabBar` now reports a press after the bar has handled it
- [x] A dropped tab's window wasn't activated, so its page didn't get the keyboard. The tree now raises and activates its window after a drop
- [x] The text editor's context menu opened to the left of the pointer, by the width of the line-number gutter and the margin. Qt gives a scroll area's menu position in its viewport's coordinates, and it was mapped from the editor's
- [x] The spelling language in `settings.json` was used as a file name without being checked, so a hand-edited `../x` named a dictionary outside the dictionaries folder. `SpellCheckers` now takes only a single name, and treats anything else as not installed
- [x] A path with a `..` segment passed `Vault::contains`, so a rename or move could take an entry out of the vault, and `Vault::openModel` opened a `../` or absolute path from a hand-edited `workspace.json`. `Vault` now takes only plain paths (see "Identity" in [Architecture.md](Architecture.md))

## Rough edges

- [ ] The licenses dialog's Qt entry links to Qt's list of the libraries Qt itself bundles (PDFium in Qt PDF, among others) rather than showing their notices. Shipping their texts would mean collecting them from the Qt build in use
- [ ] Tabs can be dragged past the left or right end of the tab bar without starting a drag; may want to clamp them
- [ ] No auto-scroll while dragging a tab in an overflowing bar (close to impossible with Qt)
- [ ] Under the "C" locale (a Linux session with no locale set), the file tree doesn't read a number in a name as a number: "chapter 10" sorts before "chapter 2"
- [ ] A vault whose root path is not plain (a trailing separator, a `.` or `..` segment) contains nothing by `Vault::contains`, so every create, rename, move, and delete in it is refused. The folder picker and the known-vaults list give plain paths; a root from the command line will need cleaning first
- [ ] In a release build, `PrimeDocument` notices a view document out of step with the prime only when their lengths differ. An edit applied at the wrong position keeps the lengths equal and goes unnoticed until they diverge. A full comparison when a save reads the text would catch it before it reaches disk
- [ ] When `PrimeDocument` resets a view that was out of step, that view's cursor goes to the start of the file
- [ ] A pane whose files were all deleted between sessions is restored as an empty pane inside a split, where in use a pane closes with its last tab. `TabPaneTree::restore` builds the saved shape and prunes nothing. A split left with one child (by this, or by a hand-edited `workspace.json`) is kept the same way
- [ ] On Windows, deleting a folder that a tree has listed, from outside Suzuri, logs Qt's "FindNextChangeNotification failed ... (Access is denied.)" once per watched folder. Harmless: the rows still go
- [ ] When `Vault::addToDictionary` can't write `dictionary.txt`, the failure is only logged: the word is accepted until the vault closes and is gone the next time it opens, with nothing shown
- [ ] A bundled dictionary is copied to the dictionaries folder once and never replaced, so a newer one in a later release doesn't reach an existing install
- [ ] If a file reloads from disk while the editor's context menu is open, a suggestion chosen afterward is inserted wherever the word's cursor ended up, not over the word
- [ ] Menus are drawn rounded in a see-through window, which needs a compositor on Linux. Without one, the corners may show dark. `Style` could draw square menus there instead
- [ ] A web address or a file path is spellchecked piece by piece, so "github" and "com" are marked
- [ ] Unverified: text in decomposed Unicode form (a letter followed by a separate combining accent) is probably marked as misspelled against a dictionary that stores the single-character form. If so, normalize a word before it is checked

## Before release

- [x] `LogView` opens on every launch, for testing. Put it behind its command-line flag
- [x] `CHANGELOG.md` has only a placeholder release entry. Write the real notes before tagging
- [ ] Test the OS logout save path (`App::onCommitDataRequest_`)
- [x] Need the remaining Edit menu and text editor context menu options (cut, copy, paste, select all, delete)
- [x] A licenses dialog crediting what Suzuri bundles: Qt, Hunspell, the en_US dictionary, the fonts, and Lucide

## Code

- [ ] Verify the `PrimeDocument` header, and rename it
- [ ] Rename `BaseWindow`'s protected members that have a trailing underscore
- [ ] Bring `MenuBuilder` over from Hearth (maybe into Coco)?
- [x] Decide on `private slots:`. It isn't used consistently, and using it properly would break up sections that are organized by purpose. Either remove the keyword everywhere or sort slots into it everywhere
- [ ] `AbstractFileModel` has `notifyX()` functions that only emit a signal. Could they just be signals (perhaps named `notify...`)?
- [ ] Decide whether static helpers go before other functions, per access level
- [ ] A shared `TabPage` base for tab pages, holding title and pin state as typed members in place of the window title and a dynamic property. It would trade the leaf's property reads for a cast. Worth doing once a third such value appears
- [x] Run the tests on every push, for Suzuri and for Coco's smoke test. `release.yml` runs only on a version tag, and now builds the tests too: pass `-DAPP_BUILD_TESTS=OFF` there
- [ ] `AbstractFileView` has `setSpellChecker` and `setAcceptedWords`, which only `TextFileView` implements, and `VaultWindow::makeView_` already holds the `TextFileView` when it makes one. Wiring spelling there would take both off the base, and `hunspell.hxx` out of every view's includes
- [ ] Two tests decide whether a palette is dark: `Style` reads the window color, `TextEditor` the page color. Settle one rule with themes, which need it too. `views/` can't include `ui/Style.h`, so a shared one needs a home both can reach
- [ ] CI has no Windows job. Qt 6.11 installs on Windows only through the official installer, which needs a Qt account (see `release.yml`). Add the job once the open-source installer handles it

## Untested

What the tests in `Suzuri/tests/` don't reach.

- [ ] `Updater`'s request to GitHub and its handling of the reply. The decision it makes from the reply is tested (`UpdateCheckTest`)
- [ ] A failed write: the buffer stays modified, and `Vault::flush` names the file. There is no portable way to make a write fail on demand
- [ ] The autosave debounce starting again on each edit. Proving it needs timing tight enough to fail on a slow machine
- [ ] A successful `Vault::moveToTrash`. It would put a file in the real system trash on every run
- [ ] `Vault::recreateRoot`
- [ ] A file that starts with two byte-order marks. On Qt 6.4 one is dropped on load, which the comment on the mark in `TextFileModel.h` says can't happen. Check it on the Qt in use
- [ ] `AppConfig`, `JsonIo`, `WorkspaceFile`, and everything in `views/` (apart from `TextSearch`) and `ui/`. The find bar and the way `TextFileView` drives a search are among them, as are the misspelling underlines and the spelling items in the editor's context menu. Considered for `TabPaneTree`'s save and restore and left out: a mistake there costs a layout, not text, and shows on the next launch

## Audits

Passes over the whole codebase against [CodeStyle.md](CodeStyle.md).

- [ ] Signals passed up parent chains
- [ ] Signals that only ever drive one slot in the same class
- [ ] Consistent terms: what is a pane, a leaf, a view, a page
- [ ] Naming and casing of types, functions, and members, including trailing underscores on private members
- [ ] Parent parameters not named for their parent type
- [ ] Widgets that start without a parent and are inserted into one later: name the parameter to say so
- [ ] Constructor work outside `setup_`
- [ ] `const`, `[[nodiscard]]`, `[[maybe_unused]]`
- [ ] `QDir` where a `Coco::Path` function would do
- [ ] Paths held or passed as `QString`
- [ ] Unneeded `Coco::Path(...)` conversions
- [ ] Includes not using the full path from `src/`
- [ ] Blank lines inside namespace braces
- [ ] Public functions nothing outside the class calls
- [ ] Order of sections within classes
- [ ] Helper classes used in one header that could move to its `Internal` namespace (`ZoomButton` in `ZoomControl.h`, for example)
- [ ] `TODO` comments in the source
