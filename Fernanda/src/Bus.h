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
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Path.h"

#include "Debug.h"
#include "Window.h"

namespace Fernanda {

class IFileModel;
class IFileView;
enum class SaveResult;

// We don't ever need to use this manually in command handler returns. Bus
// handles this itself. Outside usage is more for converting parameters when
// executing commands
template <typename T> inline [[nodiscard]] QVariant qVar(const T& value)
{
    return QVariant::fromValue<T>(value);
}

struct Command
{
    // Embarrassing note, but:
    // Can't have a type/kind enum for registration type (has return, has
    // command, etc) because there's nothing to store it in! We aren't
    // registering commands but handlers, obviously (plus, how would "has no
    // command" make any sense even if we were)?

    QVariantMap params{};
    Window* context = nullptr;

    Command(const QVariantMap& params = {}, Window* context = nullptr)
        : params(params)
        , context(context)
    {
    }

    // Without default value, will log unfound parameter but still return
    // invalid QVariant
    [[nodiscard]] QVariant param(const QString& key) const
    {
        if (!params.contains(key)) {
            INFO("Parameter {} not found in command params!", key);
        }

        return params.value(key);
    }

    [[nodiscard]] QVariant
    param(const QString& key, const QVariant& defaultValue) const
    {
        return params.value(key, defaultValue);
    }

    template <typename T> [[nodiscard]] T param(const QString& key) const
    {
        return param(key).value<T>();
    }

    template <typename T>
    [[nodiscard]] T param(const QString& key, const T& defaultValue) const
    {
        auto variant = params.value(key);
        if (!variant.isValid() || !variant.canConvert<T>()) return defaultValue;
        return variant.value<T>();
    }
};

template <typename T>
concept InterceptorWithCommand = std::is_invocable_r_v<bool, T, const Command&>;

template <typename T>
concept InterceptorWithoutCommand =
    std::is_invocable_r_v<bool, T>
    && !std::is_invocable_r_v<bool, T, const Command&>;

template <typename T>
concept HandlerWithCommandReturnsVoid =
    std::same_as<void, std::invoke_result_t<T, const Command&>>;

template <typename T>
concept HandlerWithCommandReturnsValue =
    std::is_invocable_v<T, const Command&>
    && !std::same_as<void, std::invoke_result_t<T, const Command&>>;

template <typename T>
concept HandlerWithoutCommandReturnsVoid =
    std::is_invocable_v<T> && std::same_as<void, std::invoke_result_t<T>>
    && !std::is_invocable_v<T, const Command&>;

template <typename T>
concept HandlerWithoutCommandReturnsValue =
    std::is_invocable_v<T> && !std::same_as<void, std::invoke_result_t<T>>
    && !std::is_invocable_v<T, const Command&>;

template <typename T, typename... Args>
concept ReturnsQVariant = std::is_invocable_r_v<QVariant, T, Args...>;

class Bus : public QObject
{
    Q_OBJECT

public:
    explicit Bus(QObject* parent = nullptr)
        : QObject(parent)
    {
        setup_();
    }

    virtual ~Bus() override { TRACER; }

    template <typename InterceptorT>
    void addInterceptor(const QString& id, InterceptorT&& interceptor)
    {
        // Handles:
        // (const Command&)->bool and
        // ()->bool

        INFO("Registering interceptor for: {}", id);

        if constexpr (InterceptorWithCommand<InterceptorT>) {
            INFO("-> InterceptorWithCommand branch");

            // (const Command&)->bool
            interceptors_[id] << interceptor;

        } else if constexpr (InterceptorWithoutCommand<InterceptorT>) {
            INFO("-> InterceptorWithoutCommand branch");

            // ()->bool
            interceptors_[id] << [interceptor = std::forward<InterceptorT>(
                                      interceptor)](const Command& cmd) {
                (void)cmd;
                return interceptor();
            };

        } else {

            static_assert(
                InterceptorWithCommand<InterceptorT>
                    || InterceptorWithoutCommand<InterceptorT>,
                "Interceptor must be callable as (const Command&)->bool or "
                "()->bool");
        }
    }

    template <typename HandlerT>
    void addCommandHandler(const QString& id, HandlerT&& handler)
    {
        // Handles:
        // (const Command&)->void,
        // (const Command&)->T,
        // ()->void, and
        // ()->T

        INFO("Registering handler for: {}", id);

        if constexpr (HandlerWithCommandReturnsVoid<HandlerT>) {
            INFO("-> HandlerWithCommandReturnsVoid branch");

            // (const Command&)->void
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                handler(cmd);
                return QVariant{};
            };

        } else if constexpr (HandlerWithCommandReturnsValue<HandlerT>) {
            INFO("-> HandlerWithCommandReturnsValue branch");

            // (const Command&)->T
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                if constexpr (ReturnsQVariant<HandlerT, const Command&>) {
                    return handler(cmd);
                } else {
                    return qVar(handler(cmd));
                }
            };

        } else if constexpr (HandlerWithoutCommandReturnsVoid<HandlerT>) {
            INFO("-> HandlerWithoutCommandReturnsVoid branch");

            // ()->void
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                (void)cmd;
                handler();
                return QVariant{};
            };

        } else if constexpr (HandlerWithoutCommandReturnsValue<HandlerT>) {
            INFO("-> HandlerWithoutCommandReturnsValue branch");

            // ()->T
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                (void)cmd;
                if constexpr (ReturnsQVariant<HandlerT>) {
                    return handler();
                } else {
                    return qVar(handler());
                }
            };

        } else {

            static_assert(
                HandlerWithCommandReturnsVoid<HandlerT>
                    || HandlerWithCommandReturnsValue<HandlerT>
                    || HandlerWithoutCommandReturnsVoid<HandlerT>
                    || HandlerWithoutCommandReturnsValue<HandlerT>,
                "Command handler must be callable as (const Command&)->T, "
                "(const Command&)->void, ()->T or ()->void");
        }
    }

    void execute(const QString& id, const Command& cmd)
    {
        (void)runCommand_(id, cmd);
    }

    void execute(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
    {
        (void)runCommand_(id, { params, context });
    }

    void execute(const QString& id, Window* context)
    {
        (void)runCommand_(id, { {}, context });
    }

    [[nodiscard]] QVariant call(const QString& id, const Command& cmd)
    {
        return runCommand_(id, cmd);
    }

    [[nodiscard]] QVariant call(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
    {
        return runCommand_(id, { params, context });
    }

    [[nodiscard]] QVariant call(const QString& id, Window* context)
    {
        return runCommand_(id, { {}, context });
    }

    template <typename T>
    [[nodiscard]] T call(const QString& id, const Command& cmd)
    {
        return runCommand_(id, cmd).value<T>();
    }

    template <typename T>
    [[nodiscard]] T call(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
    {
        return runCommand_(id, { params, context }).value<T>();
    }

    template <typename T>
    [[nodiscard]] T call(const QString& id, Window* context)
    {
        return runCommand_(id, { {}, context }).value<T>();
    }

    // NO queries. Calls can be used as queries. It isn't a big deal!

signals:
    /// Re-verified:
    void lastWindowClosed();
    void windowCreated(Window* context);
    void windowDestroyed(Window* context);
    // View may be nullptr!
    void activeFileViewChanged(Window* context, IFileView* view);
    void treeViewDoubleClicked(Window* context, const QModelIndex& index);
    void fileModelReadied(Window* context, IFileModel* model);
    void fileModelModificationChanged(IFileModel* model, bool modified);
    void fileModelMetaChanged(IFileModel* model);
    void treeViewContextMenuRequested(
        Window* context,
        const QPoint& globalPos,
        const QModelIndex& index);
    void viewDestroyed(IFileModel* model);

    /// Old:

    // void workspaceOpened();

    // WindowService

    //void visibleWindowCountChanged(int count);

    // Window may be nullptr!
    // void activeWindowChanged(Window* window);

    // FileService

    //void fileSaved(SaveResult result, const Coco::Path& path);
    //void fileSavedAs(
        //SaveResult result,
        //const Coco::Path& path,
        //const Coco::Path& oldPath = {});
    // void windowSaveExecuted(Window* window, SaveResult result);
    // void workspaceSaveExecuted(SaveResult result);

    // ViewService

    //void windowTabCountChanged(Window* window, int count);

    //void viewClosed(IFileView* view);

    // SettingsModule

    void settingChanged(const QString& key, const QVariant& value);

    // Maybe:
    // void workspaceShuttingDown(Workspace* workspace);
    // void windowShown(Window* window);
    // void windowClosed(Window* window);

private:
    QHash<QString, std::function<QVariant(const Command&)>> commandHandlers_{};
    QHash<QString, QList<std::function<bool(const Command&)>>> interceptors_{};

    void setup_();

    [[nodiscard]] QVariant runCommand_(const QString& id, const Command& cmd)
    {
        for (auto& interceptor : interceptors_[id]) {
            if (interceptor(cmd)) {
                logCmdIntercepted_(id, cmd);
                return {};
            }
        }

        if (auto handler = commandHandlers_.value(id)) {
            auto result = handler(cmd);
            logCmdRan_(id, cmd, result);
            return result;
        } else {
            logCmdNoHandler_(id, cmd);
            return {};
        }
    }

    void logCmdIntercepted_(const QString& id, const Command& cmd) const
    {
        constexpr auto log_format =
            "Intercepted: {}\n\tParams: {}\n\tContext: {}";
        INFO(log_format, id, cmd.params, cmd.context);
    }

    void logCmdRan_(
        const QString& id,
        const Command& cmd,
        const QVariant& result) const
    {
        constexpr auto log_format =
            "Executed: {}\n\tParams: {}\n\tContext: {}\n\tResult: {}";
        INFO(log_format, id, cmd.params, cmd.context, result);
    }

    void logCmdNoHandler_(const QString& id, const Command& cmd) const
    {
        constexpr auto log_format =
            "No handler found!: {}\n\tParams: {}\n\tContext: {}";
        INFO(log_format, id, cmd.params, cmd.context);
    }
};

} // namespace Fernanda
