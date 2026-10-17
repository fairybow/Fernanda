# Code Style

Conventions for Fernanda's code, comments, and docs. For the design these serve, see [Architecture.md](Architecture.md).

Formatting is handled by `.clang-format` (run `wintools/ClangFormatAll.bat`): 80 columns, 4 spaces, braces on every block, CRLF line endings. This document covers what a formatter can't.

## Principles

- **Sane:** conventional C++ and Qt. Predictable, not clever.
- **Simple:** nothing more complex than the problem requires. No abstraction layer, indirection, or generality without a concrete reason. A small type with a real purpose is fine: a small struct in its own header, for instance, when several files use it and it is easier to find there.
- When the two conflict, sane wins.
- A feature waits until something needs it.

## Files

- Code is header-only, apart from `Main.cpp` and the tests (see [Tests](#tests)). A header is named for the main class it holds.
- Every source file is listed in `CMakeLists.txt`, headers included, even where the build would work without it.
- Every file starts with the license statement (`wintools/ApplyLicenseStatement.bat`).
- A file goes in the folder of the feature it belongs to; "Source layout" in [Architecture.md](Architecture.md) lists the folders. A control that knows nothing of the app goes in `ui/widgets/`. Something only views use goes in `views/`.
- Includes run one way, as that section sets out: nothing in `core/` or `models/` includes `views/` or `ui/`, and `views/` takes only `ui/widgets/` from `ui/`.
- No generic utility headers. Put a function in a header named for what it is about (`core/VaultDotDir.h`, not a `CoreUtility.h`). `ui/UiUtility.h` exists and should not grow.
- `ui/UiConstants.h` holds values tuned by hand and constants shared by more than one class; `views/ViewConstants.h` does the same for the parts in `views/`. Any other constant with one consumer stays private to that consumer.
- Default values for user settings live in the config type that owns the setting, not in a separate header.

## Includes

- First-party includes use the full path from `src/`, even for a file in the same folder: `"core/FileRef.h"`, not `"FileRef.h"`. A file in `src/` itself is just its name: `"App.h"`.
- Order, each group separated by a blank line: standard library, Qt, Coco, first-party.
- Include what a file names. Don't rely on another header pulling it in.

## Naming

- Types are `PascalCase`. Functions and members are `camelCase`. Local variables are `snake_case`. Constants are `UPPER_SNAKE_CASE`.
- Descriptive beats short: `lastUsedEpochMs`, not `ts`.
- **Private members end with an underscore. Protected and public members don't.** This includes the members of a private nested struct or class when they are themselves private; a public member of a private struct has no underscore.
- Anything private in spirit ends with an underscore too, such as a type in an `Internal` namespace that is technically reachable.
- A helper type used only inside one header goes in that header's `Internal` namespace, with an underscore.
- `setupX` functions return nothing. `buildX` functions return what they built.

## Classes

- Constructor work goes in a private `setup_` function, called from the constructor and nowhere else.
- UI setup is split into small, single-purpose functions.
- **No default arguments for something the caller always passes.** This includes Qt's `QWidget* parent = nullptr`: pass the parent explicitly.
- A class that never has a parent takes no parent parameter at all.
- When an object always has the same kind of parent, name the parameter for it (`parentVaultWindow`, `parentView`), even if its type is `QWidget*` or `QObject*`.
- A public function that nothing outside the class calls should be private.
- Initialize every member and local, containers included (`QStringList list{};`). A member that every constructor sets in its initializer list needs no default.
- Use `const`, `[[nodiscard]]`, and `[[maybe_unused]]` wherever they apply.

## Pointers and ownership

- Raw pointers are the default. A smart pointer can hide a lifetime mistake that is better found and fixed in a codebase this size.
- `QPointer` is available where it is really needed. On the GUI thread, a raw pointer cleared from the target's `destroyed` signal does the same job.
- Every object has one evident owner. No singletons.

## Signals and slots

- **Never pass a signal up a chain of parents.** If a widget re-emits a child's signal only so its own parent can hear it, the design is wrong. Give the owner of the behavior a direct connection, or hand down an action it already owns.
- Don't use a signal where a function call would do. A signal that only ever drives one slot in the same class should be a call.

## Paths and strings

- Paths are `Coco::Path` wherever possible. `QString` paths appear only at the edge of a Qt API that requires one.
- Don't wrap a string in `Coco::Path(...)` where it converts on its own.
- Use `Coco::Path` functions in place of `QDir`. Add one to Coco if it is missing.
- String literals use the `u"..."_s` form (`Qt::StringLiterals`). `QStringLiteral` is for `Publication.h` and `Version.h` only.

## Translations

- Wrap every user-facing string in `tr()` where it is used, in a class with `Q_OBJECT`. Where there is no such class, use `QCoreApplication::translate` with a context named for the code.
- Never build a count by hand ("1 match" and "%1 matches"). Use `tr("%Ln match(es)", nullptr, n)`, so each language gets its own plural forms and the number its locale's format. Add the English forms to `Translation_en.ts`.
- Never glue a sentence together from translated pieces. Translate the whole sentence, with `%1` for what goes in it, so a language can put the parts in its own order.
- After changing any translatable text, build the `update_translations` target, translate in Qt Linguist, and commit the `.ts` files. A new string is marked unfinished until it is translated.
- Check the contexts `update_translations` produces. A context that isn't a class's full name (`TabPaneTree` where `Fernanda::Ui::TabPaneTree` is meant, or a class that doesn't call `tr()`) is `lupdate` misreading the code, and its strings are never looked up. The known cause is a function whose return type is qualified (`TabPaneTree::RestoredPage buildPage_()`): `lupdate` takes the function for a member of the qualifying class. Give the type a private alias in the calling class and use that.

## Namespaces

- One blank line after a namespace opens and one before it closes.

## Tests

Tests are Qt Test, in `Fernanda/tests/`. The `APP_BUILD_TESTS` CMake option builds them and CTest runs them.

- One `.cpp` per test class, named for the class (`VaultTest.cpp`) and built as its own executable. Register it with `app_add_test` in `tests/CMakeLists.txt`.
- A test of a class with `Q_OBJECT` needs that class's header, and the header of each `QObject` base class, listed in `AppTestMocs` in the same file. A missing one shows as a link error naming the class's `staticMetaObject`.
- **Test functions go under `private slots:` and take no trailing underscore.** Qt Test finds its tests by looking for private slots, pairs each with its `_data` function by name, and prints the names in its results. These are the only exceptions to the rules above, and only in test files.
- Test through the public API. No `friend` declarations, and no hook added to the app's code for a test to use.
- Name a test for the behavior it checks, as a statement: `flushDoesNotRecreateADeletedFile`.
- Each test builds what it needs and shares nothing with the others, so they run in any order. Files go in a `QTemporaryDir`.
- Write text outside ASCII as escapes (`u"\u00E9"`, `"\xC3\xA9"`), never as the characters themselves, so a case doesn't depend on how a compiler or an editor reads the file.
- For something that should happen later, wait with `QTRY_COMPARE` or `QTRY_VERIFY`, never a fixed sleep. For something that should not happen, wait a fixed time and then check. A slow machine can then only make a test pass when it shouldn't, never fail when it shouldn't.
- Keep tests that wait on a timer or the file watcher apart from tests that don't (`VaultTimedTest.cpp` beside `VaultTest.cpp`).
- A test asserts what the code's comments and the docs promise. When one fails, decide which of the two is wrong before changing either.
- A known bug gets a test marked `QEXPECT_FAIL`, and an entry in [TODO.md](TODO.md). The run stays green, and reports the test once the bug is fixed.
- Make sure a new test can fail: break the code it guards, once, and watch it fail.
- Every push builds the app and runs the tests on Linux and macOS (`.github/workflows/ci.yml`). The Linux build runs under AddressSanitizer and UndefinedBehaviorSanitizer, so a memory error or undefined behavior fails the test it happens in. Windows isn't covered there, so run the tests on Windows before pushing.

## Comments

Prefer code that explains itself: a good name over a comment that restates it.

- A comment says what the code does, or why, in the present tense.
- **No history.** Not what the code used to do, what it replaced, or in what order things were built. That is what version control is for. The exception is a warning against a tempting wrong approach: state the reason it is wrong, without the story.
- No references to planning documents, step numbers, or section numbers.
- At most one pointer to a doc per file, in the class's header comment, by heading name: `See docs/Architecture.md, "Saving"`. Use one only when the reasoning spans several files.
- Refer to other code by naming the type or function.
- `TODO` marks something to do.

## Docs

- A doc describes behavior, a design, or a process that spans files. It isn't a tour of one file.
- Name public types and files where they help a reader. Avoid private members and helper names, which are the first things to be renamed.
- Diagrams and tables are welcome. Keep code snippets rare; they go stale.
- [Features.md](Features.md) is for users and names no classes. [Architecture.md](Architecture.md) is for contributors.
- When behavior changes, update Features.md. When something is deferred, decided against, or resolved, update [Future.md](Future.md). Update Architecture.md when a rule that spans files changes.
