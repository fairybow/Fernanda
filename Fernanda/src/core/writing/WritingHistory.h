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

#pragma once

#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QString>
#include <QtMinMax>
#include <QtTypes>

#include "core/CoreConstants.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// A vault's writing history: for each day, the net change in words, the words
// added, and the time spent writing. A plain value, with no timer and no clock
// of its own. recordEdit takes the time as nowMs (Coco::Time::epoch()), so a
// test can hand it any time it likes.
//
// Every edit is recorded, including one that changes no word count, since
// each counts as activity. An edit's word delta adds to the day's net words,
// and a gain also adds to its added words, so deleting never takes away from
// what was added. Undoing typed words is an edit like any other: it lowers the
// net and leaves the added words as they were.
//
// Active time is the sum of the gaps between edits that are no longer than
// WRITING_IDLE_LIMIT_MS. A longer gap is a pause and adds nothing, so the
// first edit after one adds nothing either. A gap counts toward the day of the
// edit that closes it, so a gap across midnight goes to the new day.
//
// A day is the local calendar date of the edit.
//
// The history is kept per machine, so two machines syncing one vault never
// write the same file. This machine's days are its own, read from and written
// to JSON; other machines' days are only read, and added in. Every query
// reports the sum of both
class WritingHistory
{
public:
    struct Day
    {
        qint64 netWords = 0; // negative on a day that cut more than it wrote
        qint64 addedWords = 0;
        qint64 activeMs = 0;

        [[nodiscard]] bool isEmpty() const noexcept
        {
            return netWords == 0 && addedWords == 0 && activeMs == 0;
        }

        Day& operator+=(const Day& other) noexcept
        {
            netWords += other.netWords;
            addedWords += other.addedWords;
            activeMs += other.activeMs;
            return *this;
        }
    };

    // Read this machine's days from its JSON (see toJson). A missing key, a
    // key that isn't a date, or a value that isn't an object is skipped, so a
    // damaged file loses only what's damaged
    [[nodiscard]] static WritingHistory fromJson(const QJsonObject& json)
    {
        WritingHistory history{};
        history.ownDays_ = daysFromJson_(json);
        return history;
    }

    // This machine's days only. Other machines' days stay in their own files
    [[nodiscard]] QJsonObject toJson() const
    {
        QJsonObject days{};

        for (auto it = ownDays_.cbegin(); it != ownDays_.cend(); ++it) {
            QJsonObject day{};
            day[NET_WORDS_KEY_] = it.value().netWords;
            day[ADDED_WORDS_KEY_] = it.value().addedWords;
            day[ACTIVE_MS_KEY_] = it.value().activeMs;
            days[it.key().toString(Qt::ISODate)] = day;
        }

        QJsonObject json{};
        json[DAYS_KEY_] = days;
        return json;
    }

    // Replace what was read from other machines with these, each the JSON one
    // of them wrote (toJson)
    void setOtherMachines(const QList<QJsonObject>& jsons)
    {
        otherDays_.clear();

        for (const auto& json : jsons) {
            addDays_(otherDays_, daysFromJson_(json));
        }
    }

    // Record an edit made at nowMs that changed the word count by wordDelta
    // (zero for an edit that changed none). Returns whether this machine's
    // days changed, and so whether there is anything new to save
    bool recordEdit(qint64 nowMs, qint64 wordDelta)
    {
        Day change{};
        change.netWords = wordDelta;
        change.addedWords = qMax(wordDelta, 0);

        if (hasLastEdit_) {
            // A clock set back by hand gives a negative gap, which adds nothing
            auto gap = nowMs - lastEditMs_;

            if (gap > 0 && gap <= WRITING_IDLE_LIMIT_MS) {
                change.activeMs = gap;
            }
        }

        hasLastEdit_ = true;
        lastEditMs_ = nowMs;

        if (change.isEmpty()) {
            return false;
        }

        ownDays_[dayOf_(nowMs)] += change;
        return true;
    }

    // Everything recorded on this date, by every machine
    [[nodiscard]] Day day(QDate date) const
    {
        auto result = ownDays_.value(date);
        result += otherDays_.value(date);
        return result;
    }

    // Everything recorded, by every machine
    [[nodiscard]] Day lifetime() const
    {
        Day result{};

        for (const auto& day : ownDays_) {
            result += day;
        }

        for (const auto& day : otherDays_) {
            result += day;
        }

        return result;
    }

    // The dates with anything recorded, by any machine, each counted once
    [[nodiscard]] int daysWritten() const
    {
        auto count = static_cast<int>(ownDays_.size());

        for (auto it = otherDays_.cbegin(); it != otherDays_.cend(); ++it) {
            if (!ownDays_.contains(it.key())) {
                ++count;
            }
        }

        return count;
    }

private:
    static constexpr auto DAYS_KEY_ = "days"_L1;
    static constexpr auto NET_WORDS_KEY_ = "netWords"_L1;
    static constexpr auto ADDED_WORDS_KEY_ = "addedWords"_L1;
    static constexpr auto ACTIVE_MS_KEY_ = "activeMs"_L1;

    QMap<QDate, Day> ownDays_{};
    QMap<QDate, Day> otherDays_{};

    // The last edit is for measuring gaps in this run only, and isn't saved:
    // the first edit after launch begins a new stretch of writing
    bool hasLastEdit_ = false;
    qint64 lastEditMs_ = 0;

    [[nodiscard]] static QDate dayOf_(qint64 epochMs)
    {
        return QDateTime::fromMSecsSinceEpoch(epochMs).date();
    }

    static void addDays_(QMap<QDate, Day>& into, const QMap<QDate, Day>& days)
    {
        for (auto it = days.cbegin(); it != days.cend(); ++it) {
            into[it.key()] += it.value();
        }
    }

    // Added words and active time can't be negative, whatever a hand-edited
    // file says. A day that reads as empty is left out
    [[nodiscard]] static QMap<QDate, Day> daysFromJson_(const QJsonObject& json)
    {
        QMap<QDate, Day> days{};
        auto days_json = json.value(DAYS_KEY_).toObject();

        for (auto it = days_json.constBegin(); it != days_json.constEnd();
             ++it) {
            auto date = QDate::fromString(it.key(), Qt::ISODate);

            if (!date.isValid() || !it.value().isObject()) {
                continue;
            }

            auto day_json = it.value().toObject();
            Day day{};
            day.netWords = day_json.value(NET_WORDS_KEY_).toInteger();
            day.addedWords =
                qMax(day_json.value(ADDED_WORDS_KEY_).toInteger(), 0);
            day.activeMs = qMax(day_json.value(ACTIVE_MS_KEY_).toInteger(), 0);

            if (!day.isEmpty()) {
                days[date] = day;
            }
        }

        return days;
    }
};

} // namespace Fernanda
