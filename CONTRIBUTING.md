# Contributing to Fernanda

Thanks for your interest in contributing to Fernanda. This document covers how to report issues, propose changes, and submit code.

## Reporting Issues

When opening an issue, please include:

- What you expected to happen and what actually happened
- Steps to reproduce the problem
- Your OS and Qt version
- Any relevant log output

For feature requests, describe the use case and why it fits Fernanda's goals as a plain-text-first creative writing workbench.

## Before You Start

**Open an issue first.** Before starting work on anything beyond a trivial fix, please open an issue to discuss the change. Fernanda is an opinionated project with specific architectural goals, and not every contribution will be a good fit. A quick conversation up front saves everyone time.

## Understanding the Codebase

Fernanda's architecture is intentionally structured. Before making changes to core code, please read:

- [Architecture.md](Fernanda/docs/Architecture.md): the service/bus model, ownership rules, and how workspaces, services, and modules interact
- [CodeStyle.md](Fernanda/docs/CodeStyle.md): naming conventions, formatting rules, and patterns used throughout the project

A few things that may surprise you:

- **No UI or project files**: Fernanda's widgets are, for better and worse, created in code, and it's built with CMake. Qt Creator is an excellent, sane choice for Qt development, but I have chosen not to use it for this project
- **Header-only by convention.** Nearly all implementation lives in headers. Source files (`.cpp`) exist only where necessary (circular dependencies, large translation units, or Qt MOC requirements). Don't move implementations to source files without discussion
- **Services own mechanics, Workspaces own policy.** If you're adding behavior, think about where the decision belongs. Services should stay somewhat generic; workspace-specific logic belongs in the Workspace subclass (this is why you'll see the word `hook` pop-up quite a lot)
- **Bus for lateral communication, direct calls for explicit dependencies.** Workspaces can call on their Services directly, but their sister Services don't communicate with each other directly. Cross-service (lateral) coordination goes through the Bus (signals and commands)

## Submitting a Pull Request

- Reference the issue your PR addresses
- Keep changes focused. One PR per concern
- Follow the code style in [CodeStyle.md](Fernanda/docs/CodeStyle.md)
- Test on your platform. Fernanda targets Windows, macOS, and Linux, but testing on your own OS is the minimum expectation
- Keep commit messages clear and concise (definitely a case of "do as I say, not as I do" ;) )

## Scope

Fernanda is a creative writing workbench, not a general-purpose text editor or IDE. Contributions that align with that focus are welcome. Changes that would add significant complexity for marginal benefit, or that conflict with the project's architectural direction, will likely be declined.

I'm selective about what gets merged, and that's not a reflection on the quality of your work. It just means Fernanda has a specific vision.

## License

Contributions are accepted under Fernanda's existing license (GPL v3). By submitting a PR, you agree that your contribution falls under the same terms.
