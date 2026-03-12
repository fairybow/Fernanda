# Closures (Draft)

**General notes:**
- "Modified archive" (Notebook only): working directory has changed from its original state or has no corresponding archive (is new)
- Notepad never closes
- Notebooks close when their last window closes
- Notepad deletes models when their view counts become 0
- Notebook never closes file models manually (only via Qt ownership when FileService dies)

## Close tab

Triggered by ViewService `CLOSE_TAB` command (index param, -1 = current).

**ViewService mechanics:**
- Normalizes index, gets view
- Calls `canCloseTabHook` (if registered)
- If approved: removes view from `TabWidget` and deletes it, emits `viewDestroyed(model)`

**Notepad policy:**
- If model is modified AND this is the last view on it: raise view, prompt to save
- Cancel aborts, save/discard proceeds
- Otherwise proceeds immediately

**Notebook policy:**
- No hook registered, always proceeds

## Close tab everywhere

Triggered by ViewService `CLOSE_TAB_EVERYWHERE` command (index param, -1 = current).

**ViewService mechanics:**
- Gets target model from view at index
- Collects all views across all windows that reference this model
- Calls `canCloseTabEverywhereHook` with the list of views
- If approved: removes all collected views from `TabWidget` and deletes them, emits single `viewDestroyed(model)`

**Notepad policy:**
- If model is modified: prompt to save
- Cancel aborts, save/discard proceeds
- Otherwise proceeds immediately

**Notebook policy:**
- No hook registered, always proceeds

## Close window tabs

Triggered by ViewService `CLOSE_WINDOW_TABS` command.

**ViewService mechanics:**
- Collects all views and their models in the target window
- Calls `canCloseWindowTabsHook` with the list of views
- If approved: removes all views in window and deletes them, emits `viewDestroyed(model)` for each unique model

**Notepad policy:**
- Collects modified models that only exist in this window (skips models with views in other windows)
- If any found: prompt to save
- Cancel aborts, save selected/discard proceeds
- Otherwise proceeds immediately

**Notebook policy:**
- No hook registered, always proceeds

## Close all tabs (in all workspace windows)

Triggered by ViewService `CLOSE_ALL_TABS` command.

**ViewService mechanics:**
- Collects all views and their models across all workspace windows
- Calls `canCloseAllTabsHook` with the list of views
- If approved: removes all views in all windows and deletes them, emits `viewDestroyed(model)` for each unique model

**Notepad policy:**
- Collects all modified models across all windows
- If any found: prompt to save
- Cancel aborts, save selected/discard proceeds
- Otherwise proceeds immediately

**Notebook policy:**
- No hook registered, always proceeds

## Close window

Triggered by Window's `closeEvent` (user clicking X or calling `close()`).

**Window/WindowService mechanics:**
- If `isBatchClose_` flag is set: bypasses hook, accepts close immediately (used by `closeAll()`)
- Otherwise: calls `canCloseHook` with the window
- If approved: accepts close event, window destructor runs (views deleted via Qt ownership)
- If rejected: ignores close event, window remains open

**Notepad policy:**
- Collects modified models that only exist in this window (skips models with views in other windows)
- If any found: prompt to save
- Cancel aborts, save selected/discard proceeds
- Does NOT close Notepad workspace (even if last window)

**Notebook policy:**
- If NOT the last window: proceeds immediately
- If IS the last window AND archive is modified: prompt to save archive
- Cancel aborts, save/discard proceeds
- If last window closes, Notebook workspace dies (models deleted via Qt ownership when FileService dies)

## Close all windows

Triggered by WindowService `closeAll()` method and `CLOSE_ALL_WINDOWS` command.

**WindowService mechanics:**
- Gets list of all workspace windows (top to bottom)
- Calls `canCloseAllHook` with the list of windows
- If approved: sets `isBatchClose_` flag, closes all windows in sequence (views deleted via Qt ownership), clears flag
- If rejected: returns false, no windows close
- The `isBatchClose_` flag causes individual `closeEvent` handlers to bypass per-window hooks

**Notepad policy:**
- Collects all modified models across all windows
- If any found: prompt to save
- Cancel aborts entire operation, save selected/discard proceeds
- Does NOT close Notepad workspace

**Notebook policy:**
- If archive is modified: prompt to save archive
- Cancel aborts, save/discard proceeds
- Notebook workspace dies (models deleted via Qt ownership when FileService dies)

## Quit

Triggered by Application `tryQuit()` method.

**Application mechanics:**
- Iterates through all Notebooks (most to least recently opened), calls `canQuit()` on each
- If any Notebook refuses: aborts entire quit operation
- Calls `canQuit()` on Notepad
- If Notepad refuses: aborts entire quit operation
- If all approve: calls `QApplication::quit()`

**Notepad policy (`canQuit`):**
- If no windows exist: returns true immediately
- Otherwise: calls `windows->closeAll()` and returns result
- This triggers the full `canCloseAllWindows` hook (save prompt if needed)

**Notebook policy (`canQuit`):**
- Calls `windows->closeAll()` and returns result
- This triggers the full `canCloseAllWindows` hook (save prompt if needed)
- If approved, Notebook dies when last window closes

## Quit (passive)

Triggered automatically when all windows close naturally (not via explicit `tryQuit()`).

**Flow:**
- WindowService detects last window has closed and emits `lastWindowClosed` on Bus
- Workspace propagates signal to Application
- Application responds based on workspace type and remaining workspaces

**When Notebook's last window closes:**
- Application removes Notebook from list and deletes it
- Models deleted via Qt ownership when FileService dies
- If no Notebooks remain AND Notepad has no windows: calls `quit()`

**When Notepad's last window closes:**
- If no Notebooks exist: calls `quit()`
- Otherwise: Notepad remains alive with no windows

**Note:** This path bypasses all `canQuit()` checks since windows have already successfully closed (with their own save prompts if needed)

## System shutdown / session end

Triggered by OS shutdown/logout via Qt's `QGuiApplication::commitDataRequest` signal.

**Application mechanics:**
- Iterates through all Notebooks (most to least recently opened), calls `canQuit()` on each
- If any Notebook refuses: calls `manager.cancel()` to prevent shutdown
- Calls `canQuit()` on Notepad (if it exists)
- If Notepad refuses: calls `manager.cancel()` to prevent shutdown
- If all approve: allows OS to proceed with shutdown

**Workspace behavior:**
- User can cancel any prompt to prevent shutdown
- If user approves all prompts, application terminates and OS continues shutdown
