# <img src="Fernanda/res/icons/Fernanda.ico" alt="Colorful conch shell icon." width="26px"/> Fernanda
<a id="top"></a>

<p align="center">
	<kbd>
		<img src="./Fernanda/docs/screens/main_screen.png" alt="PNG of 'Fernanda v0.14.0-beta32'"/>
		<br>Fernanda v0.14.0-beta32
	</kbd>
</p>
<p align="center">
	<a href="https://github.com/fairybow/Fernanda/releases/"><img src="https://img.shields.io/github/v/release/fairybow/Fernanda?include_prereleases&color=f34b7d" alt="Latest Release"/></a>
	<a href="LICENSE"><img src="https://img.shields.io/github/license/fairybow/Fernanda?color=orange" alt="License: GPL-3.0"/></a>
	<br>
	<a href="https://www.qt.io/"><img src="https://img.shields.io/badge/Qt-v6.4.2-brightgreen?logo=qt" alt="Qt v6.4.2"/></a>
	<a href="https://github.com/rikyoz/bit7z"><img src="https://img.shields.io/badge/Bit7z-v4.0.0--RC-blue" alt="Bit7z v4.0.0-RC"/></a>
	<a href="https://www.7-zip.org/"><img src="https://img.shields.io/badge/7zip-v22.01-ffbf00" alt="7zip v22.01"/></a>
	<br>
	<a href="#install"><img src="https://img.shields.io/badge/Windows%20(x64)-5B5B5B?logo=windows" alt="Platform Windows (x64)"/></a>
	<a href="#install"><img src="https://img.shields.io/badge/Arch%20Linux-5B5B5B?logo=archlinux" alt="Platform Arch Linux"/></a>
	<a href="https://github.com/fairybow/Fernanda/search?l=c%2B%2B"><img src="https://img.shields.io/badge/C%2B%2B20-5B5B5B?logo=c%2B%2B" alt="C++20"/></a>
	<br>
	<a href="https://shields.io/"><img src="https://img.shields.io/badge/omg-these%20are%20so%20cute-00b3b3"/></a>
</p>

## :tea: **Hello**

Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)

- [About](#heartpulse-about)
- [Build](#honeybee-build)
- [Features](#star-features)
	- [Key filters](#features-key_filters)
	- [Shortcuts](#features-shortcuts)
	- [`.story`](#features-files)
	- [Tools](#features-tools)
- [Installation](#floppy_disk-installation)
- [Roadmap](#compass-roadmap)
- [Thanks](#smiley_cat-thanks)
- [Screens](#hibiscus-screens)
- [Bye](#heartpulse-bye)

## :heartpulse: **About**
<a id="heartpulse-about"></a>

This is a personal project, a work-in-progress, and I am *so* not a programmer. Still, I decided I didn't like existing novel-writing software very much, and I wanted to make something all my own. My hope is that it's easy to use, lightly-customizable, and distraction-free, for faster, more peaceful drafting.

You can try it [here](#install).

Fernanda's look was inspired by the nice and quiet feeling of using [WordStar](https://en.wikipedia.org/wiki/WordStar) on [DOSBox](https://www.dosbox.com/) to draft, and its interface was inspired by [Atom](https://github.com/atom/atom).

Fernanda got its name because I just really like the name a lot. But, as it turns out, Fernanda means an ["adventurous, bold journey"](https://en.wikipedia.org/wiki/Fernanda) (the kind one might be *called* to, say), and I think that's neat. <img src="./Fernanda/res/icons/Fernanda.ico" alt="Colorful conch shell icon." width="16px"/>

[:leftwards_arrow_with_hook:](#top)

## :honeybee: **Build**
<a id="honeybee-build"></a>

Fernanda is built with:
- C++
- [Qt](https://www.qt.io/)
- [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/)
- and [NSIS](https://nsis.sourceforge.io/)

Fernanda relies on:
- Bit7z by [@rikyoz](https://github.com/rikyoz/bit7z)
- and, thus, 7-zip by [Igor Pavlov](https://www.7-zip.org/)

Our current default font is:
- Mononoki by [@madmalik](https://github.com/madmalik/mononoki)

Among the other fonts used are:
- Cascadia Mono by [@microsoft](https://github.com/microsoft/cascadia-code)
- Cozette by [@slavfox](https://github.com/slavfox/Cozette) (vectorized bitmap font, so looks crisp only at certain sizes)
- Day Roman by Apostrophic Labs
- Dot Matrix by [Dionaea Fonts](https://dionaea.com/information/fonts.php) (`Help > Create sample themes...`)
- Fixedsys Excelsior by [@kika](https://github.com/kika/fixedsys)
- Iosevka by [@be5invis](https://github.com/be5invis/Iosevka)
- More Perfect DOS VGA by [Zeh Fernando](https://zehfernando.com/) and [@L∆MEUR](https://laemeur.sdf.org/fonts/)
- Nouveau IBM by Arto Hatanp‰‰
- OpenDyslexic by [Abbie Gonzalez](https://opendyslexic.org/)
- and Ysabeau by [@CatharsisFonts](https://github.com/CatharsisFonts/Ysabeau)

Solarized theme palettes by [@altercation](https://github.com/altercation/solarized)

[Markdown](https://www.markdownguide.org/)/[Fountain](https://fountain.io/) preview support provided with:
- [Markdown-it](https://github.com/markdown-it/markdown-it)
	- Several Markdown-it plugins, like:
		- markdown-it-source-map by [@tylingsoft](https://github.com/tylingsoft/markdown-it-source-map)
- github-markdown-css by [@sindresorhus](https://github.com/sindresorhus/github-markdown-css)
- and Fountain.js by [@mattdaly](https://github.com/mattdaly/Fountain.js)

[:leftwards_arrow_with_hook:](#top)

## :star: **Features**
<a id="star-features"></a>

**General:**
- Most things are togglable
- Settings are auto-saved
- Incorporation of custom themes and fonts (`.ttf` and `.otf`)
- Examples (project and custom themes and font) available via the `Help` menu
- Save backups (in [`{userdata}\backup\.rollback`](#folders))
- Global line, word, and character totals
- Export to PDF, text, or directory
- Markdown/Fountain preview

---

**Key filters:**
<a id="features-key_filters"></a>
- Auto-closing for `"", (), {}, []`
- 2 spaces will skip the cursor past a closing item, closing the gap (see below)
- Auto em-/en-dash formatting from hyphen/minus key

<p align="center">
	<kbd>
		<img src="./Fernanda/docs/screens/key_filters_1.gif" alt="GIF of 'Key filters'"/>
		<br>Key filters
	</kbd>
</p>

---

**Shortcuts:**
<a id="features-shortcuts"></a>
- `F11`: Cycle editor themes (Amber, Green, Grey)
- `Alt` + `F10`: Cycle fonts
- `Alt` + `F11`: Cycle editor themes (all)
- `Alt` + `F12`: Cycle window themes
- `Alt` + `Insert`: Nav previous
- `Alt` + `Delete`: Nav next
- `Alt` + `Minus (-)` / `Ctrl` + `Mouse Wheel Down`: Decrease font size
- `Alt` + `Plus (+)` / `Ctrl` + `Mouse Wheel Up`: Increase font size
- `Ctrl` + `Y`: Redo
- `Ctrl` + `Z`: Undo
- `Ctrl` + `Shift` + `C`: Wrap selection or block in quotes

---

**`.story`:**
<a id="features-files"></a>
- They're 7-zip (`.7z`, non-compressed) archives
- They'll be associated with Fernanda by the installer and can be opened in the usual ways
- They can also be opened with [7-zip](https://www.7-zip.org/), and the contents viewed and/or copied elsewhere
- Items deleted (cut) within Fernanda are moved to a `.cut` folder within the `.story` archive

If you want to manually rename, move, or delete items within a `.story` file via 7-zip, be sure to delete `story.xml`, too, so that it can be remade on next open. (You will lose any reorderings that are at the same directory level, as well as same-level, file-on-file parenting).

---

**Tools:**
<a id="features-tools"></a>
- :pushpin: **Always-on-top:**
	- Pin Fernanda to the top of your window order (will interfere with popups!)
- :bubble_tea:	**Stay awake:**
	- Keep the screen awake without input (Windows only)
- :timer_clock: **Timer:**
	- A silent countdown timer (left-click to start or pause; right-click to reset)

<p align="center">
	<kbd>
		<img src="./Fernanda/docs/screens/timer_1.gif" alt="GIF of 'Timer'"/>
		<br>Timer
	</kbd>
</p>

[:leftwards_arrow_with_hook:](#top)

## :floppy_disk: **Installation**
<a id="floppy_disk-installation"></a>

You can grab the installer on the [Releases](https://github.com/fairybow/Fernanda/releases/) page. (AUR [here](https://aur.archlinux.org/packages/Fernanda).)

**Update:**
- Run the latest installer and overwrite.

**Remove:**
- Run the uninstaller, found in the installation folder
- Or remove the installation folder (however, running the uninstaller should also remove file association / certain registry information)

Fernanda should create the following folders on your OS:

**Folder locations:**
<a id="folders"></a>
- `%HOMEPATH%\.fernanda`
	- e.g. `C:\Users\{username}\.fernanda`
	- This is the user data folder (using "-dev" creates a separate folder, `.fernanda (dev)`)
	- Backup (`.bak`) saves are here
- `%PROGRAMFILES%\Fernanda`
	- e.g. `C:\Program Files\Fernanda`
	- The default install location
- `%HOMEPATH%\Documents\Fernanda`
	- e.g. `C:\Users\{username}\Documents\Fernanda`
	- Where `.story` files are kept

They can be opened via the `Help` menu.

[:leftwards_arrow_with_hook:](#top)

## :compass: **Roadmap**
<a id="compass-roadmap"></a>

Fernanda is available for Windows (beta pre-release) [above](#install), and an early Arch Linux package is also available on the AUR (courtesy of [@philipplenk](https://github.com/philipplenk)).

Right now, I'm kind of going where the wind takes me. Fernanda is for drafting. Specifically, for me, it's for encouraging a more productive drafting headspace, and any features that seem like a must for making that happen easier, I'm going to try and add.

In my mind, near-finished work can be revised in other programs. So, some things you might find in other binder-style writing programs aren't necessarily on the table for me right now (but also not necessarily out-of-the-question, either). I'm not interested in adding spellcheck, for example. I think for my purposes it would be distracting. Nor am I interested in any kind of highly-involved formatting or pre-publishing processes. I am, however, planning on adding a way to mark files for compilation for general export (one big file, or several if you want, where things more drafty than others, or notes, can be left out). Ways to view outlines or organize smaller notes or scraps (or even map them) is not necessarily out-of-the-question.

If I think of something big, I'll try to add it right here. Otherwise, these things, of varying importance or levels of commitment, will also appear in [To-do.md](./Fernanda/docs/To-do.md).

- Persist undo/redo stacks between file changes
- Separators and possibly customizable file icons, for better mental organization at-a-glance
- Simple storage of links and research materials
- Markdown/Fountain export
- Scroll sync for Fountain between editor and preview
- Pane, preview, status bar, and file menu auto-collapse (and expand on area hover)

More to come.

[:leftwards_arrow_with_hook:](#top)

## :smiley_cat: **Thanks**
<a id="smiley_cat-thanks"></a>

A major thanks to [@philipplenk](https://codemetas.de/) for their teaching and interest and support in helping me work toward a small dream like this (and for helping me make Fernanda available for [Arch Linux](https://aur.archlinux.org/packages/Fernanda)).

A major thanks, too, to [@rikyoz](https://github.com/rikyoz/) for their tireless work on the very amazing [Bit7z library](https://github.com/rikyoz/bit7z).

[:leftwards_arrow_with_hook:](#top)

## :hibiscus: **Screens**
<a id="hibiscus-screens"></a>

<table>
	<thead>
		<tr>
			<th colspan="2">
				<p align="center">
					<kbd>
						<img src="./Fernanda/docs/screens/banner_old.png" alt="PNG of 'Banner'"/>
						<br>Old banner<br><br>Themes: window Light / editor Pocket;<br>Font: Fixedsys Excelsior by <a href="https://github.com/kika/fixedsys">@kika</a>.
					</kbd>
				</p>
			</th>
		</tr>
	</thead>
	<thead>
		<tr>
			<th colspan="2">
				<p align="center">
					<kbd>
						<img src="./Fernanda/docs/screens/main_screen.png" alt="PNG of 'Fernanda v0.14.0-beta32 running on Windows 11.'"/>
						<br>Fernanda v0.14.0-beta32 running on Windows 11.<br><br>Wallpaper: "Red sky background" by <a href="https://www.deviantart.com/masterteacher/art/Red-sky-background-356199141">MasterTeacher</a>;<br>Themes: window Solarized Light / editor Snooze;<br>Font: Mononoki by <a href="https://github.com/madmalik/mononoki">@madmalik</a>.
					</kbd>
				</p>
			</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>
				<p align="center">
					<kbd>
						<img src="./Fernanda/docs/screens/light_pocket.png" alt="PNG of 'Fernanda v0.14.0-beta32 - Themes: window Light / editor Pocket'"/>
						<br>Fernanda v0.14.0-beta32<br>Themes: window Light / editor Pocket
					</kbd>
				</p>
			</td>
			<td>
				<p align="center">
					<kbd>
						<img src="./Fernanda/docs/screens/dark_pocket_alt.png" alt="PNG of 'Fernanda v0.14.0-beta32 - Themes: window Dark / editor Pocket-Alt'"/>
						<br>Fernanda v0.14.0-beta32<br>Themes: window Dark / editor Pocket-Alt
					</kbd>
				</p>
			</td>
		</tr>
		<tr>
			<td>
				<p align="center">
					<kbd>
						<img src="./Fernanda/docs/screens/solarized_light_snooze_alt.png" alt="PNG of 'Fernanda v0.14.0-beta32 - Themes: window Solarized Light / editor Snooze-Alt'"/>
						<br>Fernanda v0.14.0-beta32<br>Themes: window Solarized Light / editor Snooze-Alt
					</kbd>
				</p>
			</td>
			<td>
				<p align="center">
					<kbd>
						<img src="./Fernanda/docs/screens/solarized_dark_snooze.png" alt="PNG of 'Fernanda v0.14.0-beta32 - Themes: window Solarized Dark / editor Snooze'"/>
						<br>Fernanda v0.14.0-beta32<br>Themes: window Solarized Dark / editor Snooze
					</kbd>
				</p>
			</td>
		</tr>
	</tbody>
</table>

[:leftwards_arrow_with_hook:](#top)

## :heartpulse: **Bye**
<a id="heartpulse-bye"></a>

[:leftwards_arrow_with_hook:](#top)
