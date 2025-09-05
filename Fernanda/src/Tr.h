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

// Note: Qt Linguist requires the tr method to be in place
#define TR_(Name, TrCall)                                                      \
    inline QString Name() { return TrCall; }

namespace Fernanda {

QString
tr(const char* sourceText, const char* disambiguation = nullptr, int n = -1);

namespace Tr {

    namespace Buttons {

        // Maybe split and/or associate with particular widgets or scenarios
        TR_(ok, tr("OK"));
        TR_(licenses, tr("Licenses"));
        TR_(aboutQt, tr("About Qt"));
        TR_(save, tr("Save"));
        TR_(discard, tr("Discard"));
        TR_(cancel, tr("Cancel"));

    } // namespace Buttons

    namespace Dialogs {

        TR_(openFileCaption, tr("Open existing file"));
        TR_(saveFileCaption, tr("Save file as"));

        TR_(aboutTitle, tr("About"));

        inline QString aboutBody()
        {
            auto arg_0 = [] {
                return tr("<b>Fernanda</b> is a plain text editor for drafting "
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
                .arg(arg_3()
                         .arg(VERSION_FULL_STRING)
                         .arg(VERSION_RELEASE_NAME_STRING));
        }

        TR_(savePromptTitle, tr("Unsaved Changes"));
        TR_(savePromptSingleFileBodyFormat,
            tr("Would you like to save your "
               "changes?<p><b>File:</b></p><p>%0</p>"));

    } // namespace Dialogs

    namespace Menus {

        TR_(file, tr("File"));
        TR_(fileNewTab, tr("New tab"));
        TR_(fileNewWindow, tr("New window"));
        TR_(fileOpen, tr("Open..."));
        TR_(fileSave, tr("Save"));
        TR_(fileSaveAs, tr("Save as..."));
        TR_(fileSaveWindow, tr("Save all in window"));
        TR_(fileSaveAll, tr("Save all"));
        TR_(fileClose, tr("Close"));
        TR_(fileCloseWindowFiles, tr("Close all in window"));
        TR_(fileCloseAllFiles, tr("Close all"));
        TR_(fileCloseWindow, tr("Close window"));
        TR_(fileCloseAllWindows, tr("Close all windows"));
        TR_(fileQuit, tr("Quit"));

        TR_(edit, tr("Edit"));
        TR_(editUndo, tr("Undo"));
        TR_(editRedo, tr("Redo"));
        TR_(editCut, tr("Cut"));
        TR_(editCopy, tr("Copy"));
        TR_(editPaste, tr("Paste"));
        TR_(editDelete, tr("Delete"));
        TR_(editSelectAll, tr("Select all"));

        TR_(view, tr("View"));
        TR_(viewPreviousTab, tr("Previous tab"));
        TR_(viewNextTab, tr("Next tab"));
        TR_(viewPreviousWindow, tr("Previous window"));
        TR_(viewNextWindow, tr("Next window"));

        TR_(settingsOpen, tr("Settings"));

        TR_(help, tr("Help"));
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
