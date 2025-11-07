# The `.fnx` File

## Archive Structure

TODO: Recoverable trash

```
Notebook.fnx
├── content/            # Content directory
├── Model.xml           # Content's virtual structure
└── Settings.ini        # The Notebook's settings (overrides Notepad settings)
```

The `Settings.ini` file is optional. It's generated as needed and removing it will result only in settings reverting to Notepad settings (or default).

## `Model.xml` Example

This file describes the virtual structure of the Notebook. While the files exist in `content`, the folders and structure only exist in this model.

```
<?xml version="1.0"?>
<notebook>
  <folder name="Chapter 1">
    <file name="1" uuid="xxx1" type="plaintext" extension=".txt"/>
  </folder>
  <file name="Notes" uuid="xxx2" type="plaintext" extension=".txt">
    <file name="Other Notes" uuid="xxx3" type="plaintext" extension=".txt"/>
  </file>
</notebook>
```

This could in theory be deleted and then regenerated, but we'd lose the file names and virtual directory structure. TODO: However, file names could be preserved by just naming each file `name_uuid.ext` and parsing appropriately.

## Content

Each file will be named with a UUID and the appropriate extension (i.e., `.txt`).
