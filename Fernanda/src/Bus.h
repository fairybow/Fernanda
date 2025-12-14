/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Commander.h"
#include "Debug.h"
#include "Window.h"

namespace Fernanda {

// Just include?
class AbstractFileModel;
class AbstractFileView;

class Bus : public Commander
{
    Q_OBJECT

public:
    explicit Bus(QObject* parent = nullptr)
        : Commander(parent)
    {
        setup_();
    }

    virtual ~Bus() override { TRACER; }

signals:
    // TODO: These should be used mostly to communicate from service to service?
    // I think if this is being used to talk from service to Workspace, then we
    // can just use a direct signal...
    // - Go through and check all (FInd All Ref) for this

    /// Re-verified:
    void lastWindowClosed();
    void windowCreated(Window* context);
    void windowDestroyed(Window* context);
    // File view may be nullptr!
    void activeFileViewChanged(Window* context, AbstractFileView* fileView);
    void treeViewDoubleClicked(Window* context, const QModelIndex& index);
    void fileModelReadied(Window* context, AbstractFileModel* fileModel);
    void
    fileModelModificationChanged(AbstractFileModel* fileModel, bool modified);
    void fileModelMetaChanged(AbstractFileModel* fileModel);
    void treeViewContextMenuRequested(
        Window* context,
        const QPoint& globalPos,
        const QModelIndex& index);
    void viewDestroyed(AbstractFileModel* fileModel);

    /// Old:

    // SettingsModule

    void settingChanged(const QString& key, const QVariant& value);

private:
    void setup_();
};

} // namespace Fernanda
