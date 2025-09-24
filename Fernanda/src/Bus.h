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
#include "Coco/Path.h"

#include "Utility.h"

/// TODO (for registering handlers)
/// - Make sure we aren't casting return values to QVar when registering (it
/// isn't needed!)
/// - Can we just return Coco::Path without QString conversion?
/// - Check where we can remove Utility.h include (toQVariant unneeded)
/// - Also check lambda args

namespace Fernanda {

class IFileModel;
class IFileView;
class Window;
enum class SaveResult;

struct Command
{
    QVariantMap params{};
    Window* context = nullptr;

    Command(const QVariantMap& params = {}, Window* context = nullptr)
        : params(params)
        , context(context)
    {
    }

    template <typename T> inline T param(const QString& key) const
    {
        if (!params.contains(key)) {
            constexpr auto log_format =
                "\n\tParameter '%0' not found in command params";
            COCO_LOG_THIS(QString(log_format).arg(key));
        }

        return params.value(key).value<T>();
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
    std::is_invocable_r_v<void, T, const Command&>;

template <typename T>
concept HandlerWithCommandReturnsValue =
    std::is_invocable_v<T, const Command&>
    && !std::is_invocable_r_v<void, T, const Command&>;

template <typename T>
concept HandlerWithoutCommandReturnsVoid =
    std::is_invocable_r_v<void, T> && !std::is_invocable_v<T, const Command&>;

template <typename T>
concept HandlerWithoutCommandReturnsValue =
    std::is_invocable_v<T> && !std::is_invocable_r_v<void, T>
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
        initialize_();
    }

    virtual ~Bus() override { COCO_TRACER; }

    template <typename InterceptorT>
    void addInterceptor(const QString& id, InterceptorT&& interceptor)
    {
        // Handles:
        // (const Command&)->bool and
        // ()->bool

        if constexpr (InterceptorWithCommand<InterceptorT>) {

            // (const Command&)->bool
            interceptors_[id] << interceptor;

        } else if constexpr (InterceptorWithoutCommand<InterceptorT>) {

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

        if constexpr (HandlerWithCommandReturnsVoid<HandlerT>) {

            // (const Command&)->void
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                handler(cmd);
                return QVariant{};
            };

        } else if constexpr (HandlerWithCommandReturnsValue<HandlerT>) {

            // (const Command&)->T
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                if constexpr (ReturnsQVariant<HandlerT, const Command&>) {
                    return handler(cmd);
                } else {
                    return QVariant::fromValue(handler(cmd));
                }
            };

        } else if constexpr (HandlerWithoutCommandReturnsVoid<HandlerT>) {

            // ()->void
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                (void)cmd;
                handler();
                return QVariant{};
            };

        } else if constexpr (HandlerWithoutCommandReturnsValue<HandlerT>) {

            // ()->T
            commandHandlers_[id] = [handler = std::forward<HandlerT>(handler)](
                                       const Command& cmd) {
                (void)cmd;
                if constexpr (ReturnsQVariant<HandlerT>) {
                    return handler();
                } else {
                    return QVariant::fromValue(handler());
                }
            };

        } else {
            static_assert(
                HandlerWithCommandReturnsVoid<HandlerT>
                    || HandlerWithCommandReturnsValue<HandlerT>
                    || HandlerWithoutCommandReturnsVoid<HandlerT>
                    || HandlerWithoutCommandReturnsValue<HandlerT>,
                "Handler must be callable as (const Command&)->void, (const "
                "Command&)->T, ()->void or ()->T");
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
    // Workspace

    void workspaceInitialized();

    // WindowService

    void windowCreated(Window* window);
    void visibleWindowCountChanged(int count);
    void lastWindowClosed();

    // Window may be nullptr!
    void activeWindowChanged(Window* window);
    void windowDestroyed(Window* window);

    // FileService

    void fileReadied(IFileModel* model, Window* window);
    void fileModificationChanged(IFileModel* model, bool modified);
    void fileMetaChanged(IFileModel* model);
    void fileSaved(SaveResult result, const Coco::Path& path);
    void fileSavedAs(
        SaveResult result,
        const Coco::Path& path,
        const Coco::Path& oldPath = {});
    void windowSaveExecuted(Window* window, SaveResult result);
    void workspaceSaveExecuted(SaveResult result);

    // ViewService

    void windowTabCountChanged(Window* window, int count);

    // View may be nullptr!
    void activeFileViewChanged(IFileView* view, Window* window);
    void viewClosed(IFileView* view);

    // SettingsModule

    void settingChanged(const QString& key, const QVariant& value);

    // Maybe:

    // void workspaceShuttingDown(Workspace* workspace);
    // void windowShown(Window* window);
    // void windowClosed(Window* window);

private:
    QHash<QString, std::function<QVariant(const Command&)>> commandHandlers_{};
    QHash<QString, QList<std::function<bool(const Command&)>>> interceptors_{};

    void initialize_();

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

    void logCmdIntercepted_(const QString& id, const Command& cmd)
    {
        constexpr auto log_format =
            "\n\tIntercepted: \"%0\"\n\tParams: %1\n\tContext: %2";
        COCO_LOG_THIS(QString(log_format)
                          .arg(id)
                          .arg(toQString(cmd.params))
                          .arg(toQString(cmd.context)));
    }

    void
    logCmdRan_(const QString& id, const Command& cmd, const QVariant& result)
    {
        constexpr auto log_format = "\n\tExecuted: \"%0\"\n\tParams: "
                                    "%1\n\tContext: %2\n\tResult: %3";
        COCO_LOG_THIS(QString(log_format)
                          .arg(id)
                          .arg(toQString(cmd.params))
                          .arg(toQString(cmd.context))
                          .arg(toQString(result)));
    }

    void logCmdNoHandler_(const QString& id, const Command& cmd)
    {
        constexpr auto log_format =
            "\n\tNo handler found: \"%0\"\n\tParams: %1\n\tContext: %2";
        COCO_LOG_THIS(QString(log_format)
                          .arg(id)
                          .arg(toQString(cmd.params))
                          .arg(toQString(cmd.context)));
    }
};

} // namespace Fernanda
