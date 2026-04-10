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

#include <functional>
#include <utility>

#include <QObject>

namespace Fernanda {

class Bus;

class AbstractService : public QObject
{
    Q_OBJECT

public:
    AbstractService(Bus* bus, QObject* parent = nullptr)
        : QObject(parent)
        , bus(bus)
    {
    }

    virtual ~AbstractService() = default;

    void initialize()
    {
        if (initialized_) return;
        registerBusCommands();
        connectBusEvents();
        postInit();
        initialized_ = true;
    }

protected:
    Bus* bus;

    virtual void registerBusCommands() = 0;
    virtual void connectBusEvents() = 0;

    // For setup code that requires access to other services via Bus (and thus
    // can't be done in the ctor), e.g., anything that doesn't make sense with
    // just Bus event or command connection/registration. However, should NOT be
    // used for getting settings values, as Notebooks will NOT have set their
    // override path yet!
    virtual void postInit() {};

private:
    bool initialized_ = false;
};

// Declares a hook member (private) and its getter and setters (public). The
// macro contains access specifiers, so code following it will be public!
#define DECLARE_HOOK(Type, GetterName, SetterName)                             \
private:                                                                       \
    Type GetterName##_{};                                                      \
                                                                               \
public:                                                                        \
    Type GetterName() const noexcept { return GetterName##_; }                 \
    void SetterName(const Type& hook) { GetterName##_ = hook; }                \
    template <typename ClassT, typename ReturnT, typename... Args>             \
    void SetterName(ClassT* object, ReturnT (ClassT::*hook)(Args...))          \
    {                                                                          \
        GetterName##_ = [object, hook](Args... args) -> ReturnT {              \
            return (object->*hook)(std::forward<Args>(args)...);               \
        };                                                                     \
    }

} // namespace Fernanda
