/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <Coco/Debug.h>

#include "core/CoreConstants.h"
#include "core/Publication.h"
#include "core/UpdateCheck.h"
#include "core/Version.h"

namespace Suzuri {

using namespace Qt::StringLiterals;

// Whether a newer Suzuri has been released, and what the user has said about
// it. Owned by App, which saves its two settings to suzuri.json and hands it to
// every window that shows update state: the General settings page and the
// status bar's notice both read it here. Suzuri only tells the user; the
// release page is where they download and install.
//
// A check asks GitHub for the repository's releases and leaves the decision to
// UpdateCheck::availableUpdate. Prereleases count when this build is one, or
// when STABLE_BUILDS_SEE_PRERELEASES says a stable build should see them.
//
// Checks run when asked (check, from the General page) and, while
// checksAutomatically is on, UPDATE_FIRST_CHECK_DELAY_MS after start and every
// UPDATE_CHECK_INTERVAL_MS after that. An automatic check that fails is only
// logged, and the state goes back to what it was; one that was asked for
// reports why in failure(). One check runs at a time: asking while one is
// running makes it report, and starts nothing new.
//
// The notice is for one version. Dismissing it remembers that version, so the
// notice stays away across launches until a newer release is found
class Updater : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Unchecked,
        Checking,
        UpToDate,
        Available,
        Failed
    };

    Updater() { setup_(); }

    // A check still running is dropped without reporting: its finished signal
    // would otherwise reach this object as it's destroyed
    ~Updater() override
    {
        if (reply_) {
            reply_->disconnect(this);
        }
    }

    [[nodiscard]] State state() const noexcept { return state_; }

    // The newest release found by the last check that found one. Meaningful
    // in State::Available
    [[nodiscard]] const UpdateCheck::Release& release() const noexcept
    {
        return release_;
    }

    // Why the last check that was asked for failed. Meaningful in
    // State::Failed
    [[nodiscard]] const QString& failure() const noexcept { return failure_; }

    [[nodiscard]] bool checksAutomatically() const noexcept
    {
        return checksAutomatically_;
    }

    [[nodiscard]] const QString& dismissedVersion() const noexcept
    {
        return dismissedVersion_;
    }

    // Whether the status bar's notice shows: an update is available, and it
    // isn't the version whose notice was dismissed
    [[nodiscard]] bool hasNotice() const
    {
        return state_ == State::Available &&
               release_.version != dismissedVersion_;
    }

    // Begin with the saved settings. Called once, by App, after the session
    // is restored
    void start(bool checksAutomatically, const QString& dismissedVersion)
    {
        checksAutomatically_ = checksAutomatically;
        dismissedVersion_ = dismissedVersion;

        if (checksAutomatically_) {
            periodic_.start();

            QTimer::singleShot(UPDATE_FIRST_CHECK_DELAY_MS, this, [this] {
                if (checksAutomatically_) {
                    request_(false);
                }
            });
        }
    }

    // Turning automatic checks on checks now, then on the usual interval
    void setChecksAutomatically(bool on)
    {
        if (checksAutomatically_ == on) {
            return;
        }

        checksAutomatically_ = on;

        if (on) {
            periodic_.start();
            request_(false);
        } else {
            periodic_.stop();
        }

        emit settingsChanged();
    }

    // Hide the notice for the release found, in every window
    void dismiss()
    {
        if (!hasNotice()) {
            return;
        }

        dismissedVersion_ = release_.version;
        emit settingsChanged();
        emit stateChanged();
    }

    // A check the user asked for, which reports a failure
    void check() { request_(true); }

signals:
    // The state, the release found, or whether the notice shows changed
    void stateChanged();

    // checksAutomatically or dismissedVersion changed. App saves them
    void settingsChanged();

private:
    QNetworkAccessManager network_{};
    QTimer periodic_{};
    QNetworkReply* reply_ = nullptr;

    State state_ = State::Unchecked;
    UpdateCheck::Release release_{};
    QString failure_{};

    bool checksAutomatically_ = true;
    QString dismissedVersion_{};

    // The running check's: whether it reports a failure, and the state to go
    // back to when it doesn't
    bool reports_ = false;
    State stateBefore_ = State::Unchecked;

    void setup_()
    {
        periodic_.setInterval(UPDATE_CHECK_INTERVAL_MS);
        connect(&periodic_, &QTimer::timeout, this, [this] {
            request_(false);
        });
    }

    void request_(bool reports)
    {
        if (reply_) {
            reports_ = reports_ || reports;
            return;
        }

        reports_ = reports;
        stateBefore_ = state_;
        setState_(State::Checking);

        QNetworkRequest request(QUrl(QString::fromLatin1(UPDATE_RELEASES_URL)));
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
        request.setHeader(
            QNetworkRequest::UserAgentHeader,
            PUB_APP_NAME_QSTRING + u"/"_s + VERSION_FULL_QSTRING);
        request.setTransferTimeout(UPDATE_CHECK_TIMEOUT_MS);

        reply_ = network_.get(request);
        connect(reply_, &QNetworkReply::finished, this, &Updater::onFinished_);
    }

    void onFinished_()
    {
        auto* reply = reply_;
        reply_ = nullptr;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            fail_(reply->errorString());
            return;
        }

        QJsonParseError parse_error{};
        auto document = QJsonDocument::fromJson(reply->readAll(), &parse_error);

        if (parse_error.error != QJsonParseError::NoError ||
            !document.isArray()) {
            fail_(tr("GitHub's reply wasn't a list of releases."));
            return;
        }

        auto include_prereleases =
            UpdateCheck::isPrerelease(VERSION_FULL_QSTRING) ||
            STABLE_BUILDS_SEE_PRERELEASES;

        auto update = UpdateCheck::availableUpdate(
            document.array(),
            VERSION_FULL_QSTRING,
            include_prereleases);

        if (update) {
            DEBUG("Update available: {}", update->version);
            release_ = *update;
            setState_(State::Available);
        } else {
            DEBUG("No update available");
            setState_(State::UpToDate);
        }
    }

    void fail_(const QString& reason)
    {
        WARN("Update check failed: {}", reason);

        if (reports_) {
            failure_ = reason;
            setState_(State::Failed);
        } else {
            setState_(stateBefore_);
        }
    }

    void setState_(State state)
    {
        state_ = state;
        emit stateChanged();
    }
};

} // namespace Suzuri
