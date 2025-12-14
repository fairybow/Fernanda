# Saves (Draft)

TODO: Find Coco::PathUtil dialogs and remove from places that shouldn't be using them. Should only be FileService, I think.

## Current State

`AbstractFileModel::save()` does the I/O directly. This conflates two concerns:
1. **Content management** (what the model holds, modification state, undo/redo)
2. **Persistence** (reading/writing to disk)

## Who Should Handle Persistence?

**FileService** is the natural owner of file I/O. This aligns with your existing architecture where Services handle mechanics.

| Component | Responsibility |
|-----------|----------------|
| **AbstractFileModel** | Content state, modification tracking, undo/redo, provides content for reading |
| **FileService** | Actual I/O operations (`save`, `saveAs`, `load`) |
| **Workspace** | Policy decisions (when to prompt, what to do with user's choice) |
| **SavePrompt** | Pure UI - display info, collect choices |

## Benefits of FileService Owning I/O

1. **Testability** - Models can be tested without filesystem access
2. **Workspace flexibility** - Notepad saves to OS paths, Notebook saves to temp dir (different FileService implementations or configurations)
3. **Centralized I/O concerns** - Error handling, encoding, backup copies, etc. live in one place
4. **Model stays pure** - Just a document container

## What SavePrompt Gets

SavePrompt becomes even simpler - it's *purely* a choice collector:

```cpp
struct FileInfo {
    QString title;
    QString subtitle;  // Optional path/context
};

// Returns: choice + which indices were selected
Result exec(const QList<FileInfo>& files, QWidget* parent);
```

It knows nothing about saving, models, or services. It's just: "Here are some file names. User, which ones do you want to save?"

## The Full Flow (Notepad Example)

```cpp
// In Notepad's canCloseWindowTabs hook:

// 1. Collect modified models (Workspace knows policy)
QList<AbstractFileModel*> modifiedModels = collectModifiedModelsUniqueToWindow(window);

// 2. Build display data (Workspace bridges domain → UI)
QList<SavePrompt::FileInfo> displayList;
for (auto* model : modifiedModels) {
    displayList.append({ model->meta()->title(), model->meta()->toolTip() });
}

// 3. Show prompt (pure UI)
auto result = SavePrompt::exec(displayList, window);

if (result.choice == SaveChoice::Cancel)
    return Accept::No;

// 4. Execute saves (Workspace calls Service for mechanics)
if (result.choice == SaveChoice::Save) {
    for (int idx : result.selectedIndices) {
        auto* model = modifiedModels[idx];
        
        // File needs a path first?
        if (!model->meta()->isOnDisk()) {
            auto path = FileDialogs::saveAs(window);  // Another pure UI utility
            if (path.isEmpty()) 
                return Accept::No;  // User cancelled
            files->saveAs(model, path);
        } else {
            files->save(model);
        }
    }
}

return Accept::Yes;
```

## The Layering

```
┌─────────────────────────────────────────────┐
│  SavePrompt / FileDialogs                   │  ← Pure UI utilities
│  (display info in, choices out)             │
└─────────────────────────────────────────────┘
                     ↕
┌─────────────────────────────────────────────┐
│  Workspace (Notepad/Notebook)               │  ← Policy layer
│  (decides when to prompt, interprets        │
│   choices, coordinates services)            │
└─────────────────────────────────────────────┘
                     ↕
┌─────────────────────────────────────────────┐
│  FileService                                │  ← Mechanics layer
│  (actual I/O, model management)             │
└─────────────────────────────────────────────┘
                     ↕
┌─────────────────────────────────────────────┐
│  AbstractFileModel                                 │  ← Domain layer
│  (content, state, provides data for I/O)    │
└─────────────────────────────────────────────┘
```

## FileService API Sketch

```cpp
class FileService : public IService {
public:
    SaveResult save(AbstractFileModel* model);                    // Save to existing path
    SaveResult saveAs(AbstractFileModel* model, const Coco::Path& path);  // Save to new path
    
private:
    SaveResult writeToPath(AbstractFileModel* model, const Coco::Path& path);
};
```

The model provides content (perhaps via `model->content()` or similar), FileService handles the actual `QFile` operations.

TODO: Add this to new doc:

| Scenario | Archive Exists | Modified | Action |
|----------|---------------|----------|--------|
| Closure | Yes | No | Close immediately |
| Closure | Yes | Yes | Prompt → Save to existing path |
| Closure | No | (always "modified") | Prompt → Save As dialog → Save |
| NOTEBOOK_SAVE | Yes | No | No-op |
| NOTEBOOK_SAVE | Yes | Yes | Save to existing path |
| NOTEBOOK_SAVE | No | (always "modified") | Save As dialog → Save → Update path/subtitle |
| NOTEBOOK_SAVE_AS | Any | Any | Save As dialog → Save → Update path/subtitle |
