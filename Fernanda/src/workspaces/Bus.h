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

using namespace Qt::StringLiterals;

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

    inline static auto WINDOWS_SET = u"windows:set"_s;
    inline static auto WINDOWS = u"windows:list"_s;
    inline static auto GET_SETTING = u"settings:get"_s;
    inline static auto SET_SETTING = u"settings:set"_s;
    inline static auto EDITOR_THEMES = u"style:editor_themes"_s;
    inline static auto WINDOW_THEMES = u"style:window_themes"_s;

    /// TODO STYLE: Define struct here, ThemeData? (Centralized place - keep Bus
    /// from including a style module file, style module can pass themes to
    /// whatever and we know they'll be able to read it)

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
