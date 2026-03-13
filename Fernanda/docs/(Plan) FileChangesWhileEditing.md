# File Changes Plan

| Scenario | Notepad | Notebook | Internal intercept possible | Alert | Popup |
|---|---|---|---|---|---|
| Renamed | Yes | No (UUIDs) | Yes (TreeView) | Yes | No |
| Moved | Yes | No (UUIDs) | Yes (TreeView) | Yes | No |
| Deleted | Yes | No (not yet) | Future (Trash) | Yes | No |
| Content changed externally | Yes | No (shouldn't happen) | No | Yes | Yes (refresh) |
| Permissions changed | Yes | No (working dir) | No | Maybe later | No |
| Volume unmounted | Yes | No (working dir) | No | Same as deleted | No |
| File replaced (atomic write) | Yes | No | No | Yes | Yes (refresh) |
