/*
 * Fernanda is a plain text editor for fiction writing
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

#include <functional>
#include <type_traits>
#include <utility>

#include <QAction>
#include <QHash>
#include <QList>
#include <QObject>

#include "core/Debug.h"
#include "ui/Window.h"

namespace Fernanda {

// Manages dynamic menu action states for a window. Actions are bound to
// predicate functions via MenuBuilder::toggle(), grouped by int keys. Calling
// refresh() with a key re-evaluates all predicates in that group and updates
// their associated actions' enabled state
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

    void bind(QAction* action, int key, Predicate predicate)
    {
        if (!action || !predicate) return;

        toggles_[key].append({ action, std::move(predicate) });
        action->setEnabled(toggles_[key].last().predicate());
    }

    template <typename KeyT>
        requires std::is_enum_v<KeyT>
                 && std::is_same_v<std::underlying_type_t<KeyT>, int>
    void bind(QAction* action, KeyT key, Predicate predicate)
    {
        bind(action, static_cast<int>(key), std::move(predicate));
    }

    void refresh(int key)
    {
        for (auto& toggle : toggles_.value(key))
            toggle.action->setEnabled(toggle.predicate());
    }

    template <typename KeyT>
        requires std::is_enum_v<KeyT>
                 && std::is_same_v<std::underlying_type_t<KeyT>, int>
    void refresh(KeyT key)
    {
        refresh(static_cast<int>(key));
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
    QHash<int, QList<Toggle_>> toggles_{};
};

} // namespace Fernanda
