/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <functional>
#include <utility>

#include <QAction>
#include <QHash>
#include <QObject>
#include <QList>
#include <QString>

#include "Debug.h"
#include "Window.h"

namespace Fernanda {

class MenuState : public QObject
{
    Q_OBJECT

public:
    using Predicate = std::function<bool()>;

    explicit MenuState(Window* window, QObject* parent = nullptr)
        : QObject(parent)
        , window_(window)
    {
    }

    virtual ~MenuState() override { TRACER; }

    Window* window() const { return window_; }

    void bind(QAction* action, const QString& key, Predicate predicate)
    {
        if (!action || !predicate) return;

        toggles_[key].append({ action, std::move(predicate) });
        action->setEnabled(toggles_[key].last().predicate());
    }

    void refresh(const QString& key)
    {
        for (auto& toggle : toggles_.value(key))
            toggle.action->setEnabled(toggle.predicate());
    }

    void refreshAll()
    {
        for (auto it = toggles_.begin(); it != toggles_.end(); ++it)
            for (auto& toggle : *it)
                toggle.action->setEnabled(toggle.predicate());
    }

private:
    struct Toggle_
    {
        QAction* action;
        Predicate predicate;
    };

    Window* window_;
    QHash<QString, QList<Toggle_>> toggles_{};
};

} // namespace Fernanda
