/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QWidget>

#include "Coco/Concepts.h"

#include "Debug.h"
#include "MenuState.h"

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

    using Slot = std::function<void()>;
    using CheckedSlot = std::function<void(bool)>;

    explicit MenuBuilder(Mode mode, QWidget* parent)
        : mode_(mode)
        , parent_(parent)
    {
        if (mode == MenuBar && !qobject_cast<QMainWindow*>(parent)) {
            WARN(
                "MenuBar mode with non-QMainWindow parent; set() will be a "
                "no-op");
        }
    }

    ~MenuBuilder() { TRACER; }

    MenuBuilder& menu(const QString& title)
    {
        if (!parent_) return *this;
        if (mode_ != MenuBar) return *this;

        ensureMenuBar_();

        currentMenu_ = new QMenu(title, menuBar_);
        menuBar_->addMenu(currentMenu_);
        lastAction_ = nullptr;

        return *this;
    }

    MenuBuilder& action(const QString& text)
    {
        if (!parent_) return *this;
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
        if (!parent_) return *this;
        if (mode_ != MenuBar) return *this;

        ensureMenuBar_();
        lastAction_ = action_(text, menuBar_);
        menuBar_->addAction(lastAction_);

        return *this;
    }

    MenuBuilder& addAction(QAction* action)
    {
        if (!parent_) return *this;
        ensureCurrentMenu_();
        currentMenu_->addAction(action);
        lastAction_ = action;
        return *this;
    }

    MenuBuilder& addBarAction(QAction* action)
    {
        if (!parent_) return *this;
        if (mode_ != MenuBar) return *this;

        ensureMenuBar_();
        menuBar_->addAction(action);
        lastAction_ = action;

        return *this;
    }

    MenuBuilder& capture(QAction** actionOut)
    {
        if (actionOut) *actionOut = lastAction_;
        return *this;
    }

    // TODO: Just use a single template for these with type traits check?
    MenuBuilder& onUserTrigger(
        QObject* receiver,
        Slot slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        if (!parent_) return *this;

        if (lastAction_) {
            lastAction_->connect(
                lastAction_,
                &QAction::triggered,
                receiver,
                std::move(slot),
                type);
        }

        return *this;
    }

    // TODO: Just use a single template for these with type traits check?
    MenuBuilder& onUserTrigger(
        QObject* receiver,
        CheckedSlot slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        if (!parent_) return *this;

        if (lastAction_) {
            lastAction_->connect(
                lastAction_,
                &QAction::triggered,
                receiver,
                std::move(slot),
                type);
        }

        return *this;
    }

    template <typename ReceiverT, typename MethodClassT>
        requires Coco::Concepts::QObjectDerived<ReceiverT>
                 && std::is_base_of_v<MethodClassT, ReceiverT>
    MenuBuilder& onUserTrigger(
        ReceiverT* receiver,
        void (MethodClassT::*memberSlot)(),
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return onUserTrigger(
            static_cast<QObject*>(receiver),
            [receiver, memberSlot] { (receiver->*memberSlot)(); },
            type);
    }

    MenuBuilder& onToggle(
        QObject* receiver,
        CheckedSlot slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        if (!parent_) return *this;

        if (lastAction_) {
            lastAction_->connect(
                lastAction_,
                &QAction::toggled,
                receiver,
                std::move(slot),
                type);
        }

        return *this;
    }

    template <typename ReceiverT, typename MethodClassT>
        requires Coco::Concepts::QObjectDerived<ReceiverT>
                 && std::is_base_of_v<MethodClassT, ReceiverT>
    MenuBuilder& onToggle(
        ReceiverT* receiver,
        void (MethodClassT::*memberSlot)(),
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return onToggle(
            static_cast<QObject*>(receiver),
            [receiver, memberSlot] { (receiver->*memberSlot)(); },
            type);
    }

    MenuBuilder&
    enabledToggle(MenuState* state, int key, MenuState::Predicate predicate)
    {
        if (state && lastAction_ && predicate) {
            state->bind(lastAction_, key, std::move(predicate));
        }

        return *this;
    }

    template <typename KeyT>
        requires std::is_enum_v<KeyT>
                 && std::is_same_v<std::underlying_type_t<KeyT>, int>
    MenuBuilder&
    enabledToggle(MenuState* state, KeyT key, MenuState::Predicate predicate)
    {
        return enabledToggle(
            state,
            static_cast<int>(key),
            std::move(predicate));
    }

    template <typename CallableT>
        requires std::invocable<CallableT, MenuBuilder&>
    MenuBuilder& apply(CallableT&& callable)
    {
        callable(*this);
        return *this;
    }

    template <typename CallableT>
        requires std::invocable<CallableT, MenuBuilder&>
    MenuBuilder& applyIf(bool condition, CallableT&& callable)
    {
        if (condition) callable(*this);
        return *this;
    }

    MenuBuilder& autoRepeat(bool enabled)
    {
        if (!parent_) return *this;

        if (lastAction_) lastAction_->setAutoRepeat(enabled);
        return *this;
    }

    MenuBuilder& shortcut(const QKeySequence& keySequence)
    {
        if (!parent_) return *this;

        if (lastAction_) lastAction_->setShortcut(keySequence);
        return *this;
    }

    MenuBuilder& checkable(bool initial = false)
    {
        if (!parent_) return *this;

        if (lastAction_) {
            lastAction_->setCheckable(true);
            lastAction_->setChecked(initial);
        }

        return *this;
    }

    MenuBuilder& separator()
    {
        if (!parent_) return *this;

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
        if (!parent_) return;
        if (!currentMenu_ || mode_ != ContextMenu) return;

        currentMenu_->setAttribute(Qt::WA_DeleteOnClose);
        currentMenu_->popup(globalPos);
    }

    void set()
    {
        if (!parent_) return;
        if (!menuBar_ || mode_ != MenuBar) return;

        if (auto window = qobject_cast<QMainWindow*>(parent_))
            window->setMenuBar(menuBar_);
    }

private:
    Mode mode_;
    QWidget* parent_;

    QMenuBar* menuBar_ = nullptr;
    QMenu* currentMenu_ = nullptr;
    QAction* lastAction_ = nullptr;

    void ensureMenuBar_()
    {
        if (!menuBar_) menuBar_ = new QMenuBar(parent_);
    }

    void ensureCurrentMenu_()
    {
        if (!currentMenu_) currentMenu_ = new QMenu(parent_);
    }

    QAction* action_(const QString& text, QWidget* parent)
    {
        auto action = new QAction(text, parent);
        action->setAutoRepeat(false);
        return action;
    }
};

} // namespace Fernanda
