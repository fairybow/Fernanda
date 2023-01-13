# To-Do

## General
- [ ] Installer script needs to detect if Fernanda is running
- [ ] Remove all non-standard abbreviations (like "cur" for current)
- [ ] Moveable documents folder
- [ ] Popup if temps exist on start (but would need to reload last project first, and then check if temps exist?)
- [ ] Move the startup `ColorBar` singleshot to a different window event? `showEvent` doesn't seem to be working for this
- [ ] Move the `nullptr` checks closer to the functions that create the `nullptr`, if possible (viz., don't wait till the info is sent to `Story` to cancel a nullptr from `Pane renameItem()`)
- [ ] Combine all the messagebox functions into something that generates most of it as a default config
- [ ] Replace certain bool args with enums for descriptive actions taken (like "finalize" in `dom->renames()`)
- [ ] Alphabetize enums and generally clean up headers
- [ ] Custom highlight colors for line highlight and `Delegate` highlight (like with cursor)?
- [ ] Remove the regular-scrolls-toggled-off for no theme; can just style appropriately in base
- [ ] Add border variables to stylesheets--for the most part, these can just be `transparent`, but allows for user to decide in customs
- [ ] Hold current story file from being edited while Fernanda is open? (Not sure this is possible. Plus, will need a way to temporarily request access for `Archiver` for editing)
- [ ] Export selected / all
- [ ] Export to PDF using Markdown or Fountain
- [ ] Total word count (export-marked or all)
- [ ] Is there a way to link swatches?
- [ ] Implement QStringLiterals where possible
- [ ] Temp save before cut?
- [ ] Delete trailing spaces on save
- [x] ~~Mutex for running only one instance (sorta)~~
- [x] ~~Add dialogs to free functions? (added class)~~
- [x] ~~Change/remove all "`metaDoc`" names~~
- [x] ~~Make Pallete.txt an .md~~
- [x] ~~Fonts and credits to Readme~~
- [x] ~~File path as opening arg~~
- [x] ~~Prompt to save or abandon changes (and then clear temp folder either way) on switching projects (opening if a project already exists)~~
- [x] ~~Show open `.story` in window title?~~
- [x] ~~Move closing popup to an `unsavedChanges()`, which should also do the temp clearing, with clearing self dependent on a bool that will also toggle the message (are you sure you want to close, vs. are you sure you want to switch stories)~~
- [x] ~~Handle cutting of multiple items (deleting folder or parent file), activeKey of cut file isn't being nullptred when parent dir is cut, for example, and then all contained cut file keys remain in edits list~~
- [x] ~~Convert paths to `std::filesystem::path`~~
- [x] ~~Activate dev menu via command arg~~
- [x] ~~Redo Dependency Tree~~

### Known issues
- [ ] Known issue: Windows scale > 100% negates the effects of `setTextCursor(0)`
- [ ] Known issue: Cycling fonts and themes, and then forcing repaint by editing open document (to be marked dirty or clean) is causing outlines (often blue, sometimes red) to appear around the icons in Pane (Delegate)
- [ ] ^ Cycling themes not needed. Appears to be best reproducable by cycling editor fonts quickly and then typing (with light window theme)
- [ ] ^ The issue may be largely solved, but cycling from Dark to Light does produce a dark blue outline around Open Folder and File icons (but not Closed Folder, from what I could tell?)

## Classes

### Archiver
- [ ] Switch to streams (i.e., for `add()` and `create()`)
- [ ] Rename/refactor functions to better reflect their roles / be more descriptive
- [x] ~~Saving after deleting an item that has children causes a crash in `rename()` (It might be just deleting things that have moved. They may be receiving the non-existant rename path instead of rel_path?)~~

### Dom
- [ ] Mark files as exportable

### Fernanda (MainWindow)

- [ ] Separate menu into its own class?
- [ ] Auto-hide menu option
- [ ] Auto-hide scrollbar
- [ ] Rename menu locals to reflect the alphabetical order of the items
- [ ] Rename `viewToggles()` to match
- [ ] What can be connected to the `storyOpened()`/`storyClosed()` signals?
- [x] ~~Order the items in menus Window and Editor (and General, if applicable)~~
- [x] ~~Add path to user data folder to the sample themes popup~~
- [x] ~~^ Or a button to open UD folder?~~
- [x] ~~Open UD folders from Help menu~~
- [x] ~~Make Dev menu instead of Status Bar items~~
- [x] ~~I don't think `toggleWidget()` or `toggleGlobals()` are slots~~
- [x] ~~View menu function is gross and bad~~
- [x] ~~Combine two cursor toggles into one submenu~~
- [x] ~~Toggle-specific menu~~

### Editor / LineNumberArea
- [ ] It is not clear to me that `updateLineNumberAreaWidth(int newBlockCount)` actually uses `newBlockCount` arg
- [ ] Save undo/redo stacks
- [ ] Editor spacing and kerning sliders
- [ ] Arrow keys follow block strangely
- [ ] `LineNumberArea` is not showing up initially on blank documents
- [ ] ^ I can't tell if the above is fixed. I have zero idea what would have fixed it, but it seems fixed.
- [ ] Wrap for parentheses and other closables
- [ ] If a filter was just applied, backspace should function as undo
- [ ] Make thin cursor change color when there's a selection?
- [x] ~~Avoid passing entire document for cursor underpaint lol~~
- [x] ~~Toggle chonky cursor vs regular~~
- [x] ~~Style horizontal scrollbar~~
- [x] ~~Toggle cursor blink~~
- [x] ~~Block cursor should default to char width, then to average char width~~
- [x] ~~Convert shadow to its own overlay, so that it won't interfere with LNA (like on Solarized themes)~~
- [x] ~~Get rid of I-bar cursor on locked editor~~
- [x] ~~"Memory creep" from not deleting fonts~~
- [x] ~~Remove annoying white block under cursor -_-~~

### Indicator
- [ ] Deactivate for extremely large strings / convert to non-automatic counting (refresh symbol)

### Keyfilter
- [ ] Auto-format ellipses
- [ ] Auto-delete spaces before punctuation
- [ ] Quote wrapping does not account for non-American punctuation/quote order
- [ ] Add guillemets (both versions)?
- [ ] On enter-press if the next `char` is closing parenthesis, quote, etc., then apply a skip before new line?
- [x] ~~Avoid passing entire document lol~~

### Pane
- [ ] Persist selected-item highlight between saves/moves
- [ ] `persistentEditor()` or an input dialog for `rename()`? (`openPersistentEditor(itemModel->indexFromItem(temp_item));`)
- [x] ~~Style scrollbars~~
- [x] ~~Nav / wrap-around function slightly busted~~

### PaneDelegate
- [ ] For `paint()` and `updateEditorGeometry()` - these override public functions, and idk if that matters
- [ ] `updateEditorGeometry()` is currently unused
- [ ] May need to remove and re-add icons / labels. They seem to be doubling up sometimes--visible r/g/b or pink outlines?

### StartCop

- [ ] Accept args (on file click, prompt to save if needed and open new file; possibly also switch to dev mode)

### Story

- [ ] Only hold X amount of backups per file
- [x] ~~Probably don't need to keep full file path for root~~
- [x] ~~Cutting a file can result in a temp save of cut file appearing in root of `.story` on save (possibly fixed by clearing activeKey in Story on file cut)~~
- [x] ~~Cutting a file can result in error in edited keys list~~
- [x] ~~`devGetEditedKeys()` deleted keys currently remain.~~

## Namespaces

### Path

- [x] ~~May not need `makePosix()`; Bit7z only accepts `\\` paths for searching~~

### Res

- [x] ~~There surely must be a smarter way to incorporate `.otf` into the fonts RC list (applies to MainWindow, too)~~
- [x] ~~Convert to `std::filesystem::path`~~

### Ud

- [ ] Split up the enums with namespaces for editor, data, and window (removing arg requirement for a ConfigGroup) (Unclear on how to do this at the moment)
- [x] ~~Enums for group (and possibly value)~~