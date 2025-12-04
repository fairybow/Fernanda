# Claude SoC Audit

# Revised Audit

You raise excellent points on both counts. Let me reconsider:

## Point 2 — Window as Context is Fine

You're right. Looking at the Bus architecture more carefully:

```cpp
void execute(const QString& id, const QVariantMap& params, Window* context)
```

`Window* context` is an **opaque routing handle**, not a dependency. FileService doesn't reach into Window internals — it just says "I made this model, and it belongs to *this* context." ViewService then receives the event and knows where to put the view.

This is proper message passing. FileService treats Window as an opaque token, not as something it manipulates. The flow is:

```
FileService: "Here's a model for context X" → Bus event
ViewService: "I'll create a view and put it in context X's TabWidget"
```

FileService doesn't know or care what a Window contains. It's just forwarding context. **Point 2 is moot.**

## Point 8 — Window/WindowService Tight Coupling is Intentional

You're also right here. Window and WindowService form a deliberate **cohesive unit**:

- WindowService is the "controller" for Window lifecycle
- Window is a managed container that defers policy to its service
- The `friend` relationship and `service_` pointer make this explicit

This is similar to how Qt's own classes work internally (e.g., `QWidgetPrivate`). The tight coupling is *by design* — they're meant to be developed and reasoned about together.

The alternative (pure signals) would add complexity without real benefit. As long as:
- Only WindowService sets `service_`
- Window only uses `service_` for close-permission checks
- No other classes depend on this relationship

...then it's clean. **Point 8 can be noted as "intentional design" rather than a violation.**

---

## Revised Audit

| # | Location | Issue | Severity | Fix Approach |
|---|----------|-------|----------|--------------|
| 1 | TextFileModel::save() | Model does I/O | **High** | Move to FileService |
| 2 | ~~FileService::openOffDiskTxtIn~~ | ~~Service takes Window~~ | — | *Moot: Window is opaque context* |
| 3 | FileService::newDiskTextFileModel_ | Service sets doc content | Medium | Model loads own content |
| 4 | ViewService::onFileModelReadied_ | Type-checks concrete models | Medium | Factory/registry pattern |
| 5 | Notepad/Notebook canClose hooks | Iterates views to get models | Medium | Model-centric queries |
| 6 | Notebook context menu | Workspace builds UI | Medium | Delegate to Module |
| 7 | Notebook status indicator | Workspace modifies Window | Low | Service/Module handles |
| 8 | ~~Window::service_~~ | ~~Bidirectional coupling~~ | — | *Intentional: cohesive unit* |
| 9 | SavePrompt | Takes domain types | Medium | Use DTOs |
| 10 | FnxModel::FileInfo | Public ctor takes DOM | Low | Private ctor |
| 11 | Menus quit action | Calls app() directly | Low | Use Bus command |

---

Does this revised view align with your understanding? If so, we have 9 actual items to address, with #1 (save/saveAs moving to FileService) being the highest priority since it directly impacts the SavePrompt work you want to do.

---

# Initial Audit

I'll systematically audit your codebase. Let me start reading through the key files.Now I have a comprehensive view. Here's my audit:

---

# Encapsulation & Separation of Concerns Audit

## 1. **TextFileModel::save() / saveAs()** — Model Does I/O

**Location:** `TextFileModel.h:46-76`

**Violation:** The model directly calls `TextIo::write()` to persist data. Models should manage content state, not perform I/O operations.

**Fix:** 
- Move `save()` / `saveAs()` implementations to `FileService`
- Add `IFileModel::content()` method (returns QString for text models)
- FileService calls `model->content()` and handles the I/O
- Model's `setModified(false)` is called by FileService after successful write

```cpp
// FileService
SaveResult save(IFileModel* model) {
    auto text_model = qobject_cast<TextFileModel*>(model);
    if (!text_model) return SaveResult::NoOp;
    
    auto path = model->meta()->path();
    if (path.isEmpty()) return SaveResult::NoOp;
    
    auto success = TextIo::write(text_model->content(), path);
    if (success) text_model->setModified(false);
    return success ? SaveResult::Success : SaveResult::Fail;
}
```

---

## 2. **FileService::openOffDiskTxtIn()** — Service Knows About Windows

**Location:** `FileService.h:92-98`

**Violation:** FileService directly takes a `Window*` and emits to Bus with it. FileService should manage *models*, not know about window context.

**Fix:**
- FileService should just return the model: `IFileModel* createOffDiskTextModel()`
- The *caller* (Workspace's `newTab` hook) receives the model and emits `fileModelReadied` with appropriate window context
- Or: make this a command that ViewService or Workspace triggers

---

## 3. **FileService::newDiskTextFileModel_()** — FileService Sets Document Text

**Location:** `FileService.h:174-189`

**Violation:** FileService reads file content and directly sets `document->setPlainText(text)`. This couples FileService to `QTextDocument` internals.

**Fix:**
- Add `IFileModel::loadFromPath(const Coco::Path&)` or `loadContent(const QString&)`
- TextFileModel implements this to set its document text
- FileService calls `model->loadContent(TextIo::read(path))` — model handles its own internal setup

---

## 4. **ViewService::onFileModelReadied_()** — Service Type-Checks Concrete Models

**Location:** `ViewService.h:572-596`

**Violation:** ViewService does `qobject_cast<TextFileModel*>` and `qobject_cast<NoOpFileModel*>` to decide which view type to create. This creates tight coupling between ViewService and concrete model types.

**Fix:** Use a factory pattern or registry:
- Option A: `IFileModel` has virtual `createView()` method (model creates its own view)
- Option B: Register model-type → view-type mappings in ViewService
- Option C: `IFileModel::viewTypeHint()` returns an enum, ViewService switches on that

```cpp
// Option C (simplest):
// In IFileModel:
enum class ViewType { Text, NoOp, Image /* future */ };
virtual ViewType viewType() const = 0;

// In ViewService:
switch (model->viewType()) {
case ViewType::Text: view = new TextFileView(...); break;
case ViewType::NoOp: view = new NoOpFileView(...); break;
}
```

---

## 5. **Notepad/Notebook — canClose* Hooks Access Views to Get Models**

**Location:** `Notepad.h:85-250`, `Notebook.h:110-145`

**Violation:** Workspace hooks iterate through `IFileView*` lists and call `view->model()` to collect models. The Workspace should work at the model level, not dig through views.

**Fix:** ViewService should provide model-centric queries:
- `QList<IFileModel*> modifiedModelsIn(Window*)` 
- `QList<IFileModel*> modifiedModelsUniqueToWindow(Window*)`
- `QList<IFileModel*> allModifiedModels()`

The hooks receive models directly, not views:

```cpp
// Change hook signature:
using CanCloseWindowTabsHook = std::function<bool(const QList<IFileModel*>&)>;

// ViewService builds the model list before calling hook
```

---

## 6. **Notebook::onTreeViewContextMenuRequested_()** — Workspace Builds UI

**Location:** `Notebook.h:337-369`

**Violation:** Notebook directly creates `QMenu`, `QAction`, and manages popup. Workspace is policy, not UI construction.

**Fix:**
- Create a `NotebookTreeViewContextMenu` class or use a Module
- Notebook emits a signal or calls a Module method with the context
- The Module/helper constructs the menu and wires actions back to Notebook via Bus commands

---

## 7. **Notebook::addWorkspaceIndicator_()** — Workspace Manipulates Window UI

**Location:** `Notebook.h:272-288`

**Violation:** Notebook directly accesses `window->statusBar()` and adds widgets. Workspaces shouldn't know about Window internals.

**Fix:**
- Add a `WindowService::setStatusBarIndicator(text, color)` method or similar
- Or emit a Bus signal that a Module (e.g., `StatusBarModule`) handles
- Window UI composition belongs in Services/Modules, not Workspace policy

---

## 8. **Window::service_** — Window Holds Back-Reference to Service

**Location:** `Window.h:59-60`

**Violation:** `Window` has a `WindowService* service_` member, creating bidirectional coupling. Window should be a dumb container.

**Current Use:** `closeEvent` needs to check with service about whether close is allowed.

**Fix:** 
- Instead of Window calling `service_->canClose()`, have Window emit a signal `closeRequested()`
- WindowService installs an event filter (already does) to intercept close events
- Or: Window's closeEvent emits `aboutToClose()` signal that WindowService connects to

Actually, looking at `WindowService.cpp`, it already uses an event filter. The `service_` pointer may be legacy — verify if it's actually needed and remove if not.

---

## 9. **SavePrompt Takes IFileModel***

**Location:** `SavePrompt.h` (entire file)

**Violation:** SavePrompt depends on `IFileModel*` and `FileMeta*`. A dialog utility should be domain-agnostic.

**Fix:** (Already discussed)
- SavePrompt takes `QList<FileInfo>` where FileInfo is `{title, subtitle}`
- Caller builds FileInfo list from models
- SavePrompt returns `{choice, selectedIndices}`

---

## 10. **FnxModel::FileInfo Constructor Takes QDomElement**

**Location:** `FnxModel.h:54-57`

**Violation:** `FileInfo` is supposed to be a public DTO, but its constructor takes internal `QDomElement`. External code could theoretically construct invalid FileInfo.

**Fix:**
- Make the `QDomElement` constructor private
- Add a public constructor: `FileInfo(Coco::Path relPath, QString name)`
- FnxModel internally uses the private constructor

```cpp
struct FileInfo {
    Coco::Path relPath{};
    QString name{};
    
    bool isValid() const { return !relPath.isEmpty() && !name.isEmpty(); }
    
private:
    friend class FnxModel;
    FileInfo(const QDomElement& element);  // Internal only
    
public:
    FileInfo() = default;
};
```

---

## 11. **Menus::makeAppQuitAction_()** — External Function Knows About Application

**Location:** `Menus.cpp` (referenced in `Menus.h:102`)

**Violation:** Menu utility code likely calls `app()->tryQuit()` directly, coupling menus to Application.

**Fix:**
- Register a Bus command `application:quit` at Application level
- Menu action triggers `bus->execute("application:quit")`
- Application handles the command

---

## Summary Table

| # | Location | Issue | Severity | Fix Approach |
|---|----------|-------|----------|--------------|
| 1 | TextFileModel::save() | Model does I/O | High | Move to FileService |
| 2 | FileService::openOffDiskTxtIn | Service takes Window | Medium | Return model, caller emits |
| 3 | FileService::newDiskTextFileModel_ | Service sets doc content | Medium | Model loads own content |
| 4 | ViewService::onFileModelReadied_ | Type-checks concrete models | Medium | Factory/registry pattern |
| 5 | Notepad/Notebook canClose hooks | Iterates views to get models | Medium | Model-centric queries |
| 6 | Notebook context menu | Workspace builds UI | Medium | Delegate to Module |
| 7 | Notebook status indicator | Workspace modifies Window | Low | Service/Module handles |
| 8 | Window::service_ | Bidirectional coupling | Low | Verify if needed |
| 9 | SavePrompt | Takes domain types | Medium | Use DTOs |
| 10 | FnxModel::FileInfo | Public ctor takes DOM | Low | Private ctor |
| 11 | Menus quit action | Calls app() directly | Low | Use Bus command |

---

Would you like me to prioritize these and suggest an order for tackling them, or dive deeper into any specific fix?
