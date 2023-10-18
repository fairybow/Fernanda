### By `MenuBar` interaction:

#### Clicking new file:

- AKA, creating a blank **named file by path** and opening a document + new tab for it

- `New file` first opens a dialog, dishes warnings for potential overwriting, regarding path name
- `MenuBar` emits a signal to `MainWindow` asking to open a new file at the given `path`
- `MainWindow` handles this by calling the function to open any tab by path and supplying optional `arg` to first write a new file at that path
- `MainWindow->Document` writes an empty text file to the path
- `MainWindow->Document` sets the current document by path...
- [...]
- `Document` asks `MainWindow` to set the "current document"'s text (there is no "current document", technically)
- `MainWindow->Document` sets the current document text to the text from `Editor->TrueEditor`
	- `Document` ignores this if its **current ID** is `null`
	- If not, though, it serves a `TextDocument` by the **current ID** before it is changed:
		- It checks its **cache** first, and if nothing is found, a "new" `TextDocument` is made to save the state of the outgoing tab (file/text)
		- If the document for this tab was evicted from the **cache**, then it must be recovered
		- To do so, two empty strings are created (**initial** and **original** texts)
		- `Document` checks if a temporary file (named for the outgoing ID) is extant
		- If so, **initial text** is set to the text of the temp file
		- Next, if the ID is present in the `map` `m_extantPathsToIds`, then we read the file at the extant path for the contents of **original text**
		- The recovered texts are used to make a new `TextDocument` that is then inserted into the cache (which, if we did any recovering at all, means the cache was already full, so another document is evicted to make room)
	- The served document has its text set.
	- A temporary save (named for the outgoing ID) is created
- `Document` sets its **current ID** by `path`:
	- ID is retrieved if it's present in the member `map` `m_extantPathsToIds`
	- If not present, a new ID is created, adding `map[path] = id`, and the new ID is added to a lifetime-of-the-application list (containing all IDs, which may or may not have corresponding paths in the `map`)
- [...]
- `Document` returns a string from the "current" document:
	- It searches by (new) **current ID** and `path`
	- It sends a signal to start/restart the auto save timer, the timing out of which triggers the above process (of saving the current tab's text to a document object + a temp file) every 25 seconds
- `MainWindow->TabBar` then serves a tab (using `Documents` **current ID** with the intended `path` (for displaying the title on the tab)
	- `TabBar` calls the function to get or make an index via ID
	- In this case, it will not find one, so it will create one, passing in both arguments
	- We temporarily block signals and insert a new tab, returning its index
	- The new tab is titled after the `path`, but `path` isn't used otherwise
	- The new tab's data is a variant map containing its **ID** (same as the corresponding document, which is either in the cache or preserved as a temp file to be recovered), and its title
	- `TabBar` unblocks signals and sets its current index to the new tab
- `MainWindow->Editor` sets its current text to the string retrieved from `Document` above

#### Clicking open file:

- AKA, opening a **named file by path** and switching to (if already opened) or creating its tab and document

- `Open file` opens file section dialog and sends the `path` to `MainWindow` via signal
- The same initial `MainWindow` function above is run (opening a new file), minus the argument for writing a new file
- Instead, `MainWindow->Document` sets the current `TextDocument` by `path`
- `Document` runs through the same processes as above, returning a `QString` to be sent, by `MainWindow`, to the `Editor` after first serving a (possibly new) tab
- If a user selects a currently opened tab via `MenuBar`'s dialog, then `MainWindow->TabBar->serve` will just find the extant index and switch to it

#### Clicking save file:

- AKA asking to save file represented by current tab (with current text)
- `Save file` signals
- `MainWindow->Document` checks that the **current ID** is not `null` and that the corresponding document is edited.
- If both are true, then `MainWindow->Document->save` is called, showing `Indicator` red/green for success/failure
- ...

### By `TabBar` interaction:

#### Clicking on a tab:

- AKA, opening (switching to, visually) a **named or unnamed file's document text by ID**

- Clicking on a tab changes `TrueTabBar`'s current index
- Its container, the overarching `TabBar` (holding the tab bar + its controls) emits a signal with the new, current `index`'s **ID**
- `MainWindow->Document` sets its current document by this **ID**, returning a `QString` that's then sent to `Editor`
- (`Document` again runs through the same processes to save before returning the string)
- If the document was evicted, then recovery should find initial text (for the document) and, if the document was created as a file from disk, original text (for checking/setting edited state)

#### Clicking "add tab":

- AKA, opening an **unnamed, non-existent, pathless file :pensive:**

- Clicking the `+` button causes `TabBar` to emit a signal asking `MainWindow` to start the process
- `MainWindow->Document` creates an empty document, returning a **new ID**
	- It first makes an ID, which is added only to the **lifetime registry**, and then sends it through the document retrieval function (args: a new ID, and no path)
	- The search of the cache will come up empty, and so the new document will be created
	- In the creation process, we'll start out again with 2 strings (**intial** and **original** texts)
	- The **ID** will not be found to have been evicted
	- The path is empty
	- So, the strings remain empty and are added to a new `TextDocument`, which is cached under the new **ID**
- `MainWindow->Document` then sets its current document to newly created `TextDocument` via the returned **ID**
- `MainWindow->TabBar` is called to serve a tab
	- Since the **ID** is new, signals are blocked, and a new tab is inserted
	- The tab's variant map data is given the **ID** and an empty `QString` in place of the title
	- Signals are unblocked and `TrueTabBar`'s current index is switched to the new tab's
	- This causes everything in the above section ("Clicking on a tab") to happen
- `MainWindow->Editor` clears itself for the new tab


