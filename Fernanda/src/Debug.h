/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <mutex>
#include <string>
#include <utility>

#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>
#include <QMessageLogContext>
#include <QString>
#include <QtLogging>
#include <QObject>

#include "Coco/Path.h"

#include "Formatters.h"

namespace Fernanda::Debug {

namespace Internal {

    constexpr auto TIMESTAMP_FORMAT = "HH:mm:ss:zz";
    constexpr auto MSG_FORMAT = "{} | {} | {}";

    static std::atomic<bool> logging{ false };
    static std::atomic<uint64_t> logCount{ 0 };
    static std::mutex handlerMutex{};
    static QtMessageHandler qtHandler = nullptr;

    static void handler(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg)
    {
        if (!logging.load(std::memory_order::relaxed)) return;

        auto count = logCount.fetch_add(1, std::memory_order::relaxed);
        auto timestamp = QDateTime::currentDateTime().toString(
            TIMESTAMP_FORMAT); /// Use something faster? Cache?
        auto new_msg = std::format(MSG_FORMAT, count, timestamp, msg);

        QtMessageHandler qt_handler = nullptr;

        {
            std::lock_guard<std::mutex> lock(handlerMutex);
            qt_handler = qtHandler;
        }

        if (qt_handler) qt_handler(type, context, QString::fromUtf8(new_msg));
    }

} // namespace Internal

inline bool logging() noexcept
{
    return Internal::logging.load(std::memory_order::relaxed);
}

inline void setLogging(bool logging)
{
    Internal::logging.store(logging, std::memory_order::relaxed);
}

inline void initialize(bool logging, const Coco::Path& logFile = {})
{
    setLogging(logging);

    std::lock_guard<std::mutex> lock(Internal::handlerMutex);
    Internal::qtHandler = qInstallMessageHandler(Internal::handler);

    /// Set log file!
}

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

    // Could subclass QMessageLogContext and add an optional "this" pointer, and provide a print overload that takes "this" as first arg?
    // Would have to cast the QMessageLogContext back to the subclass in the handler... Would it work? Bad idea?

    template <typename... Args>
    inline void print(const char* format, Args&&... args)
    {
        if (!Internal::logging.load(std::memory_order::relaxed)) return;
        auto context = QMessageLogContext(file, line, function, nullptr);

        if constexpr (sizeof...(args) > 0) {
            auto msg = std::vformat(format, std::make_format_args(args...));
            Internal::handler(type, context, QString::fromUtf8(msg));
        } else {
            Internal::handler(type, context, QString::fromUtf8(format));
        }
    }

    /*template <typename... Args>
    inline void print(const char* format, Args&&... args)
    {
        print(nullptr, format, std::forward<Args>(args)...);
    }*/
};

} // namespace Fernanda::Debug

#define LOG(Level)                                                             \
    Fernanda::Debug::Log(Level, __FILE__, __LINE__, __FUNCTION__).print
#define INFO LOG(QtInfoMsg)
#define DEBUG LOG(QtDebugMsg)
#define WARN LOG(QtWarningMsg)
#define CRITICAL LOG(QtCriticalMsg)
#define FATAL LOG(QtFatalMsg)

// Temp?
#define TRACER INFO(__FUNCTION__)

/// Old:

// #include <QFile>
// #include <QTextStream>
// void logToFile(const QString& message) {
//     QFile file("fernanda_appguard_debug.log");
//     if (file.open(QIODevice::Append | QIODevice::Text)) {
//         QTextStream out(&file);
//         out << message << '\n';
//         file.close();
//     }
// }

// #include <QChar>
// #include <QMessageLogContext>
//
// static void _maybePrependNewline(QString& msg)
//{
//     // Local statics are only guaranteed thread-safe initialization, not
//     usage!
//     // (C++ 11 and on)
//     static QChar last_char{};
//     static std::mutex mutex{};
//
//     std::lock_guard<std::mutex> lock(mutex);
//
//     // `msg` will never be empty. It will always at least have the timestamp
//     and
//     // etc.
//     if (!last_char.isNull() && last_char != '\n') msg.prepend('\n');
//
//     last_char = msg.back();
// }
