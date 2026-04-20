# Closures

How windows and tabs are closed in Fernanda, including the deferred close coalescing system that unifies OS-level and menu-level "close all" behavior.

See: [`WindowService.h`](../src/services/WindowService.h), [`Window.h`](../src/ui/Window.h), [`ViewService.h`](../src/services/ViewService.h), [`Workspace.h`](../src/workspaces/Workspace.h), [`Notepad.h`](../src/workspaces/Notepad.h), [`Notebook.h`](../src/workspaces/Notebook.h), and [`SavePrompt.h`](../src/workspaces/SavePrompt.h)

## Closure Paths

There are three ways windows can be asked to close. All three ultimately converge on the same two outcomes (single-window prompt or combined prompt, if needed):

| Trigger | Entry point | Prompt style |
|---|---|---|
| Menu "Close Window", X button, or single `window->close()` | `Window::closeEvent` -> deferred -> single-window handler -> `canCloseHook_` | Per-window |
| Menu "Close All Windows" or "Quit" | `WindowService::closeAll()` -> `canCloseAllHook_` (one combined prompt) -> `isBatchClose_` flag -> `window->close()` on each | Combined |
| OS taskbar "Close all windows" | Multiple `Window::closeEvent`s -> all deferred -> timer coalesces -> `WindowService::closeAll()` | Combined (identical to menu) |

### Single window close

When a user clicks X on one window or selects "Close Window" from the menu, the window receives a `QCloseEvent`. The event is **deferred** to `WindowService` (see "Deferred Close Coalescing" below). On the next event loop tick, `WindowService` sees one pending window and runs the normal single-window path:

1. `canCloseHook_` is called with the window
2. The Workspace's hook implementation decides whether to allow the close (prompting the user if there are unsaved changes)
3. If allowed, `window->close()` is called again with `isDeferredClose_` set, so the close event passes straight through

### Batch close (menu)

When the user selects "Close All Windows" or "Quit", the code calls `WindowService::closeAll()` directly:

1. `canCloseAllHook_` is called with the full window list
2. The Workspace shows a single combined save prompt for all unsaved files
3. If approved, `isBatchClose_` is set and `window->close()` is called on each window
4. With `isBatchClose_` set, `Window::closeEvent` skips the hook entirely

### Batch close (OS)

When the OS requests closure of all windows (e.g., right-click taskbar -> "Close all windows"), it posts `WM_CLOSE` to each window independently in the same event loop tick. This is handled by deferred close coalescing (see below), which detects multiple pending closes and routes them through `closeAll()`, producing the same combined prompt as the menu path.

## Deferred Close Coalescing

### The problem

When the OS sends `WM_CLOSE` to multiple windows in the same event loop tick, each `Window::closeEvent` would individually invoke `canCloseHook_`, which shows a modal `SavePrompt` dialog. Modal dialogs spin nested event loops via `QDialog::exec()`.

When multiple windows each enter a nested event loop in the same call stack, the inner loops block the outer loops from completing. Concretely:

1. Window A (bottom of z-order) gets its close event processed first
2. `canCloseHook_` runs, `SavePrompt::exec()` opens a modal dialog and enters **EventLoop-A**
3. During EventLoop-A, Qt processes Window B's queued close event
4. `canCloseHook_` runs again, `SavePrompt::exec()` enters **EventLoop-B** (nested inside EventLoop-A)

Now both dialogs are visible (they are `Qt::WindowModal`, not application-modal). If the user dismisses Window A's dialog first, EventLoop-A is flagged to exit but **cannot return** until EventLoop-B exits. The dialog disappears but the window stays open. The user must dismiss Window B's dialog first (top of the stack), then Window A's loop can unwind.

This produces the symptoms: "Don't Save" appears to do nothing on some windows, and subsequent close attempts behave inconsistently.

### The solution

`Window::closeEvent` defers all non-batch close events back to `WindowService` instead of handling them immediately. Each deferred window is added to a pending set, and a zero-delay timer is started (but not restarted if already running). When the timer fires on the next event loop tick, the pending count determines behavior:

- **1 window**: Normal single-window close (`canCloseHook_` runs as usual)
- **N windows**: Treated as a batch close, routed through `closeAll()` with a single combined prompt (`canCloseAllHook_`)

This makes OS-level close-all behave identically to the menu action without changing single-window close behavior.

### Flow

```
Window::closeEvent
    |
    |-- isBatchClose_ or isDeferredClose_?
    |       |
    |       +-- Yes -> Accept (pass through to QMainWindow::closeEvent)
    |
    +-- No -> Ignore the event, call WindowService::deferClose_(this)
                |
                +-- Add window to pendingCloseWindows_
                +-- Start zero-delay timer (if not already started)
                        |
                        +-- Timer fires (next tick)
                            |
                            +-- 1 pending  -> Single-window path (canCloseHook_)
                            +-- N pending  -> closeAll() (canCloseAllHook_)
```

### Safety

The pending set holds raw pointers. Since windows have `WA_DeleteOnClose`, a window could theoretically be destroyed between deferral and timer fire (the zero-delay timer makes this extremely unlikely in practice). `processDeferredCloses_` intersects the pending set with `unorderedWindows_` before acting to discard any stale entries.

## Hook Responsibilities

### WindowService hooks

| Hook | Signature | Called by | Purpose |
|---|---|---|---|
| `canCloseHook_` | `bool(Window*)` | Deferred single-window close | Ask Workspace if one window may close |
| `canCloseAllHook_` | `bool(const QList<Window*>&)` | `closeAll()` | Ask Workspace if all windows may close |

### ViewService hooks

| Hook | Signature | Called by | Purpose |
|---|---|---|---|
| `canCloseTabHook_` | `bool(Window*, int)` | `closeTab()` | Ask Workspace if one tab may close |
| `canCloseTabEverywhereHook_` | `bool(Window*, int)` | `closeTabEverywhere()` | Ask Workspace if a model's tabs across all windows may close |
| `canCloseWindowTabsHook_` | `bool(Window*)` | `closeWindowTabs()` | Ask Workspace if all tabs in one window may close |
| `canCloseAllTabsHook_` | `bool(const QList<Window*>&)` | `closeAllTabs()` | Ask Workspace if all tabs across all windows may close |

### Workspace implementations

**Notepad** overrides all six hooks. Save prompts are per-file because files are independent entities on the OS filesystem. For window-level and workspace-level closes, it collects modified models and shows a multi-file save prompt with checkboxes.

Key detail: `canCloseWindow` uses `ExcludeMultiWindow::Yes` when collecting modified models, so files that have views in other windows are not prompted (they will survive the window close). `canCloseAllWindows` does not exclude multi-window models (everything is closing).

**Notebook** overrides only `canCloseWindow` and `canCloseAllWindows`. Tab-close hooks are left as default (`return true`) because individual file saves are not a Notebook concern (the archive is the save unit). `canCloseWindow` only prompts on the **last** window (`windows->count() > 1` short-circuits), since closing a non-last window just reduces the view count.

## Flags Reference

| Flag | Set by | Checked by | Purpose |
|---|---|---|---|
| `isBatchClose_` | `closeAll()` | `Window::closeEvent` | Skip per-window hook during batch close |
| `isDeferredClose_` | `processDeferredCloses_` (single-window branch) | `Window::closeEvent` | Skip re-deferral when executing a deferred close |

Both flags are scoped to their respective code blocks and reset immediately after the close loop completes.

## Relationship to Architecture.md

The "Closing a Window" sequence diagram in `Architecture.md` shows the general hook pattern. This document covers the full closure system in detail, including the deferred coalescing that the sequence diagram does not depict. If the two documents conflict, this one is authoritative for closure behavior.