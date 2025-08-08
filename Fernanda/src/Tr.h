#pragma once

#include <QCoreApplication>
#include <QString>

#include "Version.h"

// Note: Qt Linguist requires the tr function at the call-site
#define TR_(Name, TrCall) inline QString Name() { return TrCall; }

namespace Tr
{
    inline QString tr
    (
        const char* sourceText,
        const char* disambiguation = nullptr,
        int n = -1
    )
    {
        return QCoreApplication::translate
        (
            "Tr", sourceText,
            disambiguation, n
        );
    }

    namespace Buttons
    {
        TR_(ok, tr("OK"));
        TR_(cancel, tr("Cancel"));
        TR_(save, tr("Save"));
        TR_(discard, tr("Discard"));
        TR_(licenses, tr("Licenses"));
        TR_(aboutQt, tr("About Qt"));

    } // namespace Tr::Buttons

    namespace Env
    {
        // Help menu
        TR_(help, tr("Help"));
        TR_(about, tr("About"));

    } // namespace Tr::Env

    namespace Eco
    {
        // Shared menu items
        TR_(saveAll, tr("Save all"));
        TR_(closeAll, tr("Close all"));

        // File menu
        TR_(file, tr("File"));
        TR_(newTab, tr("New tab"));
        TR_(newWindow, tr("New window"));
        TR_(open, tr("Open..."));
        TR_(save, tr("Save"));
        TR_(saveAs, tr("Save as..."));
        TR_(saveAllInWindow, tr("Save all in window"));
        TR_(close, tr("Close"));
        //TR_(closeEverywhere, tr("Close everywhere"));
        TR_(closeAllInWindow, tr("Close all in window"));
        TR_(closeWindow, tr("Close window"));
        TR_(closeAllWindows, tr("Close all windows"));
        TR_(quit, tr("Quit"));

        // Edit menu
        TR_(edit, tr("Edit"));
        TR_(undo, tr("Undo"));
        TR_(redo, tr("Redo"));
        TR_(cut, tr("Cut"));
        TR_(copy, tr("Copy"));
        TR_(paste, tr("Paste"));
        TR_(del, tr("Delete"));
        TR_(selectAll, tr("Select all"));

        // View menu
        TR_(view, tr("View"));
        TR_(prevTab, tr("Previous tab"));
        TR_(nextTab, tr("Next tab"));
        TR_(prevWindow, tr("Previous window"));
        TR_(nextWindow, tr("Next window"));

        // Settings menu
        TR_(settings, tr("Settings"));

        // File dialogs
        TR_(openFile, tr("Open file"));
        TR_(saveFileAs, tr("Save file as"));

    } // namespace Tr::Eco

    namespace Dialogs
    {
        namespace About
        {
            namespace detail
            {
                TR_(bodyArg0_, tr("<b>Fernanda</b> is a plain text editor for drafting long-form fiction. (At least, that's the plan.)"));
                TR_(bodyArg1_, tr("It's a personal project and a work-in-progress."));
                TR_(bodyArg2_, tr("See <a href=\"%0\">%0</a> for more information."));
                TR_(bodyArg3_, tr("<b>Version</b> %0"));

                inline QString bodyHtmlTemplate_()
                {
                    return "<p>%0</p><p>%1</p><p>%2</p><p>%3</p>";
                }

            } // namespace Tr::Dialogs::About::detail

            TR_(title, tr("About"));

            inline QString body()
            {
                return detail::bodyHtmlTemplate_()
                    .arg(detail::bodyArg0_())
                    .arg(detail::bodyArg1_())
                    .arg(detail::bodyArg2_().arg(VERSION_DOMAIN))
                    .arg(detail::bodyArg3_().arg(VERSION_FULL_STRING));
            }

        } // namespace Tr::Dialogs::About

        namespace Unsaved
        {
            TR_(title, tr("Unsaved Changes"));
            TR_(body, tr("Would you like to save your changes?"));

        } // namespace Tr::Dialogs::Unsaved

    } // namespace Tr::Dialogs

    namespace FontSelector
    {
        TR_(bold, tr("Bold"));
        TR_(italic, tr("Italic"));
    }

    namespace EditorSb
    {
        TR_(centerOnScroll, tr("Center on scroll"));
        TR_(overwrite, tr("Overwrite"));
        TR_(tabStopDistance, tr("Tab stop distance: "));
        TR_(wordWrapMode, tr("Word wrap mode: "));
        TR_(noWrap, tr("None"));
        TR_(wordWrap, tr("At word boundary"));
        TR_(wrapAnywhere, tr("Anywhere"));
        TR_(wrapAtWordBoundaryOrAnywhere, tr("Smart"));
    }

} // namespace Tr

#undef TR_
