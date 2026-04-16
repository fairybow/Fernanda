# Documentation Style

Documentation describes workflows and processes that are not on their own self-evident or obvious. They describe the ideal operations of the program. Ideally, documentation matches what the program does. When the program doesn't behave as documented, the documentation is the source of truth for correct behavior, and the program has a bug.

Documentation should be somewhat abstracted from the source code where possible, to minimize stale code references. Name public, externally-facing elements (Bus signals, hook categories, public types) when they carry meaning to readers. Avoid naming internal members, private helpers, and implementation flags. Those are the parts most likely to be renamed. Illustrative diagrams and tables are fine, but extensive code snippets pull the doc toward staleness.

A good piece of documentation usually documents a workflow, pattern, or process rather than a single file.

## Reference Implementations

The following doc(s) demonstrate clean implementation of standards:

- [Splits.md](Splits.md)