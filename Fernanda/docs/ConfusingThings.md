# Confusing Things

TODO: Expand this!

## Why `poly` commands?

...

## Each Workspace instance has its own Bus

So, interceptors registered in Notepad won't be called when executing commands within a Notebook, for example.

## What does FileService do?

TODO: Solid indication we should rename the service!

`FileService` isn't really in charge of *files* per se. It's more in charge of the underlying file models. A file for the Notepad is a regular OS file, but the file for a Notebook is the archive.

## What happens when a file is opened?

...

## What happens when a file is saved?

...

## What happens when a file is closed?

Well, first, closing a tab closes a file's *view*, not the file (model) itself. The underlying file model can have multiple views onto it, which means all views show the same changes to the model.

## Models and views

There's file models and file views. There's also the Workspace type's individual model (`QFileSystemModel` or `FnxModel`) and its views (`TreeView`s).

## Why have Workspaces execute/call commands when they have access to the services' public methods?

Just for consistency, really. I'd rather err on the side of using commands for everything and then, later, if it doesn't seem too confusing, have Workspaces call the methods directly. However, given that everything is being called through commands, most (if not all) of the services' command-related methods are private anyway, since they never need to be called directly.
