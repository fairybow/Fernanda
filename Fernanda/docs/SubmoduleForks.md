# Adding a Forked Submodule at a Specific Commit

Reference procedure for adding a dependency as a Git submodule, using your own GitHub fork, pinned to a specific release commit.

## Prerequisites

- You have already forked the upstream repo on GitHub (e.g., `fairybow/Fernanda-miniz`)
- You know the release commit hash you want to pin to

## Procedure

### 1. Add the submodule

From your project directory (e.g., `Fernanda/`):

```bash
git submodule add https://github.com/fairybow/Fernanda-<lib>.git submodules/<lib>
```

### 2. Check out the release commit

```bash
cd submodules/<lib>
git checkout <commit-hash>
cd ../..
```

### 3. Set the fork's main branch to the release commit

This is cosmetic but keeps your fork clean: its `main` branch will point
directly at the release you are using rather than wherever upstream `main`
happened to be.

```bash
cd submodules/<lib>
git reset --hard <commit-hash>
git push --force
cd ../..
```

This does not change the local checkout (it was already at that commit from
step 2). It only updates where `main` points on your GitHub fork.

### 4. Commit in the parent repo

```bash
git add submodules/<lib>
git commit -m "Add <lib> submodule at <version> (<commit-hash>)"
```

## Updating to a new release later

When upstream publishes a new version you want to adopt:

```bash
cd submodules/<lib>
git remote add upstream https://github.com/<original-owner>/<lib>.git
git fetch upstream
git reset --hard <new-commit-hash>
git push --force
cd ../..
git add submodules/<lib>
git commit -m "Update <lib> to <new-version> (<new-commit-hash>)"
```

You only need the `git remote add upstream` line once. After that, just
`git fetch upstream` before resetting to the new commit.

## Example (miniz v3.1.0)

```bash
git submodule add https://github.com/fairybow/Fernanda-miniz.git submodules/miniz
cd submodules/miniz
git checkout d10b03c
git reset --hard d10b03c
git push --force
cd ../..
git add submodules/miniz
git commit -m "Add miniz submodule at v3.1.0 (d10b03c)"
```