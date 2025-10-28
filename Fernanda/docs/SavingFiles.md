# Saving Files

## Notepad

**Save / Save As**: Traditional manual file saving. Save writes the current file to disk at its existing path (or prompts for a path if new). (Save As always prompts for a new path.)

**Auto-Save**: Automatic periodic saving of files to disk in the temp folder.

TODO: Plan save; auto-save is not a priority yet.

## Notebook

**Archive Save / Save As**: User-initiated packing of the temp directory contents back into the `.fnx` archive. These operations compress and persist all temp directory changes.

**Archive Auto-Save**: Automatic periodic packing of the temp directory to the .fnx archive.

**Persistence Auto-Save**: Continuous background saving of individual file changes to temp directory. This would ensure file content persists even if the archive hasn't been saved. At least, we'd want to always save a file on last view close (when the model will be removed from FileService).

TODO: Decide if we want the persistence saving OR simply leaving all models open, even when their final view closes? That seems less than ideal...

TODO: Plan save and potentially persistence saving; archive auto-save is not a priority yet.
