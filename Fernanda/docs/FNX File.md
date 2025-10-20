# The `.fnx` File

```
Notebook.fnx
├── content/        # Content directory
├── Model.xml       # Content's virtual structure
└── Settings.ini    # The Notebook's settings (overrides Notepad settings)
```

## `Model.xml` Example

```
<?xml version="1.0"?>
<root name="Notebook 1" archive-path="/home/documents/Notebook 1.fnx">
  <folder name="Chapter 1">
    <file name="1" type="text" uuid="xxx1"/>
  </folder>
  <file name="Notes" type="text" uuid="xxx2">
    <file name="Other Notes" type="text" uuid="xxx3"/>
  </file>
</root>
```
