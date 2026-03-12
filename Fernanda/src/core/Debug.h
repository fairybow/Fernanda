/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include <QObject>
#include <QString>
#include <QtLogging>

#include <Coco/Path.h>

#include "core/Formatters.h"

// TODO: Log to file. Commented-out method is too slow. Need to maybe keep file
// open the entire time, hold static QFile
namespace Fernanda::Debug {

namespace Internal {

    void dispatch_(
        QtMsgType type,
        const char* file,
        int line,
        const char* function,
        const QObject* obj,
        std::string msg);

} // namespace Internal

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
    print(const QObject* obj, std::string_view format, Args&&... args)
    {
        if (type != QtFatalMsg && !logging()) return;

        std::string msg{};

        if constexpr (sizeof...(args) > 0) {
            msg = std::vformat(format, std::make_format_args(args...));
        } else {
            msg = format;
        }

        Internal::dispatch_(type, file, line, function, obj, std::move(msg));
    }

    template <typename... Args>
    inline void print(std::string_view format, Args&&... args)
    {
        return print(nullptr, format, std::forward<Args>(args)...);
    }
};

} // namespace Fernanda::Debug

#define LOG(Level)                                                             \
    Fernanda::Debug::Log(Level, __FILE__, __LINE__, __FUNCTION__).print
#define INFO LOG(QtInfoMsg)
#define DEBUG LOG(QtDebugMsg)
#define WARN LOG(QtWarningMsg)
#define CRITICAL LOG(QtCriticalMsg)
#define FATAL LOG(QtFatalMsg)

#define TRACER INFO(__FUNCTION__)
