# Confusing Things

TODO: Expand this!

## Why `poly` commands?

...

## What happens when a file is opened?

...

## What happens when a file is saved?

...

## What happens when a file is closed?

Well, first, closing a tab closes a file's *view*, not the file (model) itself. The underlying file model can have multiple views onto it, which means all views show the same changes to the model.

`FileService` will automatically destroy a file model when the last view on that model is closed.

## Models and views

There's file models and file views. There's also the Workspace type's individual model (`QFileSystemModel` or `FnxModel`) and its views (`TreeView`s).
