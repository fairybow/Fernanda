/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTest>
#include <QTime>
#include <QtTypes>

#include "core/CoreConstants.h"
#include "core/WritingHistory.h"

using namespace Fernanda;
using namespace Qt::StringLiterals;

// A vault's writing history (core/WritingHistory.h). The history takes the
// time as an argument, so every case runs on made-up local times: nothing here
// waits. The dates are in June, away from any daylight saving change
class WritingHistoryTest : public QObject
{
    Q_OBJECT

private slots:
    void anEmptyHistoryHasNothing()
    {
        WritingHistory history{};

        QVERIFY(history.day(DAY_).isEmpty());
        QVERIFY(history.lifetime().isEmpty());
        QCOMPARE(history.daysWritten(), 0);
    }

    void anEditAddsItsDeltaToTheDaysNetWords()
    {
        WritingHistory history{};

        QVERIFY(history.recordEdit(at_(DAY_, 9, 0), 3));
        QVERIFY(history.recordEdit(at_(DAY_, 9, 1), 4));

        QCOMPARE(history.day(DAY_).netWords, 7);
    }

    void onlyGainsCountAsAddedWords()
    {
        WritingHistory history{};
        history.recordEdit(at_(DAY_, 9, 0), 5);
        history.recordEdit(at_(DAY_, 9, 1), -3);

        QCOMPARE(history.day(DAY_).netWords, 2);
        QCOMPARE(history.day(DAY_).addedWords, 5);
    }

    void aDayThatCutsMoreThanItWroteHasNegativeNetWords()
    {
        WritingHistory history{};
        history.recordEdit(at_(DAY_, 9, 0), -10);

        QCOMPARE(history.day(DAY_).netWords, -10);
        QCOMPARE(history.day(DAY_).addedWords, 0);
        QCOMPARE(history.daysWritten(), 1);
    }

    void theFirstEditAddsNoActiveTime()
    {
        WritingHistory history{};
        history.recordEdit(at_(DAY_, 9, 0), 1);

        QCOMPARE(history.day(DAY_).activeMs, 0);
    }

    void gapsWithinTheIdleLimitAddActiveTime()
    {
        WritingHistory history{};
        auto start = at_(DAY_, 9, 0);

        history.recordEdit(start, 1);
        history.recordEdit(start + 1000, 1);
        history.recordEdit(start + 1000 + WRITING_IDLE_LIMIT_MS, 1);

        QCOMPARE(history.day(DAY_).activeMs, 1000 + WRITING_IDLE_LIMIT_MS);
    }

    void aGapPastTheIdleLimitAddsNoActiveTime()
    {
        WritingHistory history{};
        auto start = at_(DAY_, 9, 0);

        history.recordEdit(start, 1);
        history.recordEdit(start + WRITING_IDLE_LIMIT_MS + 1, 1);
        history.recordEdit(start + WRITING_IDLE_LIMIT_MS + 501, 1);

        QCOMPARE(history.day(DAY_).activeMs, 500);
    }

    void anEditWithNoDeltaStillCountsAsActivity()
    {
        WritingHistory history{};
        auto start = at_(DAY_, 9, 0);

        QVERIFY(!history.recordEdit(start, 0));
        QVERIFY(history.recordEdit(start + 2000, 0));

        QCOMPARE(history.day(DAY_).netWords, 0);
        QCOMPARE(history.day(DAY_).activeMs, 2000);
    }

    void aClockSetBackAddsNoActiveTime()
    {
        WritingHistory history{};
        auto start = at_(DAY_, 9, 0);

        history.recordEdit(start, 1);
        history.recordEdit(start - 1000, 1);

        QCOMPARE(history.day(DAY_).activeMs, 0);
    }

    void aGapAcrossMidnightCountsTowardTheNewDay()
    {
        WritingHistory history{};
        auto next_day = DAY_.addDays(1);
        auto before = at_(DAY_, 23, 59);
        auto after = at_(next_day, 0, 1);

        history.recordEdit(before, 2);
        history.recordEdit(after, 3);

        QCOMPARE(history.day(DAY_).netWords, 2);
        QCOMPARE(history.day(DAY_).activeMs, 0);
        QCOMPARE(history.day(next_day).netWords, 3);
        QCOMPARE(history.day(next_day).activeMs, after - before);
        QCOMPARE(history.daysWritten(), 2);
    }

    void lifetimeSumsEveryDay()
    {
        WritingHistory history{};
        history.recordEdit(at_(DAY_, 9, 0), 4);
        history.recordEdit(at_(DAY_.addDays(3), 9, 0), -1);

        auto lifetime = history.lifetime();
        QCOMPARE(lifetime.netWords, 3);
        QCOMPARE(lifetime.addedWords, 4);
    }

    void jsonRoundTripKeepsEveryDay()
    {
        WritingHistory history{};
        auto start = at_(DAY_, 9, 0);
        history.recordEdit(start, 5);
        history.recordEdit(start + 1000, -2);
        history.recordEdit(at_(DAY_.addDays(1), 9, 0), 7);

        auto read = WritingHistory::fromJson(history.toJson());

        QCOMPARE(read.day(DAY_).netWords, 3);
        QCOMPARE(read.day(DAY_).addedWords, 5);
        QCOMPARE(read.day(DAY_).activeMs, 1000);
        QCOMPARE(read.day(DAY_.addDays(1)).netWords, 7);
        QCOMPARE(read.daysWritten(), 2);
    }

    void jsonHoldsOnlyThisMachinesDays()
    {
        WritingHistory history{};
        history.setOtherMachines({ machineJson_(DAY_, 100, 100, 0) });
        history.recordEdit(at_(DAY_, 9, 0), 1);

        auto read = WritingHistory::fromJson(history.toJson());

        QCOMPARE(read.day(DAY_).netWords, 1);
    }

    void otherMachinesAddToEveryQuery()
    {
        WritingHistory history{};
        history.recordEdit(at_(DAY_, 9, 0), 1);
        history.setOtherMachines(
            {
                machineJson_(DAY_, 10, 12, 60000),
                machineJson_(DAY_.addDays(1), 20, 20, 0),
            });

        QCOMPARE(history.day(DAY_).netWords, 11);
        QCOMPARE(history.day(DAY_).addedWords, 13);
        QCOMPARE(history.day(DAY_).activeMs, 60000);
        QCOMPARE(history.lifetime().netWords, 31);
        QCOMPARE(history.daysWritten(), 2);
    }

    void setOtherMachinesReplacesWhatWasReadBefore()
    {
        WritingHistory history{};
        history.setOtherMachines({ machineJson_(DAY_, 10, 10, 0) });
        history.setOtherMachines({ machineJson_(DAY_, 4, 4, 0) });

        QCOMPARE(history.day(DAY_).netWords, 4);
    }

    void damagedEntriesAreSkipped()
    {
        QJsonObject days{};
        days[u"not a date"_s] = dayJson_(1, 1, 0);
        days[u"2026-06-11"_s] = 42;
        days[u"2026-06-12"_s] = dayJson_(0, 0, 0);
        days[DAY_.toString(Qt::ISODate)] = dayJson_(-2, -5, -100);

        QJsonObject json{};
        json[u"days"_s] = days;

        auto history = WritingHistory::fromJson(json);

        QCOMPARE(history.daysWritten(), 1);
        QCOMPARE(history.day(DAY_).netWords, -2);
        QCOMPARE(history.day(DAY_).addedWords, 0);
        QCOMPARE(history.day(DAY_).activeMs, 0);
    }

    void anEmptyObjectReadsAsAnEmptyHistory()
    {
        auto history = WritingHistory::fromJson({});

        QVERIFY(history.lifetime().isEmpty());
        QCOMPARE(history.daysWritten(), 0);
    }

private:
    static inline const QDate DAY_{ 2026, 6, 10 };

    // Local time, as the history reads it
    static qint64 at_(QDate date, int hour, int minute)
    {
        return QDateTime(date, QTime(hour, minute)).toMSecsSinceEpoch();
    }

    static QJsonObject dayJson_(qint64 net, qint64 added, qint64 activeMs)
    {
        QJsonObject day{};
        day[u"netWords"_s] = net;
        day[u"addedWords"_s] = added;
        day[u"activeMs"_s] = activeMs;
        return day;
    }

    // The JSON another machine would have written, holding one day
    static QJsonObject
    machineJson_(QDate date, qint64 net, qint64 added, qint64 activeMs)
    {
        QJsonObject days{};
        days[date.toString(Qt::ISODate)] = dayJson_(net, added, activeMs);

        QJsonObject json{};
        json[u"days"_s] = days;
        return json;
    }
};

QTEST_GUILESS_MAIN(WritingHistoryTest)

#include "WritingHistoryTest.moc"
