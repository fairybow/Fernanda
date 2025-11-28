# Closures

"Closing" a view means removing it from the TabWidget and deleting it.

"Closing" a model means deleting it.

"Modified archive" (Notebook only) means the working directory has changed from its original state or has no corresponding archive (is new)

Notepad never closes, except on quit.

Notebooks close when their last window closes.

There is only ever 1 Notepad. There can be multiple (or no) Notebooks.

Notebook never closes models manually (only via Qt ownership when FileService dies).

## Close tab

By ViewService command.

Notepad:
- Find view by index
- If the model has other views onto it, just close
- If the model is not modified, just close
- If this is the last view on the model, prompt for save
    - If canceled, return
    - If saved/discarded, proceed
- Close view
- If this was the last view on the model, close model

Notebook: Find view by index and close it. Model remains open.

## Close tab everywhere

By ViewService command.

Notepad:
- Find view by index
- Then find all other views onto the same model
- If the model is modified
    - Prompt to save
        - If canceled, return
        - If saved/discarded, proceed
- Close all views onto the model
- Close model

Notebook: Find view by index, then find all other views onto the same model, and close them all. Model remains open.

## Close window tabs

By ViewService command.

Notepad:
- Build two lists
- 1 contains all models unique to this window (modified or not) and would have no views after closing all window tabs
- 2 contains only modified models unique to this window
- If list 2 is not empty
    - Show multi-file save prompt with checkboxes
        - If cancel, return
        - If saved/discarded, proceed
- Close all views in the window
- Close all models in list 1

Notebook: Close all views in the window. Models remain open.

## Close all tabs (in all workspace windows)

By ViewService command.

Notepad:
- Get all modified models
- If the list is not empty
    - Show multi-file save prompt with checkboxes
        - If cancel, return
        - If saved/discarded, proceed
- Close all views in all windows
- Close all models

Notebook: Close all views in all windows. Models remain open.

## Close window

Right now, this is only called by the Window's `close()` method, which in turn calls the registered CloseAcceptor. This may change!

Notepad:
- Build two lists
    - If this is the last window
        - 1 contains all models
        - 2 contains all modified models
    - Else
        - 1 contains all models unique to this window (modified or not) that would have no views after closing all window tabs
        - 2 contains only modified models unique to this window
- If list 2 is not empty
    - Show multi-file save prompt with checkboxes
        - If cancel, return
        - If saved/discarded, proceed
- Close window (or let window close)
- Views will be closed via Qt ownership when window dies
- Close all models in list 1
- If this is the last window, Notepad does not close!

Notebook:
- If this is the last window
- And if the archive is modified
    - Prompt to save
        - If canceled, return
        - If saved/discarded, proceed
    - Close the window
    - Views will be closed via Qt ownership when window dies
    - Models will be closed via Qt ownership when FileService dies
- If this is NOT the last window
    - Close the window
    - Views will be closed via Qt ownership when window dies
    - Models will remain open

## Close all windows

By WindowService command.

Notepad:
- Get all modified models
- If the list is not empty
    - Show multi-file save prompt with checkboxes
        - If cancel, return
        - If saved/discarded, proceed
- Close windows (or let windows close)
- Views will be closed via Qt ownership when window dies
- Close all models
- This does NOT close Notepad

Notebook:
- This closes the Notebook
- If the archive is modified
    - Prompt to save
        - If canceled, return
        - If saved/discarded, proceed
- Close all windows
- Views will be closed via Qt ownership when windows die
- Models will be closed via Qt ownership when FileService dies

## Quit

Unsure of trigger yet

For Each Notebook (handle Notebooks first):
- Try Close all windows (includes possible save prompt per Notebook)
- If any save prompts are canceled, abort Quit entirely
- Application will delete Notebook and move to the next one

Notepad:
- If Notepad has any windows
    - Get all modified models
    - If the list is not empty
        - Show multi-file save prompt with checkboxes
            - If cancel, return
            - If saved/discarded, proceed
    - Close windows (or let windows close)
    - Views will be closed via Qt ownership when window dies
    - Models will be closed via Qt ownership when FileService dies (when Notepad dies)
- Notepad dies with application

Call Application::quit()

## System Shutdown

TODO: Reference Qt docs for QGuiApplication::commitDataRequest

## Potentially irrelevant things

NOTES:

// Quit procedure (from Notebook's perspective):
//...figure out after window closure

// TODO: Should we have a "quit acceptor"? It could run a new
// CLOSE_ALL_WINDOWS command from the base class? Allow us to handle
// things in a specific way when the application is closing, instead of
// just letting each window close (and possibly resulting in multiple
// save prompts, when one would be better)?


// Quit procedure (from Notepad's perspective):
// - (This could all be a closeAllWindows_ command handler? Don't know
// if that would work, due to the closeAcceptor. And, honestly, it may
// prevent the the following stuff, too)
// - Get a list of all file models (iterating backward) that are
// modified (see 9e6cd80 ViewCloseHelper)
// - Save Prompt (multi-file selection version; Save (with
// selections, defaulted to all), Discard, or Cancel)
// - Handle prompt result (Cancel return, Discard proceed without
// saves, Save (any or all selected)
// If proceeding:
// - Close all views everywhere
// - Delete all file models
// - Close all windows?
// - return true for quittable?

// TODO: Should we have a "quit acceptor"? It could run a new
// CLOSE_ALL_WINDOWS command from the base class? Allow us to handle
// things in a specific way when the application is closing, instead of
// just letting each window close (and possibly resulting in multiple
// save prompts, when one would be better)?

OLD:

**Close tab (poly, toggle):**

- In Notepad: Check if this model has views in other windows. If yes, just close this view. If this is the last view on the model: check if modified. If modified, prompt to save. On save/discard, close the view and model.
- In Notebook: Close the view without prompting. Model remains open to persist changes and undo/redo stacks.
- Toggle condition: Window has any tabs.

**Close window tabs (poly, toggle):**

- In Notepad: Iterate backward through tabs. Build a list of unique modified models that only have views in this window (skip models with views in other windows). If the list is not empty, show multi-file save prompt with checkboxes. User chooses: save selected files, discard all, or cancel. If cancel, abort. Otherwise, save chosen files, then close all views and models.
- In Notebook: Simply close all views without prompting. Archive stays marked as modified if applicable.
- Toggle condition: Window has any tabs.

**Close window:**

- Calls the window close method, which in turn calls the close acceptor, allowing necessary checks to happen before accepting or rejecting the close.
- In Notepad: This may prompt for saves (via close all tabs logic).
- In Notebook: If this is NOT the last window, close tabs without prompting. If this IS the last window, check if archive is modified. If modified, prompt to save the archive. On save/discard, close tabs and clean up temp directory. On cancel, abort window close.
- WindowService emits `windowDestroyed` on success. If this was the last window in the workspace, emits `lastWindowClosed`.

**Quit:**

- First, iterate through all Notebook workspaces. For each: check if modified, prompt to save archive if needed, close all windows. If any Notebook close is canceled, abort quit.
- Then, iterate through Notepad windows (reverse z-order). Call close window for each. If any window close is cancelled, abort quit.
- If all closes succeed, call `Application::quit()`.
