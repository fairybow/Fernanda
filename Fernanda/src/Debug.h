/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
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

// TODO: Log to file. Commented-out method is too slow. Need to maybe keep file
// open the entire time, hold static QFile
// TODO: Move Internal to .cpp (note that print is a template and uses some of
// these, though...) - just do one at a time, starting with the methods?
namespace Fernanda::Debug {

namespace Internal {

    constexpr auto TIMESTAMP_FORMAT_ = "{:%Y-%m-%d | %H:%M:%S}.{:03d}";
    constexpr auto VOC_FORMAT_ = "In {}: {}";
    constexpr auto MSG_FORMAT_ = "{} | {} | {}";

    inline std::atomic<bool> logging_{ false };
    // inline std::atomic<bool> firstWrite{ true };
    inline std::atomic<uint64_t> logCount_{ 0 };

    inline std::mutex mutex_{};
    // inline  Coco::Path logFilePath_{};
    inline QtMessageHandler qtHandler_ = nullptr;

    static std::string timestamp_()
    {
        auto now = std::chrono::system_clock::now();
        auto zone = std::chrono::current_zone();
        auto local_time = zone->to_local(now);

        return std::format(
            TIMESTAMP_FORMAT_,
            std::chrono::floor<std::chrono::seconds>(local_time),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                local_time.time_since_epoch() % std::chrono::seconds{ 1 })
                .count());
    }

    static void handler_(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg)
    {
        if (type != QtFatalMsg && !logging_.load(std::memory_order::relaxed))
            return;

        auto count = logCount_.fetch_add(1, std::memory_order::relaxed);
        auto msg_str = msg.toUtf8();
        auto new_msg = std::format(
            MSG_FORMAT_,
            count,
            timestamp_(),
            std::string_view(msg_str.constData(), msg_str.size()));

        QtMessageHandler qt_handler = nullptr;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            qt_handler = qtHandler_;

            /*if (!logFilePath_.isEmpty()) {
                auto expected = true;
                auto truncate = firstWrite.compare_exchange_strong(
                    expected,
                    false,
                    std::memory_order::relaxed);

                auto mode = QIODevice::WriteOnly | QIODevice::Text;
                mode |= truncate ? QIODevice::Truncate : QIODevice::Append;
                QFile file(logFilePath_.toQString());

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
    return Internal::logging_.load(std::memory_order::relaxed);
}

inline void setLogging(bool logging)
{
    Internal::logging_.store(logging, std::memory_order::relaxed);
}

// To be safe, don't call this before Qt has finished app construction
inline void initialize(bool logging, const Coco::Path& logFilePath = {})
{
    setLogging(logging);

    std::lock_guard<std::mutex> lock(Internal::mutex_);
    // Internal::logFilePath_ = logFilePath;
    Internal::qtHandler_ = qInstallMessageHandler(Internal::handler_);
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
        if (type != QtFatalMsg
            && !Internal::logging_.load(std::memory_order::relaxed))
            return;

        // Formatters for Qt types defined in Formatters.h
        std::string msg{};

        if constexpr (sizeof...(args) > 0) {
            msg = std::vformat(format, std::make_format_args(args...));
        } else {
            msg = format;
        }

        if (obj) { msg = std::format(Internal::VOC_FORMAT_, obj, msg); }

        auto logger = QMessageLogger(file, line, function);
        constexpr auto fmt = "%s";

        switch (type) {
        case QtDebugMsg:
            logger.debug(fmt, msg.c_str());
            break;
        case QtInfoMsg:
            logger.info(fmt, msg.c_str());
            break;
        case QtWarningMsg:
            logger.warning(fmt, msg.c_str());
            break;
        case QtCriticalMsg:
            logger.critical(fmt, msg.c_str());
            break;
        default:
        case QtFatalMsg:
            logger.fatal(fmt, msg.c_str());
            break;
        }
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
