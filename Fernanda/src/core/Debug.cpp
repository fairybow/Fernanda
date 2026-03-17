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

#include "core/Debug.h"

#include <atomic>
#include <format>
#include <mutex>
#include <string>
#include <string_view>

#include <QByteArray>
#include <QMessageLogContext>
#include <QMessageLogger>
#include <QObject>
#include <QString>
#include <QtLogging>

#include <Coco/Path.h>

#include "Time.h"

namespace Fernanda::Debug {

namespace {

    constexpr auto VOC_FORMAT_ = "In {}: {}";
    constexpr auto MSG_FORMAT_ = "{} | {} | {}";

    std::atomic<bool> logging_{ false };
    // std::atomic<bool> firstWrite{ true };
    std::atomic<uint64_t> logCount_{ 0 };

    std::mutex mutex_{};
    // Coco::Path logFilePath_{};
    QtMessageHandler qtHandler_ = nullptr;

    std::string timestamp_()
    {
        constexpr auto format = "{:%Y-%m-%d | %H:%M:%S}.{:03d}";
        auto now = Time::now();
        return std::format(format, now.seconds, now.milliseconds);
    }

    void handler_(
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

} // namespace

void initialize(bool logging, const Coco::Path& logFilePath)
{
    setLogging(logging);

    std::lock_guard<std::mutex> lock(mutex_);
    // logFilePath_ = logFilePath;
    qtHandler_ = qInstallMessageHandler(handler_);
}

bool logging() noexcept { return logging_.load(std::memory_order::relaxed); }

void setLogging(bool logging)
{
    logging_.store(logging, std::memory_order::relaxed);
}

void Log::dispatch_(
    QtMsgType type,
    const char* file,
    int line,
    const char* function,
    const QObject* obj,
    std::string msg) const
{
    // Right now, VOC_FORMAT_ is the only reason this needs to be in the
    // source file, which is fine, but worth pointing out
    if (obj) { msg = std::format(VOC_FORMAT_, obj, msg); }

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

} // namespace Fernanda::Debug
