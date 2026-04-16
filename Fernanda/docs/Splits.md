# Splits

A Window can display multiple TabWidgets side-by-side, called splits. Each split is an independent TabWidget with its own tabs, current index, and focus. One split is "active" at any time; menu actions and the tab context menu target the active split.

See: [`TabSurface.h`](../src/ui/TabSurface.h), [`TabWidget.h`](../src/ui/TabWidget.h), and [`ViewService.h`](../src/services/ViewService.h)

## Structure

A `TabSurface` is the Window's central widget and owns the splits via a horizontal splitter. When a Window is created, `TabSurface` starts with one split. Splits are added or removed on demand.

Window
+-- TabSurface
+-- QSplitter (horizontal)
+-- TabWidget (split 1)
+-- TabWidget (split 2)
+-- ...

`TabSurface` exposes navigation helpers (left/right neighbor lookup, insert-before/after) and tracks the active split. The active split changes when focus moves into any descendant of a TabWidget.

## Operations

Splits are created implicitly through operations, not as a primary user action:

- **Move to split left/right**: removes the tab from the source split and adds it to the left or right neighbor. If no neighbor exists in that direction, a new split is created there.
- **Duplicate to split left/right**: same as above, but the source tab is kept; a new view on the same model is added to the target split.
- **Close split**: closes all tabs in the active split and removes the split itself (only if more than one split exists).

Splits are also created via drag-and-drop: dragging a tab to the left or right edge of a TabWidget drops it into a new split on that side.

## Lifecycle

Splits are created when an operation needs one and don't exist yet. They're removed when:

- The user closes the split explicitly.
- A split becomes empty (all tabs removed) and more than one split exists.

Auto-collapse of empty splits can be suppressed temporarily, which is needed during tab drag operations (a tab can leave a split empty during the drag before landing in its destination).

## Policy

Close-split is gated by a Workspace hook, following the same pattern as other close operations. A Workspace can refuse the close, typically to prompt the user about unsaved changes. See [`Closures.md`](Closures.md) for the hook pattern.

## Signals

`splitCountChanged` fires when splits are added or removed, letting the Workspace refresh menu state (for example, enabling or disabling "Close split" based on whether more than one split exists).