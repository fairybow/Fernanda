# Adding a Submodule at a Specific Commit (Example)

## 1. Reset the fork to the latest release commit

Clone somewhere temporary, reset, and force push. Force push may trigger a browser login prompt.

```bash
cd C:\Dev
git clone https://github.com/fairybow/Hearth-miniz.git temp-miniz
cd temp-miniz
git reset --hard d10b03c
git push --force
```

(Verify on GitHub that the fork shows the correct hash and delete temp.)

## 2. Add the submodule

From the project directory (e.g., `Hearth/`):

```bash
git submodule add https://github.com/fairybow/Hearth-miniz.git submodules/miniz
git add submodules/miniz
git commit -m "Add miniz submodule at v3.1.1 (d10b03c)"
```

## 3. Removing a submodule (if we need to start over)

```bash
git submodule deinit -f submodules/{ submodule }
git rm -f submodules/{ submodule }
```

(May need to manually delete submodule folder in `.git/modules`.)
