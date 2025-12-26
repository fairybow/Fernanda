/*
 * Fernanda  Copyright (C) 2025  fairybow
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

// Qt Linguist requires the tr method to be in place
#define TR_(Name, TrCall)                                                      \
    inline QString Name() { return TrCall; }

namespace Fernanda {

QString
tr(const char* sourceText, const char* disambiguation = nullptr, int n = -1);

// NP = Notepad; NB = Notebook; NX = Either
// TODO: Use a coherent naming scheme...
namespace Tr {

    /// General

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
    // See: doc.qt.io/qt-6/i18n-source-translation.html#handle-plural-forms -
    // but also, it's annoying - must use EN and strip plurals using lupdate
    // -pluralonly - just do it later
    inline QString nbTrashPromptBody(int count)
    {
        return tr(
            "Are you sure you want to delete %n file(s)? <b>This cannot be "
            "undone.</b>",
            nullptr,
            count);
    };

    /// Menus

    // TODO: Mnemonics

    // NX menus:

    TR_(nxFileMenu, tr("&File"));
    TR_(nxEditMenu, tr("&Edit"));
    TR_(nxViewMenu, tr("&View"));
    TR_(nxSettingsMenu, tr("&Settings"));
    TR_(nxHelpMenu, tr("&Help"));

    TR_(nxSave, tr("Save"));
    TR_(nxSaveAs, tr("Save as..."));

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

    /// Not updated:

    namespace Menus {

        // File

        TR_(fileNewWindow, tr("New &window"));
        TR_(fileNewNotebook, tr("New notebook"));
        TR_(fileOpenNotebook, tr("Open notebook"));

        TR_(fileCloseTab, tr("Close tab"));
        TR_(fileCloseTabEverywhere, tr("Close tab everywhere"));
        TR_(fileCloseWindowTabs, tr("Close window tabs"));
        TR_(fileCloseAllTabs, tr("Close all tabs"));
        TR_(fileCloseWindow, tr("Close window"));
        TR_(fileCloseAllWindows, tr("Close all windows"));

        TR_(fileQuit, tr("&Quit"));

        // Edit

        TR_(editUndo, tr("&Undo"));
        TR_(editRedo, tr("&Redo"));
        TR_(editCut, tr("Cu&t"));
        TR_(editCopy, tr("&Copy"));
        TR_(editPaste, tr("&Paste"));
        TR_(editDelete, tr("&Delete"));
        TR_(editSelectAll, tr("&Select all"));

        // Help

        TR_(helpAbout, tr("About"));

    } // namespace Menus

    namespace FontSelector {

        TR_(bold, tr("Bold"));
        TR_(italic, tr("Italic"));

    } // namespace FontSelector

} // namespace Tr

} // namespace Fernanda

#undef TR_

/// OLD (REMOVE AS IMPLEMENTED):

/*
namespace EditorsSb {

    TR_(centerOnScroll, tr("Center on scroll"));
    TR_(overwrite, tr("Overwrite"));
    TR_(tabStopDistance, tr("Tab stop distance: "));
    TR_(wordWrapMode, tr("Word wrap mode: "));
    TR_(noWrap, tr("None"));
    TR_(wordWrap, tr("At word boundary"));
    TR_(wrapAnywhere, tr("Anywhere"));
    TR_(wrapAtWordBoundaryOrAnywhere, tr("Smart"));

} // namespace EditorsSb

namespace KeyFiltersSb {

    TR_(active, tr("Active"));
    TR_(autoClose, tr("Auto-close"));
    TR_(barging, tr("Barging"));

} // namespace KeyFiltersSb
*/
