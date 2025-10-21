# The `.fnx` File

## Archive Structure

```
Notebook.fnx
├── content/        # Content directory
├── Model.xml       # Content's virtual structure
└── Settings.ini    # The Notebook's settings (overrides Notepad settings)
```

The `Settings.ini` file is optional. It's generated as needed and removing it will result only in settings reverting to Notepad settings (or default).

## `Model.xml` Example

```
<?xml version="1.0"?>
<root>
  <folder name="Chapter 1">
    <file name="1" type="plaintext" uuid="xxx1"/>
  </folder>
  <file name="Notes" type="plaintext" uuid="xxx2">
    <file name="Other Notes" type="plaintext" uuid="xxx3"/>
  </file>
</root>
```

## Content

Each file will be named with a UUID and the appropriate extension (i.e., `.txt`).
