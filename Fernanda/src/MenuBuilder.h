/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <concepts>
#include <functional>
#include <utility>

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QWidget>

#include "Coco/Concepts.h"

#include "Debug.h"
#include "MenuState.h"
#include "Window.h"

namespace Fernanda {

// Declarative builder for menu bars and context menus. For menu bars, begin
// with .menu(), chain actions and modifiers, and finalize with .set(). For
// context menus, begin with .action() directly and finalize with .popup().
class MenuBuilder
{
public:
    enum Mode
    {
        MenuBar,
        ContextMenu
    };

    // Triggered slot only right now
    using Slot = std::function<void()>;

    explicit MenuBuilder(Mode mode, Window* window)
        : mode_(mode)
        , window_(window)
    {
    }

    virtual ~MenuBuilder() { TRACER; }

    MenuBuilder& menu(const QString& title)
    {
        if (!window_) return *this;
        if (mode_ != MenuBar) return *this;

        ensureMenuBar_();

        currentMenu_ = new QMenu(title, menuBar_);
        menuBar_->addMenu(currentMenu_);
        lastAction_ = nullptr;

        return *this;
    }

    MenuBuilder& action(const QString& text)
    {
        if (!window_) return *this;
        if (mode_ == MenuBar) ensureMenuBar_();

        ensureCurrentMenu_();
        lastAction_ = action_(text, currentMenu_);
        currentMenu_->addAction(lastAction_);

        return *this;
    }

    MenuBuilder& actionIf(bool condition, const QString& text)
    {
        if (!condition) {
            lastAction_ = nullptr;
            return *this;
        }

        return action(text);
    }

    // Add action directly to menu bar (e.g., standalone "Settings" item)
    MenuBuilder& barAction(const QString& text)
    {
        if (!window_) return *this;
        if (mode_ != MenuBar) return *this;

        ensureMenuBar_();
        lastAction_ = action_(text, menuBar_);
        menuBar_->addAction(lastAction_);

        return *this;
    }

    // TODO: Optional bool slot parameter, since we're ignoring
    // QAction::triggered checked parameter right now
    MenuBuilder& slot(
        QObject* receiver,
        Slot slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        if (!window_) return *this;

        if (lastAction_) {
            lastAction_->connect(
                lastAction_,
                &QAction::triggered,
                receiver,
                std::move(slot));
        }

        return *this;
    }

    template <typename ReceiverT, typename MethodClassT>
        requires Coco::Concepts::QObjectDerived<ReceiverT>
                 && std::is_base_of_v<MethodClassT, ReceiverT>
    MenuBuilder& slot(
        ReceiverT* receiver,
        void (MethodClassT::*memberSlot)(),
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return slot(
            static_cast<QObject*>(receiver),
            [receiver, memberSlot] { (receiver->*memberSlot)(); },
            type);
    }

    MenuBuilder&
    toggle(MenuState* state, const QString& key, MenuState::Predicate predicate)
    {
        if (state && lastAction_ && predicate) {
            state->bind(lastAction_, key, std::move(predicate));
        }

        return *this;
    }

    template <typename CallableT>
        requires std::invocable<CallableT, MenuBuilder&>
    MenuBuilder& apply(CallableT&& callable)
    {
        std::forward<CallableT>(callable)(*this);
        return *this;
    }

    MenuBuilder& autoRepeat(bool enabled)
    {
        if (!window_) return *this;

        if (lastAction_) lastAction_->setAutoRepeat(enabled);
        return *this;
    }

    MenuBuilder& shortcut(const QKeySequence& keySequence)
    {
        if (!window_) return *this;

        if (lastAction_) lastAction_->setShortcut(keySequence);
        return *this;
    }

    MenuBuilder& separator()
    {
        if (!window_) return *this;

        if (currentMenu_) currentMenu_->addSeparator();
        lastAction_ = nullptr;

        return *this;
    }

    MenuBuilder& separatorIf(bool condition)
    {
        if (!condition) return *this;
        return separator();
    }

    void popup(const QPoint& globalPos)
    {
        if (!window_) return;
        if (!currentMenu_ || mode_ != ContextMenu) return;

        currentMenu_->setAttribute(Qt::WA_DeleteOnClose);
        currentMenu_->popup(globalPos);
    }

    void set()
    {
        if (!window_) return;
        if (!menuBar_ || mode_ != MenuBar) return;

        window_->setMenuBar(menuBar_);
    }

private:
    Mode mode_;
    Window* window_;

    QMenuBar* menuBar_ = nullptr;
    QMenu* currentMenu_ = nullptr;
    QAction* lastAction_ = nullptr;

    void ensureMenuBar_()
    {
        if (!menuBar_) menuBar_ = new QMenuBar(window_);
    }

    void ensureCurrentMenu_()
    {
        if (!currentMenu_) currentMenu_ = new QMenu(window_);
    }

    QAction* action_(const QString& text, QWidget* parent)
    {
        auto action = new QAction(text, parent);
        action->setAutoRepeat(false);
        return action;
    }
};

} // namespace Fernanda
