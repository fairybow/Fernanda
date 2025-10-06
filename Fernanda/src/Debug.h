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
#include <chrono>
#include <cstdint>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QLoggingCategory>
#include <QMessageLogContext>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QtLogging>

#include "Coco/Path.h"

#include "Formatters.h"
#include "ToString.h"

// TODO: Log to file. Commented-out method is too slow. Need to maybe keep file
// open the entire time, hold static QFile
// TODO: Move Internal to .cpp
namespace Fernanda::Debug {

namespace Internal {

    constexpr auto TIMESTAMP_FORMAT = "{:%Y-%m-%d | %H:%M:%S}.{:03d}";
    constexpr auto VOC_FORMAT = "In {}: {}";
    constexpr auto MSG_FORMAT = "{} | {} | {}";

    static std::atomic<bool> logging{ false };
    // static std::atomic<bool> firstWrite{ true };
    static std::atomic<uint64_t> logCount{ 0 };

    static std::mutex mutex{};
    // static Coco::Path logFilePath{};
    static QtMessageHandler qtHandler = nullptr;

    static std::string timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto zone = std::chrono::current_zone();
        auto local_time = zone->to_local(now);

        return std::format(
            TIMESTAMP_FORMAT,
            std::chrono::floor<std::chrono::seconds>(local_time),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                local_time.time_since_epoch() % std::chrono::seconds{ 1 })
                .count());
    }

    static void handler(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg)
    {
        if (!logging.load(std::memory_order::relaxed)) return;

        auto count = logCount.fetch_add(1, std::memory_order::relaxed);
        auto new_msg = std::format(MSG_FORMAT, count, timestamp(), msg);

        QtMessageHandler qt_handler = nullptr;

        {
            std::lock_guard<std::mutex> lock(mutex);
            qt_handler = qtHandler;

            /*if (!logFilePath.isEmpty()) {
                auto expected = true;
                auto truncate = firstWrite.compare_exchange_strong(
                    expected,
                    false,
                    std::memory_order::relaxed);

                auto mode = QIODevice::WriteOnly | QIODevice::Text;
                mode |= truncate ? QIODevice::Truncate : QIODevice::Append;
                QFile file(logFilePath.toQString());

                if (file.open(mode)) {
                    QTextStream stream(&file);
                    stream << QString::fromUtf8(new_msg) << Qt::endl;
                    file.close();
                }
            }*/
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

inline void initialize(bool logging, const Coco::Path& logFilePath = {})
{
    setLogging(logging);

    std::lock_guard<std::mutex> lock(Internal::mutex);
    // Internal::logFilePath = logFilePath;
    Internal::qtHandler = qInstallMessageHandler(Internal::handler);
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

    template <typename... Args>
    inline void
    print(const QObject* obj, std::string_view format, Args&&... args)
    {
        if (!Internal::logging.load(std::memory_order::relaxed)) return;
        auto context = QMessageLogContext(file, line, function, nullptr);

        std::string msg{};

        if constexpr (sizeof...(args) > 0) {
            msg = std::vformat(format, std::make_format_args(args...));
        } else {
            msg = format;
        }

        if (obj) {
            msg = std::format(Internal::VOC_FORMAT, toString(obj), msg);
        }

        Internal::handler(type, context, QString::fromUtf8(msg));
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

// Temp implementation?
#define TRACER INFO(__FUNCTION__)
