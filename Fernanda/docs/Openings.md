# Openings (Draft)

**General notes:**
- There is only ever 1 Fernanda instance; relaunch is guarded and new args are passed to the current instance
- There is only ever 1 Notepad instance (created at startup); there can be multiple (or zero) Notebook instances
- Notepad exists for the application lifetime, even with no windows
- Files can be opened multiple times (creating multiple views on the same model)
- Notepad works on OS filesystem paths; Notebook works on extracted archive files with UUID-based naming
- Tab titles in Notepad derive from file paths; tab titles in Notebook use FnxModel display names
- FileService tracks models by path and reuses existing models when opening the same file twice

## Application startup

- StartCop guard prevents multiple instances; new args forwarded to existing instance
- Application creates directories, initializes logging, creates Notepad
- Notepad opens with initial window
- Notebooks opened from args/session (future) or user action

## New/Open Notebook

- File dialog for archive selection or creation
- Application creates Notebook instance, adds to tracking list
- Notebook creates temp working directory
- If new archive: creates empty working directory structure
- If existing archive: extracts contents to working directory
- Loads FnxModel from Model.xml
- Opens with initial window

## New window

- WindowService creates window, adds to tracking
- Emits `windowCreated` event
- ViewService responds by adding TabWidget
- TreeViewService responds by adding TreeView dock with workspace's model
- Other components can respond to this signal and add to the window

## New tab

Triggered by menu command or TabWidget's add-tab button.

**Notepad:**
- Creates new off-disk model (empty, unsaved file)
- Opens view on the new model

**Notebook:**
- Generates UUID, creates empty file in working directory
- Adds file element to DOM (marks archive modified)
- Opens view on the new file with display name as title

## Open file

Triggered by menu, tree view double-click, or internal command.

**FileService:**
- If model already exists for path: reuses it (new view on existing model)
- If new path: creates model, reads content, registers for reuse

**ViewService:**
- Creates view for the model
- Adds tab with title/tooltip from model metadata
- Tracks view count per model

**Notepad-specific:**
- Path interceptor checks for `.fnx` files and redirects to Notebook
- Tree view uses filesystem paths directly

**Notebook-specific:**
- Tree view translates FnxModel index to working directory path + display name
- Title override set from FnxModel's user-facing name (not UUID filename)

## Import file (Notebook)

- File dialog for source file selection
- Copies files to working directory with UUID names
- Adds file elements to DOM with original filenames as display names
- Opens views on imported files

## Open Notepad (from Notebook)

- If Notepad has windows: activates most recent
- If no windows: creates new Notepad window

## Tab titles and metadata

**Notepad:**
- Title from filename (or "Untitled" counter if unsaved)
- Tooltip shows full path

**Notebook:**
- Title from FnxModel display name (user-editable)
- Renaming in tree view updates tab titles via metadata change

## Model lifecycle

- FileService owns models; same path = same model reused
- ViewService tracks view count per model
- Notepad deletes models when view count reaches 0
- Notebook keeps models alive (persist changes, support undo/redo until Notebook closed)
