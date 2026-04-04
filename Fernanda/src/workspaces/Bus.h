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

#include <QObject>
#include <QString>
#include <QVariant>

#include "core/Debug.h"
#include "models/AbstractFileModel.h"
#include "ui/Window.h"
#include "views/AbstractFileView.h"
#include "workspaces/Commander.h"

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

    inline static auto WINDOWS_SET = QStringLiteral("windows:set");
    inline static auto WINDOWS = QStringLiteral("windows:list");
    inline static auto GET_SETTING = QStringLiteral("settings:get");
    inline static auto SET_SETTING = QStringLiteral("settings:set");
    inline static auto EDITOR_THEMES = QStringLiteral("style:editor_themes");
    inline static auto WINDOW_THEMES = QStringLiteral("style:window_themes");

    // TODO: Define struct here, ThemeData?

signals:
    void windowCreated(Window* context);
    void windowDestroyed(Window* context);
    void activeFileViewChanged(
        Window* context,
        AbstractFileView* fileView); // Active view can be nullptr!
    void fileViewCreated(AbstractFileView* fileView);
    void fileModelReadied(Window* context, AbstractFileModel* fileModel);
    void
    fileModelModificationChanged(AbstractFileModel* fileModel, bool modified);
    void fileModelMetaChanged(AbstractFileModel* fileModel);
    void fileModelExternallyModified(AbstractFileModel* fileModel);
    void fileModelPathInvalidated(AbstractFileModel* fileModel);
    void fileModelReloadRequested(AbstractFileModel* fileModel);
    void settingChanged(const QString& key, const QVariant& value);
};

} // namespace Fernanda
