/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

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

} // namespace Fernanda

#define DECLARE_HOOK_ACCESSORS(Type, GetterName, SetterName, MemberName)       \
    Type GetterName() const noexcept { return MemberName; }                    \
    void SetterName(const Type& hook) { MemberName = hook; }                   \
    template <typename ClassT, typename ReturnT, typename... Args>             \
    void SetterName(ClassT* object, ReturnT (ClassT::*hook)(Args...))          \
    {                                                                          \
        MemberName = [object, hook](Args... args) -> ReturnT {                 \
            return (object->*hook)(std::forward<Args>(args)...);               \
        };                                                                     \
    }
