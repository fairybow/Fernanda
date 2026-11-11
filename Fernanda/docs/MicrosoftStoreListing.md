# Store Listing

The Microsoft Store listing's text, kept here so it changes with the app. Each section is one field in Partner Center, under the product's Store listings (English). The Store shows plain text: no Markdown, links, or bullets of its own, so paste the text inside each block as it stands.

The package itself, and how it's built, are in `.github/msix/` and `.github/scripts/BuildMsix.ps1`.

## Product name

Fernanda (the reserved name; chosen from the drop-down)

## Short description

Shown at the top of the listing. Up to 1,000 characters, but some views show only the first 270, so it stays under that.

```
A writing app for fiction, poetry, and long projects, where everything you write stays in plain-text files you own.
```

## Description

Required. Up to 10,000 characters. The features list below shows beside it, so this says what Fernanda is for rather than listing everything.

The first sentence marks "open-source" with an asterisk, and the last paragraph is its footnote: Fernanda is free on GitHub, so no one buys it without knowing. It says "open-source" rather than "free" up front, since "free" beside a price could read as a claim about the price. It names GitHub but gives no address: Paint.NET's listing, which has sold a paid edition beside its free download for years, frames a purchase as support and doesn't point to the free download at all, so an address would go further than any listing known to pass certification.

```
Fernanda is open-source* software built for writing projects: novels, poetry, collections. A project is a vault, and a vault is just a folder on your computer. Everything you write stays in plain-text files you own, which any other program can open. Fernanda adds one hidden folder for its settings and leaves the rest alone.

There's no Save command and nothing to lose. Every file exists on disk from the moment it's created, and changes are written as you type. Your text is written back exactly as it was read, line endings and special characters included.

Its design follows Obsidian's, so if you've used Obsidian, vaults, tabs, splits, Go to File, and the settings will feel familiar. Where Fernanda differs, it's for long-form prose: plain .txt files come first, a Common Vault holds the notes and research every project shares, and Git is built in for writers who keep their work in a repository.

Fernanda never sends your writing anywhere on its own. Git reaches your repository's remote only when you click to sync.

Fernanda is still in beta, and new features should arrive somewhat regularly. Keep backups of your work.

Fernanda is an independent project and isn't affiliated with or endorsed by Obsidian.

See CHANGELOG.md in Fernanda's GitHub repository for details on the latest changes.

*Fernanda is free and open source, under the GNU GPL, and you can download it from GitHub at no cost. If you buy it here, you're supporting its development directly, and I love you for it. You also get easy installation and automatic updates through the Microsoft Store, with every new feature and fix. In Ukraine, Fernanda is free in the Microsoft Store.
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
Writing history for each vault: words written, net words, and time spent writing, from every computer you write on
PDFs and images open beside your text
Brackets and quotes pair as you type
A word counter, fonts chosen for prose, and settings kept per vault
In English, with a Spanish translation in progress
```

## What's new in this version

Leave blank.

## Keywords

Up to 7 terms, each up to 40 characters, and 21 different words in all. Not shown to customers.

```
novel writing software
poetry writing tool
long-form prose editor
plain text editor
distraction free writing
fiction writing
manuscript
```

## Screenshots

At least one, and up to ten, each at least 1366 × 768. The screenshots are smaller than that, so each window is centered on the brand gradient, with the website's shadow, at its own size rather than scaled up. The files are in `Fernanda/resources/microsoft-store/`, numbered in upload order:

| File | Shows |
| --- | --- |
| `01-WindowComposite.png` | The vault window, split diagonally between light and dark |
| `02-VaultLight.png`, `03-VaultDark.png` | A chapter open beside the file tree and the Common Vault drawer |
| `04-SplitLight.png`, `05-SplitDark.png` | Two panes: a chapter and a PDF |
| `06-GitLight.png`, `07-GitDark.png` | The Git page, with a diff open |
| `08-SettingsLight.png`, `09-SettingsDark.png` | Settings, on the Editor page |
| `10-ManageVaultsLight.png` | Manage Vaults |

## Store logos

Made from `Fernanda/resources/icons/designs/Fernanda.svg`, in `Fernanda/resources/microsoft-store/`. Where a slot gives two sizes, it takes exactly one of them, and the larger is used.

| Slot | File |
| --- | --- |
| 9:16 Poster art, the main logo on Windows | `PosterArt-1440x2160.png`: the ring on the brand gradient |
| 1:1 Box art | `BoxArt-2160.png`: the ring on the brand gradient |
| Store display images: 1:1 App tile icon, 300 × 300 | `AppTileIcon-300.png`: the ring on transparency |
| Store display images: 1:1, 150 × 150 | `AppTileIcon-150.png` |
| Store display images: 1:1, 71 × 71 | `AppTileIcon-71.png` |

The trailer, the 16:9 Super hero art, and the Xbox images stay empty. The hero art is only needed with a trailer, and Fernanda isn't offered on Xbox.

## Additional information

| Field | Value |
| --- | --- |
| Copyright and trademark info | Copyright (C) 2026 fairybow |
| Additional license terms | https://github.com/fairybow/Fernanda/blob/main/LICENSE |
| Developed by | fairybow |

A single URL in the license terms field shows as a link. Leaving it empty would put the app under Microsoft's Standard Application License Terms instead, which add restrictions the GPL doesn't allow.

## Pricing and availability

Every market, at a base price of $4.99, which Partner Center converts into each market's currency. Ukraine is its own market group at a price of 0, so it's free there. There's no free trial: the free download already lets anyone try it.

## Properties

| Field | Value |
| --- | --- |
| Category | Productivity |
| Privacy policy URL | https://github.com/fairybow/Fernanda/blob/main/PRIVACY.md |
| Website | https://github.com/fairybow/Fernanda |
| Support contact | https://github.com/fairybow/Fernanda/issues |

## Age ratings

The questionnaire asks what the app contains and what it lets people do. For Fernanda the answer to each is no: it has no violence, sexual content, language, gambling, or drugs of its own; people don't talk to, share with, or find each other through it; it doesn't share a location; and it sells nothing inside the app. What a user writes is their own files, not content the app shows others. This should give the lowest rating in every region.
