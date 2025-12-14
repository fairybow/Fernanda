/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAction>

namespace Fernanda {

struct CommonMenuActions
{
    struct File
    {
        QAction* newWindow = nullptr;
        QAction* newNotebook = nullptr;
        QAction* openNotebook = nullptr;

        QAction* closeTab = nullptr;
        QAction* closeTabEverywhere = nullptr;
        QAction* closeWindowTabs = nullptr;
        QAction* closeAllTabs = nullptr;
        QAction* closeWindow = nullptr;
        QAction* closeAllWindows = nullptr;

        QAction* quit = nullptr;
    } file;

    struct Edit
    {
        QAction* undo = nullptr;
        QAction* redo = nullptr;
        QAction* cut = nullptr;
        QAction* copy = nullptr;
        QAction* paste = nullptr;
        QAction* del = nullptr;
        QAction* selectAll = nullptr;
    } edit;

    QAction* settings = nullptr;

    struct Help
    {
        QAction* about = nullptr;
    } help;
};

struct NotepadMenuActions
{
    CommonMenuActions common{};

    struct File
    {
        QAction* newTab = nullptr;
        QAction* openFile = nullptr;

        QAction* save = nullptr;
        QAction* saveAs = nullptr;
        QAction* saveAll = nullptr;
        QAction* saveAllInWindow = nullptr;
    } file;
};

struct NotebookMenuActions
{
    CommonMenuActions common{};

    struct File
    {
        QAction* newFile = nullptr;
        QAction* newFolder = nullptr;
        QAction* renameItem = nullptr;
        QAction* removeItem = nullptr;
        QAction* importFile = nullptr;
        QAction* openNotepad = nullptr;

        QAction* save = nullptr;
        QAction* saveAs = nullptr;
        QAction* exportFile = nullptr;
    } file;
};

} // namespace Fernanda
