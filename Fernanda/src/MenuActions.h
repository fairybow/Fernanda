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

// TODO: Expand/collapse joint items for both workspaces
// TODO: Check which can go into View if any

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

        QAction* save = nullptr;
        QAction* saveAs = nullptr;
    } file;

    struct Notebook
    {
        QAction* openNotepad = nullptr;
        QAction* importFiles = nullptr;
    } notebook;
};

} // namespace Fernanda
