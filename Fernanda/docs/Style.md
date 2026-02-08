# Style Guide

## Code

### Initializations

Always initialize local variables, including containers

```cpp
QStringList list;   // No
QStringList list{}; // Yes
```

### Includes

Separate sections with an empty line. Sections should follow this order, top-to-bottom: main header (if `.cpp` file), third-party (each separated by a blank line), project headers.

```cpp
#include "Class.h" // If Class.cpp (1st party, but should go at the top in source files)

#include <type_traits> // 3rd party std lib

#include <QClipboard> // 3rd party Qt
#include <QString>
#include <QWidget>

#include "Windows.h" // 3rd party OS

#include "Coco/Bool.h" // 3rd party, submodule

#include "Application.h" // 1st party
#include "CustomWidget1.h"
#include "CustomWidget2.h"
```

### Guard Clauses

Prefer organizing guard clauses / early returns. Put common items together, but don't group with unrelated checks. (This is subjective.)

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
auto get_line_count = [&] { return x; };
```

**Parameter Names**

When listing parameters of type `AbstractFileModel` or `AbstractFileView` use parameter names of `fileModel` and `fileView` (not `model` and `view`, since there are multiple types of models and views in the program). It's okay, however, to use the shorter names as local variable names if the context is clear.

### Trailing Underscore Convention

**Private/internal members and non-members**: Add trailing underscore `_`

This applies to:
- Private class members (variables and methods)
- Non-member functions and variables with internal linkage (e.g., `static` in source files, anonymous namespaces)
- Conceptually private members and non-members, e.g. "internal" `detail` namespaces

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

namespace detail {
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
    
private:
    QString buttonText_;
};
```

### Functions for Clarity

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

### Comments and Documentation

**Section headers**: Must have blank line before the following function to avoid showing in IntelliSense tooltip pop-up

```cpp
// === File Operations ===

void openFile(const QString& path);
```

**Function/variable documentation**: Place directly above to show in IntelliSense pop-up

```cpp
// Opens a file and returns whether successful
bool openFile(const QString& path);
```

**TODOs**: Always prefix with `TODO:`

```cpp
// TODO: Implement auto-save functionality
```

**General notes**: Do NOT prefix with `NOTE:`

```cpp
// This assumes the file is already validated
void processValidatedFile();
```

**Permanent notes**: Place inside function body unless constant reminder (IntelliSense pop-up) needed on usage

```cpp
// Good - reminder needed every time function is called
// WARNING: This function is not thread-safe
void updateGlobalState();

void someFunction() {
    // Good - implementation detail that doesn't need to show in IntelliSense
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

See [MenuBuilder.h](../src/MenuBuilder.h) for a good example. Its interface drives the entire implementation.

TODO: Add concrete before/after example showing this in practice

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

### [MenuBuilder.h](../src/MenuBuilder.h)

General-purpose menu builder with fluent interface.

Good example of:
- Consistent naming (camelCase members, PascalCase types, trailing underscores)
- Fluent API with defensive null checks throughout
- Qt ownership patterns (parent-child relationships, proper signal connections)
- Modern C++ where appropriate (concepts, template constraints)

Unlike most of Fernanda's objects and widgets (which are intentionally specialized), MenuBuilder is a good example of a general-purpose (but Qt-specific) utility class.
