#pragma once

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

#include "Utility.h"

namespace Fernanda {

class Window;

// Commands are operations registered in their respective domains that can be
// run in any class using the Workspace's Commander. Commands take a Command
// struct argument, but it's optional
namespace Commands {

    // Scheme: type.domain.scope:action. The domain is the handler-registering
    // class. The scopes are the logical hierarchy of the program: Application,
    // Workspace, or Window.

    constexpr auto NewWindow = "cmd.workspace.workspace:new_window";
    constexpr auto CloseWindow = "cmd.workspace.workspace:close_window";
    constexpr auto CloseAllWindows =
        "cmd.workspace.workspace:close_all_windows";
    constexpr auto Quit = "cmd.workspace.application:quit";
    constexpr auto SettingsDialog = "cmd.workspace.workspace:settings_dialog";
    constexpr auto AboutDialog = "cmd.workspace.application:about_dialog";

    constexpr auto NewTab = "cmd.file_serv.window:new_tab";
    constexpr auto OpenFile = "cmd.file_serv.window:open_file";

    // A file can be open anywhere in the Workspace and is not tied to just a
    // window

    constexpr auto Undo = "cmd.view_serv.workspace:undo";
    constexpr auto Redo = "cmd.view_serv.workspace:redo";
    constexpr auto Cut = "cmd.view_serv.workspace:cut";
    constexpr auto Copy = "cmd.view_serv.workspace:copy";
    constexpr auto Paste = "cmd.view_serv.workspace:paste";
    constexpr auto Delete = "cmd.view_serv.workspace:delete";
    constexpr auto SelectAll = "cmd.view_serv.workspace:select_all";
    constexpr auto PreviousTab = "cmd.view_serv.window:previous_tab";
    constexpr auto NextTab = "cmd.view_serv.window:next_tab";

    constexpr auto PreviousWindow = "cmd.window_serv.workspace:previous_window";
    constexpr auto ViewNextWindow = "cmd.window_serv.workspace:next_window";

    constexpr auto SetSetting = "cmd.settings_mod.workspace:set";

} // namespace Commands

// Calls are Commands that return a value (they can also be executed as Commands
// with no return value)
namespace Calls {

    constexpr auto NewTreeViewModel = "call.workspace.workspace:new_tree_view_model";

    constexpr auto Save = "call.file_serv.workspace:save";
    constexpr auto SaveAs = "call.file_serv.workspace:save_as";
    constexpr auto SaveIndexesInWindow =
        "call.file_serv.workspace:save_indexes_in_window";
    constexpr auto SaveWindow = "call.file_serv.workspace:save_window";
    constexpr auto SaveAll = "call.file_serv.workspace:save_all";

    // While files are Workspace scope, file views are not. Additionally, rename
    // these constants, because we don't close files directly, we close views!

    constexpr auto CloseView = "call.view_serv.window:close_view";
    constexpr auto CloseWindowViews =
        "call.view_serv.window:close_window_views";
    constexpr auto CloseAllViews = "call.view_serv.workspace:close_all_views";

} // namespace Calls

// Queries are for returning read-only values and take optional parameters
namespace Queries {

    constexpr auto Root = "query.workspace.workspace:root";

    constexpr auto ActiveWindow = "query.window_serv.workspace:active_window";
    constexpr auto WindowList = "query.window_serv.workspace:list";
    constexpr auto ReverseWindowList =
        "query.window_serv.workspace:reverse_list";
    constexpr auto WindowSet = "query.window_serv.workspace:set";

    // This is for windows not shown (NOT minimized)
    constexpr auto VisibleWindowCount =
        "query.window_serv.workspace:visible_count";

    constexpr auto ActiveFileView = "query.view_serv.window:active_file_view";
    constexpr auto ViewCountForModel =
        "query.view_serv.workspace:model_view_count";
    constexpr auto WindowAnyViewsOnModifiedFiles =
        "query.view_serv.window:has_modified_files";
    constexpr auto WindowAnyFiles = "query.view_serv.window:has_any_files";
    constexpr auto WorkspaceAnyViewsOnModifiedFiles =
        "query.view_serv.workspace:has_modified_files";
    constexpr auto WorkspaceAnyFiles =
        "query.view_serv.workspace:has_any_files";

    constexpr auto Setting = "query.settings_mod.workspace:get";

} // namespace Queries

struct Command
{
    QString id{};
    QVariantMap params{};
    Window* context = nullptr;

    Command(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
        : id(id)
        , params(params)
        , context(context)
    {
    }
};

struct Call : Command
{
    using Command::Command;
};

// Centralized command dispatch system for use by the Workspace's Services and
// Modules
class Commander : public QObject
{
    Q_OBJECT

public:
    using CommandHandler = std::function<void(const Command&)>;
    using CallHandler = std::function<QVariant(const Call&)>;
    using QueryHandler = std::function<QVariant(const QVariantMap&)>;
    using Interceptor = std::function<bool(Command&)>; // Const ref instead?

    explicit Commander(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~Commander() override { COCO_TRACER; }

    void addInterceptor(const QString& id, Interceptor interceptor)
    {
        interceptors_[id] << interceptor;
    }

    template <typename CommandHandlerT>
    void addCommandHandler(const QString& id, CommandHandlerT&& handler)
    {
        if constexpr (std::is_invocable_v<CommandHandlerT, const Command&>) {
            cmdHandlers_[id] = std::forward<CommandHandlerT>(handler);
        } else if constexpr (std::is_invocable_v<CommandHandlerT>) {
            // Wrap parameterless handler
            cmdHandlers_[id] = [handler = std::forward<CommandHandlerT>(
                                    handler)](const Command& cmd) {
                (void)cmd;
                handler();
            };
        } else {
            static_assert(
                std::is_invocable_v<CommandHandlerT>
                    || std::is_invocable_v<CommandHandlerT, const Command&>,
                "Handler must be callable with either no parameters or const "
                "Command&");
        }
    }

    void execute(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
    {
        execute({ id, params, context });
    }

    void execute(const Command& cmd)
    {
        Command mutable_cmd = cmd;

        for (auto& interceptor : interceptors_[cmd.id]) {
            if (interceptor(mutable_cmd)) {
                logCmdIntercepted_(cmd);
                return;
            }
        }

        if (auto handler = cmdHandlers_.value(cmd.id)) {
            handler(cmd);
            logCmdExecuted_(cmd);
        } else {
            logCmdNoHandler_(cmd);
        }
    }

    template <typename CallHandlerT>
    void addCallHandler(const QString& id, CallHandlerT&& handler)
    {
        // To allow dual usage with or without parameters
        CallHandler call_handler{};

        if constexpr (std::is_invocable_v<CallHandlerT, const Call&>) {
            call_handler = std::forward<CallHandlerT>(handler);
        } else if constexpr (std::is_invocable_v<CallHandlerT>) {
            // Wrap parameterless handler
            call_handler = [handler = std::forward<CallHandlerT>(handler)](
                               const Call& call) {
                (void)call;
                return handler();
            };
        } else {
            static_assert(
                std::is_invocable_v<CallHandlerT>
                    || std::is_invocable_v<CallHandlerT, const Call&>,
                "Handler must be callable with either no parameters or const "
                "Call&");
        }

        callHandlers_[id] = call_handler;

        // Also register as command handler that ignores return value
        cmdHandlers_[id] = [call_handler](const Command& cmd) {
            (void)call_handler(static_cast<const Call&>(cmd));
        };
    }

    template <typename T>
    [[nodiscard]] T call(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
    {
        return call({ id, params, context }).value<T>();
    }

    [[nodiscard]] QVariant call(
        const QString& id,
        const QVariantMap& params = {},
        Window* context = nullptr)
    {
        return call({ id, params, context });
    }

    template <typename T> [[nodiscard]] T call(const Call& c)
    {
        return call(c).value<T>();
    }

    [[nodiscard]] QVariant call(const Call& c)
    {
        QVariant result{};

        if (auto handler = callHandlers_.value(c.id)) {
            result = handler(c);
            logCmdExecuted_(c, result);
        } else {
            logCmdNoHandler_(c);
        }

        return result;
    }

    template <typename QueryHandlerT>
    void addQueryHandler(const QString& id, QueryHandlerT&& handler)
    {
        if constexpr (std::is_invocable_v<QueryHandlerT, const QVariantMap&>) {
            queryHandlers_[id] = std::forward<QueryHandlerT>(handler);
        } else if constexpr (std::is_invocable_v<QueryHandlerT>) {
            // Wrap parameterless handler
            queryHandlers_[id] = [handler = std::forward<QueryHandlerT>(
                                      handler)](const QVariantMap& params) {
                (void)params;
                return handler();
            };
        } else {
            static_assert(
                std::is_invocable_v<QueryHandlerT>
                    || std::is_invocable_v<QueryHandlerT, const QVariantMap&>,
                "Handler must be callable with either no parameters or const "
                "QVariantMap&");
        }
    }

    template <typename T>
    [[nodiscard]] T query(const QString& id, const QVariantMap& params = {})
    {
        return query(id, params).value<T>();
    }

    [[nodiscard]] QVariant
    query(const QString& id, const QVariantMap& params = {})
    {
        QVariant answer{};

        if (auto handler = queryHandlers_.value(id)) {
            answer = handler(params);
            logQueryExecuted_(id, params, answer);
        } else {
            logQueryNoHandler_(id, params);
        }

        return answer;
    }

private:
    QHash<QString, CommandHandler> cmdHandlers_{};
    QHash<QString, CallHandler> callHandlers_{};
    QHash<QString, QueryHandler> queryHandlers_{};
    QHash<QString, QList<Interceptor>> interceptors_{};

    void logCmdIntercepted_(const Command& cmd)
    {
        constexpr auto log_format =
            "\n\tIntercepted: \"%0\"\n\tParams: %1\n\tContext: %2";
        COCO_LOG_THIS(QString(log_format)
                          .arg(cmd.id)
                          .arg(toQString(cmd.params))
                          .arg(toQString(cmd.context)));
    }

    void logCmdExecuted_(const Command& cmd, const QVariant& result = {})
    {
        if (result.isNull()) {
            constexpr auto log_format =
                "\n\tExecuted: \"%0\"\n\tParams: %1\n\tContext: %2";
            COCO_LOG_THIS(QString(log_format)
                              .arg(cmd.id)
                              .arg(toQString(cmd.params))
                              .arg(toQString(cmd.context)));
        } else {
            constexpr auto log_format = "\n\tExecuted: \"%0\"\n\tParams: "
                                        "%1\n\tContext: %2\n\tResult: %3";
            COCO_LOG_THIS(QString(log_format)
                              .arg(cmd.id)
                              .arg(toQString(cmd.params))
                              .arg(toQString(cmd.context))
                              .arg(toQString(result)));
        }
    }

    void logCmdNoHandler_(const Command& cmd)
    {
        constexpr auto log_format =
            "\n\tNo handler found: \"%0\"\n\tParams: %1\n\tContext: %2";
        COCO_LOG_THIS(QString(log_format)
                          .arg(cmd.id)
                          .arg(toQString(cmd.params))
                          .arg(toQString(cmd.context)));
    }

    void logQueryExecuted_(
        const QString& id,
        const QVariantMap& params,
        const QVariant& result)
    {
        constexpr auto log_format =
            "\n\tExecuted: \"%0\"\n\tParams: %1\n\tResult: %2";
        COCO_LOG_THIS(QString(log_format)
                          .arg(id)
                          .arg(toQString(params))
                          .arg(toQString(result)));
    }

    void logQueryNoHandler_(const QString& id, const QVariantMap& params)
    {
        constexpr auto log_format =
            "\n\tNo handler found: \"%0\"\n\tParams: %1";
        COCO_LOG_THIS(QString(log_format).arg(id).arg(toQString(params)));
    }
};

} // namespace Fernanda
