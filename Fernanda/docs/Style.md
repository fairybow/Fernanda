# Style Guide

## Code

### Initializations

Always initialize local variables, including containers

```cpp
QStringList list;   // No
QStringList list{}; // Yes
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

**Section headers**: Must have blank line before the following function to avoid showing in IntelliSense

```cpp
// === File Operations ===

void openFile(const QString& path);
```

**Function/variable documentation**: Place directly above to show in IntelliSense

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

**Permanent notes**: Place inside function body unless constant reminder needed on usage

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
