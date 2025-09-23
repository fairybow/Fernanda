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
#include <type_traits>
#include <utility>

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Debug.h"
#include "Coco/Log.h"

namespace Fernanda {

class Window;

struct Command
{
    QVariantMap params{};
    Window* context = nullptr;

    Command(const QVariantMap& params = {}, Window* context = nullptr)
        : params(params)
        , context(context)
    {
    }
};

// Make concepts internal later
template <typename T>
concept HasCommandParam = std::is_invocable_v<T, const Command&>;

template <typename T>
concept HasNoParams =
    std::is_invocable_v<T> && !std::is_invocable_v<T, const Command&>;

template <typename T, typename... Args, typename X>
concept ReturnsX = std::is_invocable_v<T, Args...>
                   && std::is_same_v<std::invoke_result_t<T, Args...>, X>;

template <typename T, typename... Args>
concept ReturnsVoid = ReturnsX<T, Args..., void>;

template <typename T, typename... Args>
concept ReturnsQVariant = ReturnsX<T, Args..., QVariant>;

template <typename T, typename... Args>
concept ReturnsBool = ReturnsX<T, Args..., bool>;

template <typename T>
concept ValidInterceptor =
    (HasCommandParam<T> && ReturnsBool<T, const Command&>)
    || (HasNoParams<T> && ReturnsBool<T>);

template <typename T>
concept ValidHandler = HasCommandParam<T> || HasNoParams<T>;

class Bus : public QObject
{
    Q_OBJECT

public:
    explicit Bus(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~Bus() override { COCO_TRACER; }

    template <typename InterceptorT>
    void addInterceptor(const QString& id, InterceptorT&& interceptor)
    {
        // We need to handle the following variants:
        //
        // interceptor(const Command&)->bool
        // interceptor()->bool
        //
        // All the following registrations need to work:
        //
        // bus->addInterceptor("id", [&](const Command&) { return
        // true; });
        //
        // bus->addInterceptor("id", [&] { return
        // true; });

        if constexpr (
            HasCommandParam<InterceptorT>
            && ReturnsBool<InterceptorT, const Command&>) {

            // Handle Command& -> bool
            interceptors_[id] << interceptor;

        } else if constexpr (
            HasNoParams<InterceptorT> && ReturnsBool<InterceptorT>) {

            // Handle () -> bool
            interceptors_[id] << [interceptor = std::forward<InterceptorT>(
                                      interceptor)](const Command& cmd) {
                (void)cmd;
                return interceptor();
            };

        } else {
            static_assert(
                ValidInterceptor<InterceptorT>,
                "Invalid interceptor signature");
        }
    }

    template <typename HandlerT>
    void addCommandHandler(const QString& id, HandlerT&& handler)
    {
        // We need to handle the following variants:
        //
        // handler(const Command&)->QVariant
        // handler(const Command&)->void
        // handler()->QVariant
        // handler()->void
        //
        // All the following registrations need to work:
        //
        // bus->addCommandHandler("id", [&](const Command&) { return
        // QVariant{}; });
        //
        // bus->addCommandHandler("id", [&] { return
        // QVariant{}; });
        //
        // bus->addCommandHandler("id", [&](const Command&)
        // {});
        //
        // bus->addCommandHandler("id", [&] {});

        if constexpr (HasCommandParam<HandlerT>) {

            if constexpr (ReturnsVoid<HandlerT, const Command&>) {

                // Handle Command& -> void
                commandHandlers_[id] = [handler = std::forward<HandlerT>(
                                            handler)](const Command& cmd) {
                    handler(cmd);
                    return {};
                };

            } else {

                // Handle Command& -> T
                commandHandlers_[id] = [handler = std::forward<HandlerT>(
                                            handler)](const Command& cmd) {
                    if constexpr (ReturnsQVariant<HandlerT, const Command&>) {
                        return handler(cmd);
                    } else {
                        return QVariant::fromValue(handler(cmd));
                    }
                };
            }

        } else if constexpr (HasNoParams<HandlerT>) {

            if constexpr (ReturnsVoid<HandlerT>) {

                // Handle () -> void
                commandHandlers_[id] = [handler = std::forward<HandlerT>(
                                            handler)](const Command& cmd) {
                    (void)cmd;
                    handler();
                    return {};
                };

            } else {

                // Handle () -> T
                commandHandlers_[id] = [handler = std::forward<HandlerT>(
                                            handler)](const Command& cmd) {
                    (void)cmd;
                    if constexpr (ReturnsQVariant<HandlerT>) {
                        return handler();
                    } else {
                        return QVariant::fromValue(handler());
                    }
                };
            }

        } else {
            static_assert(ValidHandler<HandlerT>, "Invalid handler signature");
        }
    }

    void execute(const QString& id, const Command& cmd)
    {
        (void)runCommand_(id, cmd);
    }

    // Add execute overloads

    [[nodiscard]] QVariant call(const QString& id, const Command& cmd)
    {
        return runCommand_(id, cmd);
    }

    // Add call overloads (don't forget template returns)

    // NO queries. Calls can be used as queries. It isn't a big deal!

private:
    QHash<QString, std::function<QVariant(const Command&)>> commandHandlers_{};
    QHash<QString, QList<std::function<bool(const Command&)>>> interceptors_{};

    [[nodiscard]] QVariant runCommand_(const QString& id, const Command& cmd)
    {
        for (auto& interceptor : interceptors_[id]) {
            if (interceptor(cmd)) {
                // log intercepted
                return {};
            }
        }

        if (auto handler = commandHandlers_.value(id)) {
            auto result = handler(cmd);
            // log executed with result
            return result;
        } else {
            // log no handler
            return {};
        }
    }
};

} // namespace Fernanda
