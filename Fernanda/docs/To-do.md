# To-do

## General
- [ ] Remove blank switch options
- [ ] Double-up on identical switch cases
- [ ] Export All vs Export Selected/Single
- [ ] Ability to "mark" files as countable/compilable, for both exporting a draft and calculating totals
- [ ] Installer script needs to detect if Fernanda is running
- [ ] Moveable documents folder
- [ ] Popup if temps exist on start (but would need to reload last project first, and then check if temps exist?)
- [ ] Move the startup `ColorBar` singleshot to a different window event? `showEvent` doesn't seem to be working for this
- [ ] Move the `nullptr` checks closer to the functions that create the `nullptr`, if possible (viz., don't wait till the info is sent to `Story` to cancel a nullptr from `Pane renameItem()`)
- [ ] Replace certain bool args with enums for descriptive actions taken (like "finalize" in `dom->renames()`)
- [ ] Custom highlight colors for line highlight and `Delegate` highlight (like with cursor)?
- [ ] Add border variables to stylesheets--for the most part, these can just be `transparent`, but allows for user to decide in customs
- [ ] Hold current story file from being edited while Fernanda is open? (Not sure this is possible. Plus, will need a way to temporarily request access for `Archiver` for editing)
- [ ] Export to PDF using Markdown or Fountain
- [ ] Is there a way to link swatches?
- [ ] Temp save before cut?
- [ ] Delete trailing spaces on save
- [x] ~~Export selected / all, partly done~~
- [x] ~~Total word count (export-marked or all) - partly done~~
- [x] ~~Combine all the messagebox functions into something that generates most of it as a default config~~
- [x] ~~Keep screen awake?~~
- [x] ~~Mutex for running only one instance (sorta)~~
- [x] ~~Add dialogs to free functions? (added class)~~
- [x] ~~Change/remove all "`metaDoc`" names~~
- [x] ~~Make Pallete.txt an .md~~
- [x] ~~Fonts and credits to Readme~~
- [x] ~~File path as opening arg~~
- [x] ~~Prompt to save or abandon changes (and then clear temp folder either way) on switching projects (opening if a project already exists)~~
- [x] ~~Show open `.story` in window title?~~
- [x] ~~Move closing popup to an `unsavedChanges()`, which should also do the temp clearing, with clearing self dependent on a bool that will also toggle the message (are you sure you want to close, vs. are you sure you want to switch stories)~~
- [x] ~~Handle cutting of multiple items (deleting folder or parent file), activeKey of cut file isn't being nullptred when parent directory is cut, for example, and then all contained cut file keys remain in edits list~~
- [x] ~~Convert paths to `std::filesystem::path`~~
- [x] ~~Activate dev menu via command arg~~
- [x] ~~Redo Dependency Tree~~

## Code Guidelines
- [ ] One-line implementations to headers, bottom of proper class section / namespace (before templates); if header-only, bottom of section/namespace if possible
- [ ] Header-onlys =< ~100-150 lines
- [ ] `pascalCase`: member/global variables, function names, and parameters
- [ ] `snake_case`: local-variables
- [ ] `CamelCase`: enums, classes, structs
- [ ] No uncommon/unhelpful abbreviations
- [ ] Avoid macros
- [ ] Names functionally descriptive, e.g. `editor->toggle(checked, Editor::Has::Scrolls);` or `pane->navigate(Pane::Go::Next);`
- [ ] `#include` block order: me, you, them:
```
#include "Path.h" // mine

#include <bit7z/bit7z.hpp>
#include <QFile>
#include <Windows.h> //  yours

#include <ctime>
#include <utility> // theirs
```
- [ ] Keep space around compiler instructions
- [ ] Prefer ternary operator
- [ ] Prefer early return

### Known issues
- [ ] Preview "resets" MainWindow on startup (if toggled from off to on) - [QTBUG-110116](https://bugreports.qt.io/browse/QTBUG-110116)
- [ ] Preview continues to use memory after being toggled off
- [ ] Windows scale > 100% negates the effects of `setTextCursor(0)`
- [ ] AOT toggling affects stored window position setting (which, when toggled while maximized makes unmaximizing not change the window size)
- [ ] Program will sometimes start with artifacting / color halos around pane icons (like below). Toggling window theme off and on seems to fix it?
- [ ] Cycling fonts and themes, and then forcing repaint by editing open document (to be marked dirty or clean) is causing outlines (often blue, sometimes red) to appear around the icons in Pane (Delegate)
- [ ] ^ Cycling themes not needed. Appears to be best reproducable by cycling editor fonts quickly and then typing (with light window theme)
- [ ] ^ The issue may be largely solved, but cycling from Dark to Light does produce a dark blue outline around Open Folder and File icons (but not Closed Folder, from what I could tell?)
- [x] ~~Total Counts will not be accurate if no temp has yet been made of edited file (this can happen when editing between first click and first auto-save)~~

## Classes

### Archiver[^1]
- [ ] Switch to streams (i.e., for `add()` and `create()`)
- [ ] Rename/refactor functions to better reflect their roles / be more descriptive
- [x] ~~Saving after deleting an item that has children causes a crash in `rename()` (It might be just deleting things that have moved. They may be receiving the non-existant rename path instead of relative_path?)~~

### Dom[^1]
- [x] ~~Probably should combine `elements()` and `elementsByAttribute()`~~

### Editor / PlainTextEdit / LineNumberArea
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
- [ ] Dim opacity when non-auto-count?
- [x] ~~Deactivate for extremely large strings~~
- [x] ~~^ Check character count at open, set Indicator, and then toggle auto counting based on character length (add a refresh symbol)~~

### KeyFilter
- [ ] Auto-format ellipses
- [ ] Auto-delete spaces before punctuation
- [ ] Quote wrapping does not account for non-American punctuation/quote order
- [ ] Add guillemets (both versions)?
- [ ] On enter-press if the next `char` is closing parenthesis, quote, etc., then apply a skip before new line?
- [x] ~~Avoid passing entire document lol~~

### MainWindow
- [ ] Instead of `activeStory` check for Save option, tie it to signal to show/hide story menu and make disabled
- [ ] Separate menu into its own class?
- [ ] Auto-hide menu option
- [ ] Auto-hide scrollbar
- [x] ~~For exporting, call `story->name<StdFsPath>()` from outside class and fill in, rather than appending it in Story, to support renamed exporting (probably not for directory)~~
- [x] ~~Rename menu locals to reflect the alphabetical order of the items~~
- [x] ~~Rename `viewToggles()` to match~~
- [x] ~~Order the items in menus Window and Editor (and General, if applicable)~~
- [x] ~~Add path to user data folder to the sample themes popup~~
- [x] ~~^ Or a button to open UD folder?~~
- [x] ~~Open UD folders from Help menu~~
- [x] ~~Make Dev menu instead of Status Bar items~~
- [x] ~~I don't think `toggleWidget()` or `toggleGlobals()` are slots~~
- [x] ~~View menu function is gross and bad~~
- [x] ~~Combine two cursor toggles into one submenu~~
- [x] ~~Toggle-specific menu~~

### Pane
- [ ] Persist selected-item highlight between saves/moves
- [ ] `persistentEditor()` or an input dialog for `rename()`? (`openPersistentEditor(itemModel->indexFromItem(temp_item));`)
- [x] ~~Style scrollbars~~
- [x] ~~Nav / wrap-around function slightly busted~~

### PaneDelegate
- [ ] For `paint()` and `updateEditorGeometry()` - these override public functions, and idk if that matters
- [ ] `updateEditorGeometry()` is currently unused
- [ ] May need to remove and re-add icons / labels. They seem to be doubling up sometimes--visible r/g/b or pink outlines?

### Popup
- [ ] Ensure popups happen on the same monitor...

### Preview
- [ ] Need a default size to open to from toggle if not set (-1)
- [ ] Toggling preview on will have to trigger the above, because if never toggled on, it will toggle into existence with no width
- [ ] Ink support
- [ ] Print to PDF context menu
- [x] ~~While dragging a collapsed panel open will not override the saved size, dragging it closed will (remove line 149 probably and do storeWidths on mouse release, signalled by handle, instead...avoid saving periodically when dragging)~~
- [x] ~~Scrollbar should begin tied to position in editor. Can be moved independently but when snapped, scrolls with editor.~~
- [x] ~~Replace fountain CSS~~
- [x] ~~Minify fountain CSS~~

### Splitter[^1]
- [ ] On open, if you drag to collapse a widget, it will expand to default instead of saved size (is there a way to determine the percentage open needed to mimic the saved state from qbytearray?)
- [ ] Increase trigger area, slightly beyond visible boundaries
- [ ] Preview handle should animate to the left and not right
- [ ] Add ability to hover mouse for length of time and activate collapse/expand
- [x] ~~Toggle collapse/expand on click~~

### StartCop
- [ ] Accept args (on file click, prompt to save if needed and open new file; possibly also switch to dev mode)

### Story[^1]
- [ ] Total Counts variant for compiled documents only
- [ ] Only hold X amount of backups per file
- [x] ~~Probably don't need to keep full file path for root~~
- [x] ~~Cutting a file can result in a temp save of cut file appearing in root of `.story` on save (possibly fixed by clearing activeKey in Story on file cut)~~
- [x] ~~Cutting a file can result in error in edited keys list~~
- [x] ~~`devGetEditedKeys()` deleted keys currently remain.~~

### Tools
- [x] ~~Actual countdown visible for timer~~

## Namespaces

### Path
- [x] ~~May not need `makePosix()`; Bit7z only accepts `\\` paths for searching~~

### Resource
- [ ] DataPair should be split up. A lot of instances' lhs-es aren't even paths.
- [x] ~~There surely must be a smarter way to incorporate `.otf` into the fonts RC list (applies to MainWindow, too)~~
- [x] ~~Convert to `std::filesystem::path`~~

### UserData
- [ ] Split up the enums with namespaces for editor, data, and window (removing arg requirement for a IniGroup) (Unclear on how to do this at the moment)
- [x] ~~Enums for group (and possibly value)~~

[^1]: This class is a huge mess
