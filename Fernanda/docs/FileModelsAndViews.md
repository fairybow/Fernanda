# File Models and Views

Fernanda separates file content management from its visual representation using paired abstract classes: `AbstractFileModel` (content and state) and `AbstractFileView` (display and interaction).

See: [`AbstractFileModel.h`](../src/models/AbstractFileModel.h), [`AbstractFileView.h`](../src/views/AbstractFileView.h), [`FileMeta.h`](../src/models/FileMeta.h), [`TextFileModel.h`](../src/models/TextFileModel.h), [`TextFileView.h`](../src/views/TextFileView.h), [`PdfFileModel.h`](../src/models/PdfFileModel.h), and [`PdfFileView.h`](../src/views/PdfFileView.h)

## Overview

```
AbstractFileModel          AbstractFileView
|-- FileMeta (metadata)    |-- References model
|-- Content (data)         |-- Widget setup
|-- Modification state     +-- Editing operations
+-- Undo/redo
```

This separation allows:
- **Multiple views** on a single model (same file in different windows)
- **Different view types** for the same model (future: preview vs. edit modes)
- **Model persistence** independent of view lifecycle
- **Extensibility**: New file types (images, PDFs) or alternative views (Markdown preview) without changing existing code

## AbstractFileModel

The model holds file content and tracks its state. It does *not* perform I/O, that's FileService's job.

```cpp
class AbstractFileModel : public QObject {
public:
    FileMeta* meta() const noexcept;     // Path, title, tooltip

    // These two are the contract. The rest is optional:
    virtual QByteArray data() const = 0;
    virtual void setData(const QByteArray& data) = 0;

    virtual bool isUserEditable() const;
    virtual bool hasUndo() const;
    virtual bool hasRedo() const;
    virtual void undo();
    virtual void redo();

    virtual bool isModified() const;
    virtual void setModified(bool modified);

signals:
    void modificationChanged(bool modified);
    void undoAvailable(bool available);
    void redoAvailable(bool available);
};
```

### FileMeta

Each model owns a `FileMeta` that manages path and display information:

| Property | Description |
|----------|-------------|
| `path()` | File path (empty if not on disk) |
| `isOnDisk()` | Whether the file has been saved |
| `isStale()` | Whether the file has been moved, modified, or deleted from disk |
| `title()` | Display name for tabs |
| `toolTip()` | Full path or status hint |

Title priority: custom override -> path stem -> "Untitled"

## AbstractFileView

The view displays model content and handles user interaction. Each view references exactly one model, but a model may have multiple views.

```cpp
class AbstractFileView : public QWidget {
public:
    void initialize();  // Must be called after construction
    AbstractFileModel* model() const;
    
    virtual bool isUserEditable() const = 0;
    
    virtual bool hasPaste() const;
    virtual bool hasSelection() const;
    virtual void cut();
    virtual void copy();
    virtual void paste();
    virtual void deleteSelection();
    virtual void selectAll();

signals:
    void selectionChanged();
    void clipboardDataChanged();

protected:
    virtual QWidget* setupWidget() = 0;  // Subclasses create their widget here
};
```

### Two-Phase Initialization

Views require explicit initialization after construction:

```cpp
auto view = new TextFileView(model, parent);
view->initialize();  // Calls setupWidget(), sets up layout
```

Calling a pure virtual (`setupWidget()`) from a base class constructor would dispatch to the base, not the derived class, since the derived class is not yet constructed. Two-phase initialization avoids this: the object is fully constructed first, then `initialize()` is called from outside, at which point the virtual call resolves correctly.

## Relationship Diagram

```mermaid
flowchart LR
    subgraph Model["AbstractFileModel"]
        Meta[FileMeta]
        Content[Content/Data]
        State[Modification State]
    end
    
    subgraph View1["AbstractFileView"]
        Widget1[Editor Widget]
    end
    
    subgraph View2["AbstractFileView"]
        Widget2[Editor Widget]
    end
    
    Model --> View1
    Model --> View2
    
    View1 -.->|reads/writes| Content
    View2 -.->|reads/writes| Content
```

## Lifecycle

### Model Lifecycle

1. **Creation**: FileService creates model, optionally loads data from disk
2. **Readying**: FileService emits `fileModelReadied`, ViewService creates a view
3. **Usage**: Model tracks modifications, views display and edit
4. **Saving**: FileService calls `model->data()` and writes to disk
5. **Destruction**: When last view closes (Notepad) or Workspace closes (Notebook)

### View Lifecycle

1. **Creation**: ViewService creates view with model reference
2. **Initialization**: `initialize()` sets up the internal widget
3. **Display**: View added to TabWidget, receives focus
4. **Destruction**: Tab closed, view deleted (model may persist)

## Design Notes

### Why Separate Model and View?

- **Notepad**: Closing the last view on a model closes the model (file is "closed")
- **Notebook**: Models persist even when all views close (file remains in archive)

This different policy is handled at the Workspace level, not baked into the classes.

### Why `data()` Returns QByteArray?

Content is returned as raw bytes for FileService to write. This keeps encoding decisions in one place and supports future binary file types.

### Why This Virtual Structure?

`data()`/`setData()` are pure virtual: every model must implement content storage. Modification state (`isModified`/`setModified`) has a real base implementation because any model can find itself out of sync with disk, whether through user edits or external changes (a backing file being deleted, for example). Models that bring their own tracking (like `TextFileModel` delegating to `QTextDocument`) override these.

Editing capability is separate. `isUserEditable()`, `hasUndo()`, `hasRedo()`, `undo()`, and `redo()` default to false/no-op because not all models support user-initiated content changes. A PDF can be modified (its backing file disappears) without being editable (the user cannot type into it).