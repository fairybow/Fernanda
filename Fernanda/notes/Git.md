### Replace `main` with a secondary branch:
```
git checkout main
git pull
git checkout secondary_branch
git merge -s ours main
git checkout main
git merge secondary_branch
```

and compare:
```
git diff main..secondary_branch
```

### Add submodule:
```
git submodule add <repository_url> <path>
```

and update one:
```
git submodule update --remote <path>
```

or update all:
```
git submodule update --remote
```