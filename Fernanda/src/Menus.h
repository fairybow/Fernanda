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
#include <QKeySequence>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Bool.h"

#include "Bus.h"
#include "Commands.h"
#include "MenuActions.h"
#include "Tr.h"
#include "Window.h"

// Utility functions for Notepad and Notebook menu modules
namespace Fernanda::Menus {

COCO_BOOL(AutoRepeat);

inline QAction* makeBusAction(
    Bus* bus,
    Window* window,
    const QString& commandId,
    const QVariantMap& commandParams,
    const QString& text,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    if (!window) return nullptr;

    auto action = new QAction(text, window);
    action->connect(action, &QAction::triggered, window, [=] {
        bus->execute(commandId, commandParams, window);
    });
    action->setShortcut(keySequence);
    action->setAutoRepeat(autoRepeat);

    return action;
}

inline QAction* makeBusAction(
    Bus* bus,
    Window* window,
    const QString& commandId,
    const QString& text,
    const QKeySequence& keySequence = {},
    AutoRepeat autoRepeat = AutoRepeat::No)
{
    return makeBusAction(
        bus,
        window,
        commandId,
        {},
        text,
        keySequence,
        autoRepeat);
}

// TODO: Ensure we pass -1 to certain commands as arg (for "current editor"
// ops)
// TODO: Before we continue, must document commands and summarize menu actions
inline void
initializeCommonActions(Bus* bus, Window* window, CommonMenuActions& common)
{
    if (!bus || !window) return;

    common.file.newWindow = makeBusAction(
        bus,
        window,
        Commands::NEW_WINDOW,
        {},
        Tr::Menus::fileNewWindow());

    //...
}

} // namespace Fernanda::Menus
