/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QString>

#include "Version.h"

// TODO: Go through and Find All References and delete unused
// TODO: Find and remove "vanished" type entries in TS XML (old or moved
// translations) (make a bat file probably)

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
namespace Tr {

    /// General

    TR_(notepad, tr("Notepad"));
    TR_(nbTrash, tr("Trash"));

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
    TR_(nxOpenNotebookFilter, tr("Fernanda Notebook files (*.fnx)"));
    TR_(nxBetaAlert,
        tr("<p><b>This is a prerelease!</b></p><p>You should not trust your "
           "writing with prerelease versions of this software. Regardless, "
           "always make regular back-ups of your work.</p>"));

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

    // NP dialogs:

    TR_(npOpenFileCaption, tr("Open file"));
    TR_(npOpenFileFilter,
        tr("Plain text files (*.txt);;Fernanda Notebook files (*.fnx);;All "
           "files (*)"));
    TR_(npSaveAsCaption, tr("Save as"));
    TR_(npSaveAsFilter, tr("Plain text files (*.txt);;All files (*)"));

    // NB dialogs:

    TR_(nbImportFileCaption, tr("Import file"));
    TR_(nbImportFileFilter, tr("Plain text files (*.txt)"));
    TR_(nbSaveAsCaption, tr("Save as"));
    TR_(nbSaveAsFilter, tr("Fernanda Notebook files (*.fnx)"));

    // TODO: I think a multi version might be better - then, in the singular, we
    // can name the file (instead of weirdly saying "would you like to delete 1
    // file?")
    TRN_(
        nbTrashPromptBody,
        tr("Are you sure you want to delete %n file(s)? <b>This cannot be "
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

    TR_(nxNewWindow, tr("New window"));
    TR_(nxNewNotebook, tr("New notebook"));
    TR_(nxOpenNotebook, tr("Open notebook"));

    TR_(nxSave, tr("Save"));
    TR_(nxSaveAs, tr("Save as..."));

    TR_(nxDuplicateTab, tr("Duplicate tab"));

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

    TR_(npNewTab, tr("New tab"));
    TR_(npOpenFile, tr("Open..."));
    TR_(npSaveAllInWindow, tr("Save all in window"));
    TR_(npSaveAll, tr("Save all"));

    // NB menus and context menu:

    TR_(nbNewFile, tr("New file"));
    TR_(nbNewFolder, tr("New folder"));

    // NB menu:

    TR_(notebookMenu, tr("Notebook"));
    TR_(nbOpenNotepad, tr("Open notepad"));
    TR_(nbImportFiles, tr("Import files..."));

    // NB context menu:

    TR_(nbCollapse, tr("Collapse"));
    TR_(nbExpand, tr("Expand"));
    TR_(nbRename, tr("Rename"));
    TR_(nbRemove, tr("Remove"));
    TR_(nbRestore, tr("Restore"));
    TR_(nbDeletePermanently, tr("Delete permanently"));
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
        tr("Double-spacing (or pressing Enter) against closing punctuation "
           "barges the cursor past it and closes the gap"));

    /// Editor panel

    TR_(editorPanelTitle, tr("Editor"));
    TR_(editorPanelCenterOnScroll, tr("Center on scroll"));
    TR_(editorPanelOverwrite, tr("Overwrite"));
    TR_(editorPanelTabStopDistance, tr("Tab stop distance: "));
    TR_(editorPanelWrapMode, tr("Word wrap mode: "));
    TR_(editorPanelWrapModeTooltip,
        tr("Smart: If possible, wrapping occurs at a word boundary; otherwise "
           "it will occur at the appropriate point on the line, even in the "
           "middle of a word."));
    TR_(editorPanelNoWrap, tr("None"));
    TR_(editorPanelWordWrap, tr("At word boundary"));
    TR_(editorPanelWrapAnywhere, tr("Anywhere"));
    TR_(editorPanelWrapAtWordBoundaryOrAnywhere, tr("Smart"));
    TR_(editorPanelDblClickWhitespace, tr("Double-click whitespace selection"));
    TR_(editorPanelLineNumbers, tr("Line numbers"));
    TR_(editorPanelLineHighlight, tr("Highlight current line"));
    TR_(editorPanelSelectionHandles, tr("Selection handles"));

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
    TR_(colorBarPanelPosition, tr("Position: "));
    TR_(colorBarPanelTop, tr("Top"));
    TR_(colorBarPanelBelowMenuBar, tr("Below menu bar"));
    TR_(colorBarPanelAboveStatusBar, tr("Above status bar"));
    TR_(colorBarPanelBottom, tr("Bottom"));

    /// Settings

    TR_(noTheme, tr("No theme"));

    /// Settings Dialog

    TR_(settingsTitle, tr("Settings"));
    TR_(settingsTitleFormat, tr("%0 Settings"));

} // namespace Tr

} // namespace Fernanda

#undef TR_
#undef TRN_
