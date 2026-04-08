/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QString>

#include "core/Version.h"

// TODO: Go through and Find All References and delete unused
// TODO: Set auto lupdate in CMake to remove unused translations

// Qt Linguist requires the tr method to be in place
#define TR_(Name, TrCall)                                                      \
    inline QString Name() { return TrCall; }

// Numerus
#define TRN_(Name, TrCall)                                                     \
    inline QString Name(int n) { return TrCall; }

namespace Fernanda {

QString
tr(const char* sourceText, const char* disambiguation = nullptr, int n = -1);

// NP = Notepad; NB = Notebook; NX = Either
// TODO: Use a coherent naming scheme...
// TODO: Need manual wrap for some of these
namespace Tr {

    // --- Files.h ---
    // TODO: Separate filter names?
    // NB: Most of these won't need to be translated, unless they contain
    // "document", "file", etc - but if one lives here, they all should

    TR_(filesPlainText, tr("Plain text"));
    TR_(filesNotebook, tr("Fernanda Notebook"));
    TR_(filesMarkdown, tr("Markdown source"));
    TR_(filesFountain, tr("Fountain screenplay"));
    TR_(filesHtml, tr("Hypertext Markup Language (HTML)"));
    TR_(filesPdf, tr("Portable Document Format (PDF)"));
    TR_(filesPng, tr("Portable Network Graphics (PNG)"));
    TR_(filesJpeg, tr("Joint Photographic Experts Group (JPEG)"));
    TR_(filesGif, tr("Graphics Interchange Format (GIF)"));
    TR_(filesTiff, tr("Tagged Image File Format (TIFF)"));
    TR_(filesBmp, tr("Bitmap (BMP)"));
    TR_(filesWebP, tr("WebP"));
    TR_(filesMsWord, tr("Microsoft Word (Office Open XML) document"));
    TR_(filesRichText, tr("Rich Text Format (RTF)"));
    TR_(filesWindowTheme, tr("Fernanda Window Theme"));
    TR_(filesEditorTheme, tr("Fernanda Editor Theme"));

    TR_(filesFilterAll, tr("All files"));

    // --- Workspace.h ---

    TR_(workspaceImport, tr("Import..."));
    TR_(workspaceImportCaption, tr("Import files"));
    TR_(workspaceOpenNotepad, tr("Open notepad"));

    /// === ORGANIZE THE BELOW ===

    /// General

    TR_(notepad, tr("Notepad"));
    TR_(nbTrash, tr("Trash"));
    TR_(fileMetaNotOnDisk, tr("[Not on disk]"));
    TR_(fileMetaStale, tr("[Stale]"));
    TR_(fileModifiedExternally,
        tr("This file has been modified outside Fernanda"));
    TR_(filePathInvalidated,
        tr("This file has been renamed, moved, or deleted outside Fernanda"));

    /// Buttons

    TR_(ok, tr("OK"));
    // TR_(licenses, tr("Licenses"));
    TR_(aboutQt, tr("About Qt"));
    TR_(save, tr("Save"));
    TR_(dontSave, tr("Don't save"));
    TR_(cancel, tr("Cancel"));

    /// Dialogs

    // NX dialogs:

    TR_(nxAboutTitle, tr("About"));
    TR_(nxNewNotebookTitle, tr("New Notebook"));
    TR_(nxNewNotebookBody, tr("Name:"));
    TR_(nxNewNotebookExistsErrBodyFormat,
        tr("A Notebook already exists at %0"));
    TR_(nxOpenNotebookCaption, tr("Open Notebook file"));

    TR_(nxBetaAlert,
        tr("<p><b>This is a prerelease!</b></p><p>You should not trust your "
           "writing with prerelease versions of this software. Regardless, "
           "always make regular backups of your work.</p>"));

    // TODO: std::format
    inline QString nxAboutBody()
    {
        auto arg_0 = [] {
            return tr(
                "<b>Fernanda</b> is a plain text editor for drafting "
                "long-form fiction. (At least, that's the plan.)");
        };

        auto arg_1 = [] {
            return tr("It's a personal project and a work-in-progress.");
        };

        auto arg_2 = [] {
            return tr("See <a href=\"%0\">%0</a> for more information.");
        };

        auto arg_3 = [] {
            return tr("<b>Version</b> %0 (%1)");
        };

        auto body = [] {
            return QString("<p>%0</p><p>%1</p><p>%2</p><p>%3</p>");
        };

        return body()
            .arg(arg_0())
            .arg(arg_1())
            .arg(arg_2().arg(VERSION_DOMAIN))
            .arg(
                arg_3()
                    .arg(VERSION_FULL_STRING)
                    .arg(VERSION_RELEASE_NAME_STRING));
    }

    TR_(nxSavePromptBodyFormat, tr("Do you want to save changes to %0?"));
    TR_(nxSavePromptMultiBodyFormat,
        tr("You have unsaved changes in %0 files:"));

    TR_(nxSaveFailBoxBodyFormat, tr("There was a problem saving %0"));
    TR_(nxSaveFailBoxMultiBodyFormat,
        tr("There was a problem saving these files: %0"));

    TR_(nxUpdateTitle, tr("Check For Updates"));
    TR_(nxUpdateReleasesButton, tr("Visit releases page"));
    TR_(nxUpdateLatestFormat, tr("You have the latest version (%0)"));
    TR_(nxUpdateOutOfDateFormat,
        tr("Your version (%0) is not the latest (%1)."));
    TR_(nxUpdateFailAssetFetch, tr("Asset fetch failed: %0"));
    TR_(nxUpdateFailReleaseFetchFormat, tr("Release fetch failed: %0"));
    TR_(nxUpdateFailJsonParseFormat, tr("JSON parse failed: %0"));
    TR_(nxUpdateFailNoReleasesFound, tr("No releases found!"));
    TR_(nxUpdateFailMissingAsset,
        tr("No Version.txt asset found in latest release!"));
    TR_(nxReloadPromptBodyFormat,
        tr("This content of %0 has changed outside Fernanda. Reload?"));
    TR_(nxReloadPromptReload, tr("Reload"));
    TR_(nxReloadPromptKeep, tr("Keep mine"));

    // NP dialogs:

    TR_(npOpenFileCaption, tr("Open file"));
    TR_(npSaveAsCaption, tr("Save as"));

    // NB dialogs:

    TR_(nbExportFileCaption, tr("Export file"));
    TR_(nbSaveAsCaption, tr("Save as"));

    // TODO: I think a multi version might be better - then, in the singular, we
    // can name the file (instead of weirdly saying "would you like to delete 1
    // file?")
    TRN_(
        nbTrashPromptBody,
        tr("Are you sure you want to delete %n item(s)? <b>This cannot be "
           "undone.</b>",
           nullptr,
           n));

    /// Menus

    // TODO: Mnemonics

    // NX menus:

    TR_(nxFileMenu, tr("&File"));
    TR_(nxEditMenu, tr("&Edit"));
    TR_(nxViewMenu, tr("&View"));
    TR_(nxSettingsMenu, tr("&Settings"));
    TR_(nxHelpMenu, tr("&Help"));

    TR_(nxNew, tr("New"));

    TR_(nxNewTab, tr("New tab (Plain Text)"));

    TR_(nxNewWindow, tr("New window"));
    TR_(nxNewNotebook, tr("New notebook"));
    TR_(nxOpenNotebook, tr("Open notebook"));

    TR_(nxSave, tr("Save"));
    TR_(nxSaveAs, tr("Save as..."));

    TR_(nxDuplicateTab, tr("Duplicate tab"));

    TR_(nxClose, tr("Close"));

    TR_(nxCloseTab, tr("Close tab"));
    TR_(nxCloseTabEverywhere, tr("Close tab everywhere"));
    TR_(nxCloseWindowTabs, tr("Close window tabs"));
    TR_(nxCloseAllTabs, tr("Close all tabs"));
    TR_(nxCloseWindow, tr("Close window"));
    TR_(nxCloseAllWindows, tr("Close all windows"));

    TR_(nxQuit, tr("&Quit"));

    TR_(nxUndo, tr("&Undo"));
    TR_(nxRedo, tr("&Redo"));
    TR_(nxCut, tr("Cu&t"));
    TR_(nxCopy, tr("&Copy"));
    TR_(nxPaste, tr("&Paste"));
    TR_(nxDelete, tr("&Delete"));
    TR_(nxSelectAll, tr("&Select all"));

    // This action's title is set in TreeViewService
    TR_(nxTreeView, tr("Tree view"));

    TR_(nxAbout, tr("About"));
    TR_(nxCheckForUpdates, tr("Check for updates..."));

    // NP menus:

    TR_(npOpenFile, tr("Open..."));
    TR_(npSaveAllInWindow, tr("Save all in window"));
    TR_(npSaveAll, tr("Save all"));

    // NB menus and context menu:

    TR_(nbNewFile, tr("New file (Plain Text)"));
    TR_(nbNewFolder, tr("New folder"));

    // NB context menu:

    TR_(nbCollapse, tr("Collapse"));
    TR_(nbExpand, tr("Expand"));
    TR_(nbRename, tr("Rename"));
    TR_(nbRemove, tr("Remove"));
    TR_(nbRestore, tr("Restore"));
    TR_(nbDeletePermanently, tr("Delete permanently"));
    TR_(nbExport, tr("Export..."));
    TR_(nbEmptyTrash, tr("Empty trash"));

    /// Word counter

    TRN_(wordCounterLines, tr("%n line(s)", nullptr, n));
    TRN_(wordCounterWords, tr("%n word(s)", nullptr, n));
    TRN_(wordCounterChars, tr("%n char(s)", nullptr, n));
    TR_(wordCounterLinePos, tr("ln"));
    TR_(wordCounterColPos, tr("col"));

    /// Font panel

    TR_(fontPanelTitle, tr("Font"));
    TR_(fontPanelBold, tr("Bold"));
    TR_(fontPanelItalic, tr("Italic"));

    /// Themes panel

    TR_(themesPanelTitle, tr("Themes"));
    TR_(themesPanelEditorTheme, tr("Editor theme"));
    TR_(themesPanelWindowTheme, tr("Window theme"));

    /// Key filter panel

    TR_(keyFiltersPanelTitle, tr("Key filters"));
    TR_(keyFiltersPanelAutoClose, tr("Auto-close"));
    TR_(keyFiltersPanelBarging, tr("Barging"));
    TR_(keyFiltersPanelBargingTooltip,
        tr("Double-spacing (or pressing Enter) against closing punctuation\n"
           "barges the cursor past it and closes the gap"));

    /// Editor panel

    TR_(editorPanelTitle, tr("Editor"));
    TR_(editorPanelCenterOnScroll, tr("Center on scroll"));
    TR_(editorPanelOverwrite, tr("Overwrite"));
    TR_(editorPanelTabStopDistance, tr("Tab stop distance:"));
    TR_(editorPanelWrapMode, tr("Word wrap mode:"));
    TR_(editorPanelWrapModeTooltip,
        tr("Smart: If possible, wrapping occurs at a word boundary; otherwise\n"
           "it will occur at the appropriate point on the line, even in the "
           "middle\nof a word."));
    TR_(editorPanelNoWrap, tr("None"));
    TR_(editorPanelWordWrap, tr("At word boundary"));
    TR_(editorPanelWrapAnywhere, tr("Anywhere"));
    TR_(editorPanelWrapAtWordBoundaryOrAnywhere, tr("Smart"));
    TR_(editorPanelDblClickWhitespace, tr("Double-click whitespace selection"));
    TR_(editorPanelLineNumbers, tr("Line numbers"));
    TR_(editorPanelLineHighlight, tr("Highlight current line"));
    TR_(editorPanelSelectionHandles, tr("Selection handles"));
    TR_(editorPanelLeftRightMargin, tr("Left/Right margin:"));

    /// Word counter panel

    TR_(wordCounterPanelTitle, tr("Word counter"));

    // TODO: Make the group box selectable and add an is active bool or hidden
    // guard check

    TR_(wordCounterPanelLineCount, tr("Line count"));
    TR_(wordCounterPanelWordCount, tr("Word count"));
    TR_(wordCounterPanelCharCount, tr("Character count"));
    TR_(wordCounterPanelSel, tr("Selection counts"));
    TR_(wordCounterPanelSelReplace, tr("Selection replacement"));
    TR_(wordCounterPanelLinePos, tr("Line position"));
    TR_(wordCounterPanelColPos, tr("Column position"));

    /// Color bar panel

    TR_(colorBarPanelTitle, tr("Color bar"));
    TR_(colorBarPanelPosition, tr("Position:"));
    TR_(colorBarPanelTop, tr("Top"));
    TR_(colorBarPanelBelowMenuBar, tr("Below menu bar"));
    TR_(colorBarPanelAboveStatusBar, tr("Above status bar"));
    TR_(colorBarPanelBottom, tr("Bottom"));

    /// Settings

    TR_(noTheme, tr("No theme"));

    /// Settings Dialog

    TR_(settingsTitle, tr("Settings"));
    TR_(settingsTitleFormat, tr("%0 Settings"));

    /// Markup Preview Switch

    TR_(previewEdit, tr("Edit"));
    TR_(previewSplit, tr("Split"));
    TR_(previewPreview, tr("Preview"));

    /// Notebook color chip

    TR_(chipColor, tr("Chip color..."));
    TR_(chipTextColor, tr("Text color..."));

} // namespace Tr

} // namespace Fernanda

#undef TR_
#undef TRN_
