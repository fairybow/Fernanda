# Adding a Submodule at a Specific Commit (Example)

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