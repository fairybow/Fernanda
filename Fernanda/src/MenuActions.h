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

struct MenuActions
{
    struct File
    {
        QAction* newTab = nullptr;
        QAction* newWindow = nullptr;
        QAction* newNotebook = nullptr;
        QAction* openNotebook = nullptr;
        QAction* notepadOpenFile = nullptr;
        QAction* notebookImportFile = nullptr;
        QAction* notebookOpenNotepad = nullptr;

        QAction* notepadSave = nullptr;
        QAction* notepadSaveAs = nullptr;
        QAction* notepadSaveAll = nullptr;
        QAction* notepadSaveAllInWindow = nullptr;

        QAction* notebookSave = nullptr;
        QAction* notebookSaveAs = nullptr;
        QAction* notebookExportFile = nullptr;

        QAction* closeTab = nullptr;
        QAction* closeAllTabsInWindow = nullptr;
        QAction* closeWindow = nullptr;

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

} // namespace Fernanda
