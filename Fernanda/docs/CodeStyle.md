# Code Style Guide

## Code

> [!IMPORTANT]
> If a file is over 1,000 lines, there is likely a major problem.

### Initializations

Always initialize local variables, including containers.

```cpp
QStringList list;   // No
QStringList list{}; // Yes
```

#### Constructors

Pretty much always put ctor setup logic in a separate, private method called `setup_`.

### Includes

Always use the full relative path from `src/` for first-party includes, even when the files are in the same folder. For example, `Debug.cpp` should include `"core/Debug.h"`, not `"Debug.h"`.

Separate sections with an empty line. Sections should follow this order, top-to-bottom: main header (if `.cpp` file), third-party (each separated by a blank line), project headers.

```cpp
#include "path/to/Class.h" // If Class.cpp (1st party, but goes at the top in source files)

#include <type_traits> // 3rd party std lib

#include <QClipboard> // 3rd party Qt
#include <QString>
#include <QWidget>

#include <Windows.h> // 3rd party OS

#include <Coco/Bool.h> // 3rd party, submodule

#include "core/Application.h" // 1st party
#include "ui/CustomWidget1.h"
#include "ui/CustomWidget2.h"
```

Err on the side of including everything used by name in a given file, rather than relying on transitive includes. Exceptions: headers like `QtGlobal`, `QtTypes`, or `QObject` in a `QWidget` subclass when only using the `Q_OBJECT` macro.

### Guard Clauses

Prefer guard clauses and early returns. Group related checks together, but keep unrelated checks separate. Use judgment for grouping.

```cpp
// Good:
void doSomething(Window* window, int index)
{
    if (!window || index < 0) return; // Together

    if (!isModified(window, index)) return; // Separate from the others

    //...
}

// Bad:
void doSomething(Window* window, int index)
{
    if (!window || index < 0 || !isModified(window, index))
        return; // Not the same kind of check!

    //...
}
```

### Naming Conventions

**Members and Functions**: camelCase

```cpp
void processDocument();
int lineHeight;
```

**Types** (classes, structs, enums, etc.): PascalCase

```cpp
class TextEditor;
enum class FileState;
```

**Local variables/lambdas**: snake_case

```cpp
auto line_count = 0;
auto get_line_count = [] { return x; };
```

**Parameter Names**

When listing parameters of type `AbstractFileModel` or `AbstractFileView` use parameter names of `fileModel` and `fileView` (not `model` and `view`, since there are multiple types of models and views in the program). It's okay, however, to use the shorter names as local variable names if the context is clear.

#### File names

Don't name a file or namespace "Util" or "Utilities." Give every file/namespace a unique name that reflects its purpose.

### Trailing Underscore Convention

**Private/internal members and non-members**: Add trailing underscore `_`

This applies to:
- Private class members (variables and methods)
- Non-member functions and variables with internal linkage (e.g., `static` in source files, anonymous namespaces)
- Conceptually private members and non-members, e.g. "internal" `detail` namespaces (prefer the name `Internal` over `detail`)

```cpp
class MyClass {
public:
    void publicMethod();
    
private:
    void privateMethod_();
    int privateData_;
    
    // Private nested class also gets trailing underscore
    class PrivateHelper_ {
    public:
        // Public members of a private class do NOT get trailing underscore
        void helperMethod();
        int publicValue;
    };
};

namespace Internal {
    // Internal implementation function
    void internalHelper_();
}
```

### Getters and Setters

**Getter naming**: Remove the trailing underscore from the private member name

**Setter parameter**: Use the same name as the member variable (without underscore). Setter name should be `setMemberName`.

```cpp
class Document {
public:
    // Getter uses member name without underscore
    QString buttonText() const { return buttonText_; }
    
    // Setter parameter uses same name as member (without underscore)
    void setButtonText(const QString& buttonText) { buttonText_ = buttonText; }
    // (Using `text` as the parameter name instead of `buttonText` would be fine. Use what's direct and clear.)

private:
    QString buttonText_;
};
```

### Functions

Even though it's unnecessary in many (most) cases, prefer to keep the `virtual` keyword on overridden virtuals.

#### Functions for Clarity

This:

```cpp
    void addTabWidget_(Window* window)
    {
        if (!window) return;

        auto tab_widget = new TabWidget(window);
        window->setCentralWidget(tab_widget);

        //...
    }

private slots:
    // GOOD:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;
        addTabWidget_(window);
    }
```

is better than this:

```cpp
private slots:
    // BAD:
    void onWindowCreated_(Window* window)
    {
        if (!window) return;

        auto tab_widget = new TabWidget(window);
        window->setCentralWidget(tab_widget);

        //...
    }
```

### Signals

#### The `emit` Macro

Always use the `emit` macro when emitting a signal, and always place it before the object:

```cpp
emit settingChanged(key, value);    // Good (own signal)
emit bus->fileModelReadied(model);  // Good (signal on another object)

bus->emit fileModelReadied(model);  // Bad (emit must come first)
settingChanged(key, value);         // Bad (missing emit)
```

#### Signal Forwarding

When forwarding one signal to another, prefer an explicit lambda with `emit` over a direct signal-to-signal connect. This makes the emission visible at the call site:

```cpp
// Good: the emit is visible
connect(
    panel,
    &SettingsPanel::settingChanged,
    this,
    [this](const QString& key, const QVariant& value) {
        emit settingChanged(key, value);
    });

// Avoid: hides the fact that a signal is being emitted
connect(
    panel,
    &SettingsPanel::settingChanged,
    this,
    &SettingsDialog::settingChanged);
```

### Comments and Documentation

Reserve multi-line comments (`/* ... */`) for the license statement and commenting-out entire files only.

Comments should be concise but readable. If the final (or only) sentence's final punctuation would be a period, omit it.

**Section headers**: Must have a blank line before the following function to avoid showing in IntelliSense tooltip pop-ups. Only use this format: `// --- Section ---`.

```cpp
// --- File operations ---

void openFile(const QString& path);
```

**Function/variable documentation**: Place directly above to show in IntelliSense pop-ups (no blank line between).

```cpp
// Opens a file and returns whether successful
bool openFile(const QString& path);
```

**TODOs**: Always prefix with `TODO:`. Uniform prefixes keep these easily searchable.

```cpp
// TODO: Implement autosave functionality
```

**General notes**: Do NOT prefix with `NOTE:`. If it's particularly important, you can use `NB:` (for nota bene). Adding `NOTE:` would be redundant. Its existence alone is what identifies it as a note.

```cpp
// This assumes the file is already validated
void processValidatedFile();
```

**Permanent notes**: Place inside function body unless a constant reminder (IntelliSense pop-up) is needed on usage.

```cpp
// Good: reminder needed every time function is called
// WARNING: This function is not thread-safe
void updateGlobalState();

void someFunction() {
    // Good: implementation detail that doesn't need to show in IntelliSense
    // We need to flush here because Qt buffers by default
    stream.flush();
}
```

## When in Doubt

**Design from the outside in.** When stuck on how something should work internally, write example usage code first. The ideal API often reveals the right implementation.

Examples:
- Don't know how to structure a builder? Write the call chain you wish existed
- Unclear about ownership? Draft the construction and cleanup you'd want
- Unsure about state management? Write the usage that would feel obvious

See [MenuBuilder.h](../src/menus/MenuBuilder.h) for a good example. Its interface drives the entire implementation.

## User Interface

### Menus

**Case**: Sentence case (capitalize first word only, plus proper nouns)

```
New tab
Save all
```

**Dialog indicators**: Actions that open dialogs end with `...`

```
Open file...
Save as...
```

## Reference Implementations

The following files demonstrate clean implementation of standards:

### [MenuBuilder.h](../src/menus/MenuBuilder.h)

General-purpose menu builder with fluent interface.

Good example of:
- Consistent naming (camelCase members, PascalCase types, trailing underscores)
- Fluent API with defensive null checks throughout
- Qt ownership patterns (parent-child relationships, proper signal connections)
- Modern C++ where appropriate (concepts, template constraints)

Unlike most of Fernanda's objects and widgets (which are intentionally specialized), MenuBuilder is a good example of a general-purpose (but Qt-specific) utility class.
