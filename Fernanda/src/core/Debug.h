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

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include <QObject>
#include <QtLogging>

#include <Coco/Path.h>

#include "core/Formatters.h"
#include "core/Version.h"

// TODO: Log to file. Commented-out method is too slow. Need to maybe keep file
// open the entire time, hold static QFile
namespace Fernanda::Debug {

// To be safe, don't call this before Qt has finished app construction
void initialize(bool logging, const Coco::Path& logFilePath = {});
bool logging() noexcept;
void setLogging(bool logging);

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
        if (type != QtFatalMsg && !logging()) return;

        std::string msg{};

        if constexpr (sizeof...(args) > 0) {
            msg = std::vformat(format, std::make_format_args(args...));
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

} // namespace Internal

} // namespace Fernanda::Debug

#define LOG(Level)                                                             \
    Fernanda::Debug::Log(Level, __FILE__, __LINE__, __FUNCTION__).print
#define INFO LOG(QtInfoMsg)
#define DEBUG LOG(QtDebugMsg)
#define WARN LOG(QtWarningMsg)
#define CRITICAL LOG(QtCriticalMsg)
#define FATAL LOG(QtFatalMsg)

#define TRACER INFO(__FUNCTION__)

#ifdef VERSION_DEBUG
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

// TODO: Add secondary message parameter? (Here or separate macro)
// #ifdef VERSION_DEBUG
// #    define ASSERT(cond, ...) \
//        do { \
//            if (!(cond)) FATAL("Assertion failed: {}", #cond); \
//        } while (0)
// #else
// #    define ASSERT(cond, ...) (static_cast<void>(0))
// #endif
