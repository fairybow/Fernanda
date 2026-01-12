/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVariant>

#include "AbstractFileModel.h"
#include "AbstractFileView.h"
#include "Commander.h"
#include "Debug.h"
#include "Window.h"

namespace Fernanda {

// Bus addresses cross-Service concerns. Fundamentally for Service-to-Service,
// lateral communication via commands and signals, but a Workspace could still
// connect to a signal if needed to avoid duplicating Bus signals in a given
// Service (e.g., In Service: `emit bus->thing(); emit thing();`).
class Bus : public Commander
{
    Q_OBJECT

public:
    explicit Bus(QObject* parent = nullptr)
        : Commander(parent)
    {
    }

    virtual ~Bus() override { TRACER; }

    constexpr static auto WINDOWS_SET = "windows:set";
    constexpr static auto WINDOWS = "windows:list";
    constexpr static auto GET_SETTING = "settings:get";
    constexpr static auto SET_SETTING = "settings:set";
    constexpr static auto EDITOR_THEMES = "style:editor_themes";
    constexpr static auto WINDOW_THEMES = "style:window_themes";

signals:
    void windowCreated(Window* context);
    void windowDestroyed(Window* context);
    void fileViewCreated(AbstractFileView* fileView);
    void fileModelReadied(Window* context, AbstractFileModel* fileModel);
    void
    fileModelModificationChanged(AbstractFileModel* fileModel, bool modified);
    void fileModelMetaChanged(AbstractFileModel* fileModel);
    void settingChanged(const QString& key, const QVariant& value);
};

} // namespace Fernanda
