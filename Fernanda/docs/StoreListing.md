# Store Listing

The Microsoft Store listing's text, kept here so it changes with the app. Each section is one field in Partner Center, under the product's Store listings (English). The Store shows plain text: no Markdown, links, or bullets of its own, so paste the text inside each block as it stands.

The package itself, and how it's built, are in `.github/msix/` and `.github/scripts/BuildMsix.ps1`.

## Product name

Fernanda (the reserved name; chosen from the drop-down)

## Short description

Shown at the top of the listing. Up to 1,000 characters, but some views show only the first 270, so it stays under that.

```
A writing app for fiction, poetry, and long projects. Your work is plain-text files in a folder you choose, saved as you type, with tabs, split panes, spellcheck, a focus timer, and Git built in.
```

## Description

Required. Up to 10,000 characters. The features list below shows beside it, so this says what Fernanda is for rather than listing everything.

```
I'm a writer, and Fernanda is the writing app I've wanted for myself.

Fernanda is built for writing projects: novels, poetry, collections. A project is a vault, and a vault is just a folder on your computer. Everything you write stays in plain-text files you own, which any other program can open. Fernanda adds one hidden folder for its settings and leaves the rest alone.

There's no Save command and nothing to lose. Every file exists on disk from the moment it's created, and changes are written as you type. Your text is written back exactly as it was read, line endings and special characters included.

Its design follows Obsidian's, so if you've used Obsidian, vaults, tabs, splits, Go to File, and the settings will feel familiar. Where Fernanda differs, it's for long-form prose: plain .txt files come first, a Common Vault holds the notes and research every project shares, and Git is built in for writers who keep their work in a repository.

Fernanda never sends your writing anywhere on its own. Git reaches your repository's remote only when you click to sync.

Fernanda is still in beta, and new features should arrive somewhat regularly. Keep backups of your work.

Fernanda is free and open source, under the GNU GPL. The same app is free to download on GitHub, along with its source code. Buying it here supports its development and gets you updates through the Microsoft Store.

Fernanda is an independent project and isn't affiliated with or endorsed by Obsidian.
```

## Product features

Up to 20, each up to 200 characters. Partner Center adds the bullets.

```
A vault is a folder: open any folder and start writing
No Save command: every change is written to disk as you type
Your text comes back exactly as it went in, line endings and special characters included
Tabs, split panes, and pop-out windows, with one file open in as many as you like, in sync
A Common Vault shared by every project, for templates, notes, and reference material
A file tree and Go to File, with drag-and-drop moves and delete to the Recycle Bin
Find and replace, with match case and whole word
Spellcheck as you write, with a dictionary of your own words for each vault
Files changed by other programs, Git, or a sync service reload in place, as one undo step
Git built in: see what changed, commit, branch, and sync, using the Git on your computer
A focus timer for writing in focus periods, with short and long breaks
PDFs and images open beside your text
Brackets and quotes pair as you type
A word counter, fonts chosen for prose, and settings kept per vault
In English, with a Spanish translation in progress
```

## What's new in this version

Up to 1,500 characters. Leave it blank for the first submission. For each update after, a short plain-text summary of that version's entry in `CHANGELOG.md`.

## Keywords

Up to 7 terms, each up to 40 characters, and 21 different words in all. Not shown to customers.

```
writing
novel
plain text editor
fiction writing
poetry
distraction free writing
manuscript
```

## Screenshots

At least one, and up to ten. Partner Center checks each image's size as it's uploaded. Taken at a normal window size, with a vault of sample writing rather than real work:

1. A vault window with a chapter open, the file tree beside it, and the Common Vault drawer showing
2. Two panes side by side: a draft and its notes
3. Find and replace in use
4. The Git page, with a changed file's diff open
5. Settings, on the Editor page
6. Manage Vaults

## Store logos

The 1:1 box art is required; Partner Center names its sizes on the page. Made from `Fernanda/resources/icons/designs/Fernanda.svg`, the ring centered on a plain background.

## Additional information

| Field | Value |
| --- | --- |
| Copyright and trademark info | Copyright (C) 2026 fairybow |
| Additional license terms | https://github.com/fairybow/Fernanda/blob/main/LICENSE |
| Developed by | fairybow |

A single URL in the license terms field shows as a link. Leaving it empty would put the app under Microsoft's Standard Application License Terms instead, which add restrictions the GPL doesn't allow.

## Properties

| Field | Value |
| --- | --- |
| Category | Productivity |
| Privacy policy URL | https://github.com/fairybow/Fernanda/blob/main/PRIVACY.md |
| Website | https://github.com/fairybow/Fernanda |
| Support contact | https://github.com/fairybow/Fernanda/issues |

## Age ratings

The questionnaire asks what the app contains and what it lets people do. For Fernanda the answer to each is no: it has no violence, sexual content, language, gambling, or drugs of its own; people don't talk to, share with, or find each other through it; it doesn't share a location; and it sells nothing inside the app. What a user writes is their own files, not content the app shows others. This should give the lowest rating in every region.
