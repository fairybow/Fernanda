# TODO

Bugs and code chores. Features that aren't built yet are in [Future.md](Future.md); conventions are in [CodeStyle.md](CodeStyle.md).

## Bugs

- [ ] VaultSwitcher's menu should open upward
- [ ] "Go to file" on a new tab in a pop-out used to activate the main window and never return focus to the pop-out. Likely fixed now that the switcher opens over the page's own window: verify
- [ ] Pop-out Z-order isn't restored: a pop-out that was in front of its vault window comes back behind it. Vault windows restore in the right order among themselves, so this may be pop-outs only, and may not be fixable (which would be fine)
- [ ] Opening the file that is already in the active, unpinned tab should do nothing
- [ ] Minor: dragging a file quickly from near its row's bottom edge moves the selection highlight down to the next row, though that row isn't selected
- [x] A file inside two open vaults (nested vaults, or a project vault containing the Common Vault) had a buffer in each. Each autosave came back to the other as an outside change and reloaded as an undo step, and a rename, move, or delete in one vault discarded the other's buffer, losing up to three seconds of typing. Buffers now live in one `BufferStore` for every vault and window, keyed by `PathKey` (see "One store for every buffer" in [Architecture.md](Architecture.md))
- [x] On Windows, renaming, moving, or deleting a folder failed when another vault's window had expanded a folder inside it, since that tree's watch is an open handle; and once that worked, the other window's tree collapsed the moved folder. The store now releases and updates every vault's tree around a folder operation
- [x] Names like COM¹ or LPT³ were allowed, though Windows reads the superscript digits one, two, and three as digits and reserves those names as ports in every folder. `UserTextEntry` now refuses them, and `UserTextEntryTest` covers the name rules
- [x] Opening or closing the Common Vault drawer left hover where the cursor had been: the header kept its wash after moving away, and sometimes the common tree's first row stayed lit. Qt marks what's under the mouse only when the mouse moves, and a toggle moves the header and both trees under a still cursor. One tick after a toggle, the drawer now corrects its header's under-mouse flag, and both trees hover the row actually under the cursor (`VaultTreeView::syncHover`)
- [x] Typing a space after punctuation that had gone before a step's space gave two spaces: `(word  , then` became `(word),  then`, and Enter there left a space at the end of the line. A space typed straight after that punctuation is now taken as the one already there, and Enter drops it
- [x] Closing a window with tabs ran each page's `destroyed` handler on a `TabPaneLeaf` whose destructor had already finished, reading a member of an object no longer of its type (undefined behavior, harmless in practice, found by UndefinedBehaviorSanitizer). The leaf now cuts its pages' connections in its destructor. CI runs the tests under the sanitizers too
- [x] Opening a text file added an invisible undo step and saved the file unchanged. Undoing past your own edits then replaced the whole text with itself, and the cursor jumped to the end of the file. The editor set its document's margin after the document was shared with the buffer, and Qt reports a format change as a change to all the text. The margin is now set before the document is shared, and the buffer ignores a change that leaves the text as it was
- [x] A selection handle at the start or end of a line was cut off by the side of the text area. The handles are painted into the viewport, and the text was only Qt's default 4 px document margin from its edge. Part of the side padding is now that margin (`TEXT_DOCUMENT_MARGIN`), wide enough for a handle
- [x] The Editor settings page's "Open folder" button was the dialog's default, so Enter on that page opened the dictionaries folder. A `QDialog` makes a push button auto-default unless told otherwise; `SettingsPage::addRow` now turns that off for every button in a row
- [x] The "Options" heading in the settings dialog took the hover wash, as if it could be chosen. An item view marks the row under the mouse as hovered even when it's disabled, so `Style` now gives a disabled row no hover wash
- [x] Clicking "Check for updates" moved focus to the next control. The button was disabled while a check ran, and a disabled widget gives up focus. It now stays enabled: asking again starts no second check
- [x] A combo box's drop-down list showed two edges, a rounded panel inside a square window, and was narrower than its box, cutting off "System default". `Style` now draws the list's window as a menu's (see "The style" in [Architecture.md](Architecture.md))
- [x] A combo box's longest value could lose its last character ("English (United States)" at 125%). Qt left no room after the text once the scroll bars were made thin; `Style` adds `STYLE_FIELD_H_PADDING`
- [x] At 100% scale, the tab close X and the tree's chevrons were lopsided. At 12 pixels Lucide's stroke is 1 pixel, and Qt draws a line that thin with a stroker that misplaces it by half a pixel. `Glyph::render` now draws larger and scales down (see "Following the palette" in [Architecture.md](Architecture.md))
- [x] Translations didn't work. Nothing installed a `QTranslator`, and both `.ts` files were empty, since the `update_translations` target that fills them had never been run. `Translations` now installs them at launch, and the `.ts` files are filled
- [x] A window's current tab didn't have focus after restore. The tab pages were focused while the window was still hidden, which a widget can't take, so `BaseWindow` now focuses the active page on its first show
- [x] After restore, the first click on a tab didn't focus its widget. Focus was moved on `QTabBar::tabBarClicked`, which comes before the bar changes the current tab, so the tab switch then took it. `TabBar` now reports a press after the bar has handled it
- [x] A dropped tab's window wasn't activated, so its page didn't get the keyboard. The tree now raises and activates its window after a drop
- [x] The text editor's context menu opened to the left of the pointer, by the width of the line-number gutter and the margin. Qt gives a scroll area's menu position in its viewport's coordinates, and it was mapped from the editor's
- [x] The spelling language in `settings.json` was used as a file name without being checked, so a hand-edited `../x` named a dictionary outside the dictionaries folder. `SpellCheckers` now takes only a single name, and treats anything else as not installed
- [x] A path with a `..` segment passed `Vault::contains`, so a rename or move could take an entry out of the vault, and `Vault::openModel` opened a `../` or absolute path from a hand-edited `workspace.json`. Paths are now normalized into keys, and one that climbs out of the vault is refused (see "Identity" in [Architecture.md](Architecture.md))

## Rough edges

- [ ] The licenses dialog's Qt entry links to Qt's list of the libraries Qt itself bundles (PDFium in Qt PDF, among others) rather than showing their notices. Shipping their texts would mean collecting them from the Qt build in use
- [ ] Tabs can be dragged past the left or right end of the tab bar without starting a drag; may want to clamp them
- [ ] No auto-scroll while dragging a tab in an overflowing bar (close to impossible with Qt)
- [ ] Under the "C" locale (a Linux session with no locale set), the file tree doesn't read a number in a name as a number: "chapter 10" sorts before "chapter 2"
- [x] A vault whose root path was not plain (a trailing separator, a `.` or `..` segment) contained nothing by `Vault::contains`, so every create, rename, move, and delete in it was refused. Roots and every path under them are now normalized into keys
- [ ] The known-vaults list can show one folder twice: a vault once opened by a symlinked path, and the same folder by its resolved path, which is what `App` records now
- [ ] A buffer kept after its last view's save failed is shown by no window, so no window close retries it. Focus loss and quit do
- [ ] In a release build, `PrimeDocument` notices a view document out of step with the prime only when their lengths differ. An edit applied at the wrong position keeps the lengths equal and goes unnoticed until they diverge. A full comparison when a save reads the text would catch it before it reaches disk
- [ ] When `PrimeDocument` resets a view that was out of step, that view's cursor goes to the start of the file
- [ ] A pane whose files were all deleted between sessions is restored as an empty pane inside a split, where in use a pane closes with its last tab. `TabPaneTree::restore` builds the saved shape and prunes nothing. A split left with one child (by this, or by a hand-edited `workspace.json`) is kept the same way
- [ ] On Windows, deleting a folder that a tree has listed, from outside Fernanda, logs Qt's "FindNextChangeNotification failed ... (Access is denied.)" once per watched folder. Harmless: the rows still go
- [ ] When `Vault::addToDictionary` can't write `dictionary.txt`, the failure is only logged: the word is accepted until the vault closes and is gone the next time it opens, with nothing shown
- [ ] A bundled dictionary is copied to the dictionaries folder once and never replaced, so a newer one in a later release doesn't reach an existing install
- [ ] If a file reloads from disk while the editor's context menu is open, a suggestion chosen afterward is inserted wherever the word's cursor ended up, not over the word
- [ ] Menus and drop-down lists are drawn rounded in a see-through window, which needs a compositor on Linux. Without one, the corners may show dark. `Style` could draw them square there instead
- [ ] A web address or a file path is spellchecked piece by piece, so "github" and "com" are marked
- [ ] Unverified: text in decomposed Unicode form (a letter followed by a separate combining accent) is probably marked as misspelled against a dictionary that stores the single-character form. If so, normalize a word before it is checked

## Before release

- [ ] Review the Spanish translations in Qt Linguist and mark each done. They are a draft, left marked unfinished
- [x] `LogView` opens on every launch, for testing. Put it behind its command-line flag
- [x] `CHANGELOG.md` has only a placeholder release entry. Write the real notes before tagging
- [ ] Test the OS logout save path (`App::onCommitDataRequest_`)
- [x] Need the remaining Edit menu and text editor context menu options (cut, copy, paste, select all, delete)
- [x] A licenses dialog crediting what Fernanda bundles: Qt, Hunspell, the en_US dictionary, the fonts, and Lucide

## Code

- [ ] Verify the `PrimeDocument` header, and rename it
- [ ] Rename `BaseWindow`'s protected members that have a trailing underscore
- [ ] Bring `MenuBuilder` over from Hearth (maybe into Coco)?
- [x] Decide on `private slots:`. It isn't used consistently, and using it properly would break up sections that are organized by purpose. Either remove the keyword everywhere or sort slots into it everywhere
- [ ] `AbstractFileModel` has `notifyX()` functions that only emit a signal. Could they just be signals (perhaps named `notify...`)?
- [ ] Decide whether static helpers go before other functions, per access level
- [ ] A shared `TabPage` base for tab pages, holding title and pin state as typed members in place of the window title and a dynamic property. It would trade the leaf's property reads for a cast. Worth doing once a third such value appears
- [x] Run the tests on every push, for Fernanda and for Coco's smoke test. `release.yml` runs only on a version tag, and now builds the tests too: pass `-DAPP_BUILD_TESTS=OFF` there
- [ ] `AbstractFileView` has `setSpellChecker` and `setAcceptedWords`, which only `TextFileView` implements, and `VaultWindow::makeView_` already holds the `TextFileView` when it makes one. Wiring spelling there would take both off the base, and `hunspell.hxx` out of every view's includes
- [ ] Two tests decide whether a palette is dark: `Style` reads the window color, `TextEditor` the page color. Settle one rule with themes, which need it too. `views/` can't include `ui/Style.h`, so a shared one needs a home both can reach
- [ ] CI has no Windows job. Qt 6.11 installs on Windows only through the official installer, which needs a Qt account (see `release.yml`). Add the job once the open-source installer handles it

## Untested

What the tests in `Fernanda/tests/` don't reach.

- [ ] `Updater`'s request to GitHub and its handling of the reply. The decision it makes from the reply is tested (`UpdateCheckTest`)
- [ ] A failed write: the buffer stays modified, and `BufferStore::flush` names the file. There is no portable way to make a write fail on demand
- [ ] The autosave debounce starting again on each edit. Proving it needs timing tight enough to fail on a slow machine
- [ ] A successful `Vault::moveToTrash`. It would put a file in the real system trash on every run. The store's part after the trash (`BufferStore::applyRemoval`) is tested
- [ ] That releasing another vault's tree watches lets a folder operation through on Windows. Linux and macOS watchers don't lock folders, so the tests can show the watches are given back after a failure, not that the release unblocks anything
- [ ] Which vault a window takes a file to be in (`VaultWindow::vaultOf_`): the guest mark, the workspace labels, and Go to File listing a file once
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
