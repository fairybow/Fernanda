# Code Clean-up

TODO: Add (if not present) checks for noexcept, override, virtual, const, etc.

- [ ] Documentation gaps
- [ ] Ensure (as much as possible) separation of concerns / encapsulation
- [ ] Tr clean-up
- [ ] Coco::Path slash normalization
- [ ] Ensure ambiguous method names and params for model or view are fixed to type of model and type of view (excluding local variables or other places where context is overwhelming)
- [ ] Begin removing unneeded commands and calling public methods in Workspaces where appropriate. Determine what commands Workspace uses that are still used by other Services/Modules. Whatever isn't could be a public method.
- [ ] ^ Open file at path, new .txt, windows set (maybe) commands, etc
- [ ] ^ Open file is complicated because of the need for interception (need to reconcile this command, anyway, with the Notepad and Notebook open/import file commands)
- [ ] Find all unused functions and remove
- [ ] All commands should just call functions (like in ViewService), looks much cleaner, easier to follow
- [ ] Make sure services aren't calling their own commands, dum-dum
- [ ] Ensure setup_ methods are only called by ctor (not in an initialize function); they should have only ctor-friendly setup, too
- [ ] Ensure functions are well-named (actions taken on windows have the right preposition, for example (like `openFileIn(window)`))
- [ ] Standardize callback code for close acceptor and similar
- [ ] Find code that needs to be sectioned-off into a function for clarity
- [ ] Split to h/cpp where appropriate
- [ ] Move Internal namespaces to source files
- [ ] Ensure Internals are _ postfixed and possibly without namespace once in source
- [ ] Clean includes (Commands were in Constants briefly)
- [ ] Check license statements
- [ ] Would really like to see functions organized by category (e.g., public query methods, then public actions, or whatever)
- [ ] Check for places where std::forward would be appropriate (and where args can be forwarding ref)
- [ ] Where possible, make the command handler lambdas just wrap a private method (see ViewService) - much cleaner
- [ ] Clean up lambda captures (value capture may be volatile (re: C++ 20's `=` change), specify everything)
- [ ] Defensive QHash removals, when object is removed not just the window (see TreeViewModule)
- [ ] Search and resolve all TODO comments
- [ ] Remove "NOTE:" before notes, use only TODO or it's a note
- [ ] Use Bus windowDestroyed signal instead of connecting to window destroyed signals in onWindowCreated
- [ ] Uniform use of `nodiscard`
- [ ] Revise all class descriptions
- [ ] Don't cast return values to QVar when registering handlers (it isn't needed)
- [ ] If handler isn't using a command, then make sure the lambda arg is empty (instead of `const Command&`)
- [ ] Check that if handler uses `cmd.context` that its nullptr-checked
- [ ] Ensure all command handler registrations use `cmd.param` instead of `to`
- [ ] QDomDocument::ParseResult as model for save result or similar
- [ ] Lambda captures (whether by ref/val/etc)

Find what needs automatic clean-up from member lists/hashes/sets and ensure we do so, e.g.:

```
modelViews_[model] << view;
connect(view, &QObject::destroyed, this, [&, view, model] {
    modelViews_[model].remove(view);
});
```
