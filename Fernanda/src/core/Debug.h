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

#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include <QObject>
#include <QtLogging>

#include <Coco/Path.h>

#include "core/Formatters.h"
#include "core/ToString.h"
#include "core/Version.h"

namespace Fernanda::Debug {

using LogSink = std::function<void(const QString&)>;

// To be safe, don't call this before Qt has finished app construction
void initialize(
    bool verbose,
    const Coco::Path& logDir,
    const QString& logPrefix = {},
    int logCap = 15);

void setLogSink(LogSink sink);
QtMsgType minimumLevel() noexcept;

namespace Internal {

#ifdef Q_OS_MACOS

    // libc++ fails to match certain std::formatter specializations during its
    // internal formattability check. On macOS, affected args are pre-converted
    // to std::string instead
    template <typename T> decltype(auto) sanitizeArg_macOS_(T&& arg)
    {
        if constexpr (requires { Fernanda::toString(arg); }) {
            return Fernanda::toString(arg);
        } else if constexpr (requires {
                                 {
                                     arg.toString()
                                 } -> std::convertible_to<std::string>;
                             }) {
            return arg.toString();
        } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, QString>) {
            return arg.toStdString();
        } else {
            return std::forward<T>(arg);
        }
    }

#endif

} // namespace Internal

struct Log
{
    Log(QtMsgType type, const char* file, int line, const char* function)
        : type(type)
        , file(file)
        , line(line)
        , function(function)
    {
    }

    QtMsgType type;
    const char* file;
    int line;
    const char* function;

    template <typename... Args>
    inline void
    print(const QObject* obj, std::string_view format, Args&&... args) const
    {
        if (type != QtFatalMsg && type < minimumLevel()) return;

        std::string msg{};

        if constexpr (sizeof...(args) > 0) {

#ifndef Q_OS_MACOS
            msg = std::vformat(format, std::make_format_args(args...));
#else
            msg = std::vformat(
                format,
                std::make_format_args(Internal::sanitizeArg_macOS_(args)...));
#endif

        } else {
            msg = format;
        }

        dispatch_(type, file, line, function, obj, std::move(msg));
    }

    template <typename... Args>
    inline void print(std::string_view format, Args&&... args) const
    {
        return print(nullptr, format, std::forward<Args>(args)...);
    }

private:
    void dispatch_(
        QtMsgType type,
        const char* file,
        int line,
        const char* function,
        const QObject* obj,
        std::string msg) const;
};

namespace Internal {

    inline void assertionFailed_(
        const char* condition,
        const char* file,
        int line,
        const char* function,
        std::string_view message = {})
    {
        auto msg = message.empty()
                       ? std::format("Assertion failed:\n{}", condition)
                       : std::format(
                             "Assertion failed:\n{}\n\n{}",
                             condition,
                             message);

        Log(QtFatalMsg, file, line, function).print(msg);
    }

    template <typename... Args>
    inline void assertionFailed_(
        const char* condition,
        const char* file,
        int line,
        const char* function,
        std::string_view format,
        Args&&... args)
    {

#ifndef Q_OS_MACOS
        auto message = std::vformat(format, std::make_format_args(args...));
#else
        auto message = std::vformat(
            format,
            std::make_format_args(sanitizeArg_macOS_(args)...));
#endif

        assertionFailed_(condition, file, line, function, message);
    }

} // namespace Internal

} // namespace Fernanda::Debug

#define LOG(Level)                                                             \
    Fernanda::Debug::Log(Level, __FILE__, __LINE__, __FUNCTION__).print
#define DEBUG LOG(QtDebugMsg)
#define INFO LOG(QtInfoMsg)
#define WARN LOG(QtWarningMsg)
#define CRITICAL LOG(QtCriticalMsg)
#define FATAL LOG(QtFatalMsg)

#define TRACER DEBUG(__FUNCTION__)

#ifdef VERSION_DEBUG // TODO: Generalize / accept an arg?
#    define ASSERT(condition, ...)                                             \
        ((condition) ? static_cast<void>(0)                                    \
                     : Fernanda::Debug::Internal::assertionFailed_(            \
                           #condition,                                         \
                           __FILE__,                                           \
                           __LINE__,                                           \
                           __FUNCTION__,                                       \
                           ##__VA_ARGS__))
#else
#    define ASSERT(condition, ...) static_cast<void>(false && (condition))
#endif

#define UNREACHABLE(...)                                                       \
    Fernanda::Debug::Internal::assertionFailed_(                               \
        "UNREACHABLE",                                                         \
        __FILE__,                                                              \
        __LINE__,                                                              \
        __FUNCTION__,                                                          \
        ##__VA_ARGS__)
