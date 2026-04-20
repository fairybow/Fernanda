/*
 * Hearth — a plain-text-first workbench for creative writing
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
#include <functional>
#include <mutex>
#include <utility>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QMessageLogContext>
#include <QMessageLogger>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QtLogging>

#include <Coco/Path.h>

#include "core/Disk.h"
#include "core/Time.h"
#include "fmt/Fmt.h"

namespace Fernanda::Debug {

using namespace Qt::StringLiterals;

namespace {

    constexpr auto VOC_FORMAT_ = u"In {}: {}";
    constexpr auto MSG_FORMAT_ = u"{} | {} | {}";
    auto LOG_EXT_ = u".log"_s;

    std::atomic<QtMsgType> minimumLevel_{ QtFatalMsg };
    std::atomic<uint64_t> logEntryCount_{ 0 };
    Coco::Path logDir_{};
    QString logPrefix_{};
    std::mutex mutex_{};
    QFile logFile_{};
    QTextStream logStream_{};
    LogSink logSink_{};
    QtMessageHandler qtHandler_ = nullptr;

    QString timestamp_()
    {
        auto now = Time::now();
        auto days = std::chrono::floor<std::chrono::days>(now.seconds);
        std::chrono::year_month_day ymd{ days };
        std::chrono::hh_mm_ss hms{ now.seconds - days };

        return QString::asprintf(
            "%04d-%02d-%02d | %02d:%02d:%02d.%03d",
            static_cast<int>(ymd.year()),
            static_cast<unsigned>(ymd.month()),
            static_cast<unsigned>(ymd.day()),
            static_cast<int>(hms.hours().count()),
            static_cast<int>(hms.minutes().count()),
            static_cast<int>(hms.seconds().count()),
            static_cast<int>(now.milliseconds));
    }

    QString logFileName_()
    {
        auto now = Time::now();
        auto days = std::chrono::floor<std::chrono::days>(now.seconds);
        std::chrono::year_month_day ymd{ days };
        std::chrono::hh_mm_ss hms{ now.seconds - days };

        auto timestamp = QString::asprintf(
            "%04d-%02d-%02d_%02d.%02d.%02d",
            static_cast<int>(ymd.year()),
            static_cast<unsigned>(ymd.month()),
            static_cast<unsigned>(ymd.day()),
            static_cast<int>(hms.hours().count()),
            static_cast<int>(hms.minutes().count()),
            static_cast<int>(hms.seconds().count()));

        return logPrefix_.isEmpty() ? timestamp + LOG_EXT_
                                    : logPrefix_ + "_" + timestamp + LOG_EXT_;
    }

    void handler_(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg)
    {
        if (type != QtFatalMsg
            && type < minimumLevel_.load(std::memory_order::relaxed)) {
            return;
        }

        auto count = logEntryCount_.fetch_add(1, std::memory_order::relaxed);
        auto new_msg = Fmt::format(MSG_FORMAT_, count, timestamp_(), msg);

        QtMessageHandler qt_handler = nullptr;
        LogSink log_sink{};

        {
            std::lock_guard<std::mutex> lock(mutex_);
            qt_handler = qtHandler_;
            log_sink = logSink_;

            if (logStream_.device()) logStream_ << new_msg << Qt::endl;
        }

        if (log_sink) log_sink(new_msg);
        if (qt_handler) qt_handler(type, context, new_msg);
    }

} // namespace

void initialize(
    bool verbose,
    const Coco::Path& logDir,
    const QString& logPrefix,
    int logCap)
{
    minimumLevel_.store(
        verbose ? QtDebugMsg : QtInfoMsg,
        std::memory_order::relaxed);

    std::lock_guard<std::mutex> lock(mutex_);

    if (!logDir.isEmpty()) {
        logDir_ = logDir;
        logPrefix_ = logPrefix;

        auto path = logDir / logFileName_();
        logFile_.setFileName(path.toQString());

        if (logFile_.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logStream_.setDevice(&logFile_);
            Disk::prune(logDir, logPrefix, LOG_EXT_, logCap);
            logStream_ << "VERBOSITY: " << (verbose ? "true" : "false")
                       << Qt::endl;
        }
    }

    qtHandler_ = qInstallMessageHandler(handler_);
}

void setLogSink(LogSink sink)
{
    std::lock_guard<std::mutex> lock(mutex_);
    logSink_ = std::move(sink);
}

QtMsgType minimumLevel() noexcept
{
    return minimumLevel_.load(std::memory_order::relaxed);
}

void Log::dispatch_(
    QtMsgType type,
    const char* file,
    int line,
    const char* function,
    const QObject* obj,
    QString msg) const
{
    // Right now, VOC_FORMAT_ is the only reason this needs to be in the
    // source file, which is fine, but worth pointing out
    if (obj) msg = Fmt::format(VOC_FORMAT_, obj, msg);

    auto logger = QMessageLogger(file, line, function);
    constexpr auto fmt = "%s";
    auto utf8 = msg.toUtf8();

    switch (type) {
    case QtDebugMsg:
        logger.debug(fmt, utf8.constData());
        break;
    case QtInfoMsg:
        logger.info(fmt, utf8.constData());
        break;
    case QtWarningMsg:
        logger.warning(fmt, utf8.constData());
        break;
    case QtCriticalMsg:
        logger.critical(fmt, utf8.constData());
        break;
    default:
    case QtFatalMsg:
        logger.fatal(fmt, utf8.constData());
        break;
    }
}

} // namespace Fernanda::Debug
