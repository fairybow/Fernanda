/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QUrl>

#include "Tr.h"
#include "Version.h"

// Gets and reads the content of the Version.txt asset (created in pre-build
// with VSPreBuildGenVersionFullStringTxt.ps1) in the latest release. That file
// contains that version's VERSION_FULL_STRING from Version.h, which we compare
// against the one here.
//
// TODO: (maybe) An UpdateService might make sense later, if we want to check on
// startup, run periodic background checks, and/or broadcast a Bus signal so
// multiple elements (status bar badge, menu item indicator, etc.) can react
// independently. Also, might make since if we want to implement cancellation
// and retry logic (really, anything beyond user clicking a Check... option and
// waiting to display it)
// TODO: May later want to move to non-prerelease only or provide a check box to
// include or not include them or similar
// TODO: Explain update process in body (just install over the old version)
namespace Fernanda::UpdateDialog {

namespace Internal {

    constexpr auto GITHUB_API_URL_ =
        "https://api.github.com/repos/fairybow/Fernanda/releases";
    constexpr auto GITHUB_RELEASE_URL_ =
        "https://github.com/fairybow/Fernanda/releases";
    constexpr auto VERSION_ASSET_NAME_ = "Version.txt";

    inline void showDialog_(const QString& message)
    {
        QMessageBox box{};

        box.setWindowModality(Qt::ApplicationModal);
        box.setMinimumSize(400, 200);
        box.setWindowTitle(Tr::nxUpdateTitle());
        box.setText(message);
        box.setTextInteractionFlags(Qt::NoTextInteraction);

        auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
        auto visit = box.addButton(
            Tr::nxUpdateReleasesButton(),
            QMessageBox::ActionRole);

        QObject::connect(visit, &QPushButton::clicked, [] {
            QDesktopServices::openUrl(QUrl(GITHUB_RELEASE_URL_));
        });

        box.setDefaultButton(ok);
        box.setEscapeButton(ok);
        box.exec();
    }

    inline QUrl versionTxtUrl_(const QJsonArray& assets)
    {
        for (auto& asset : assets) {
            auto obj = asset.toObject();
            if (obj.value("name").toString() == VERSION_ASSET_NAME_)
                return obj.value("browser_download_url").toString();
        }

        return {};
    }

    inline void
    fetchVersionFile_(QNetworkAccessManager* manager, const QUrl& location)
    {
        auto request = QNetworkRequest(location);
        auto reply = manager->get(request);

        QObject::connect(reply, &QNetworkReply::finished, [reply] {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                showDialog_(
                    Tr::nxUpdateFailAssetFetch().arg(reply->errorString()));
                return;
            }

            auto latest = QString::fromUtf8(reply->readAll()).trimmed();

            if (latest == VERSION_FULL_STRING) {
                showDialog_(
                    Tr::nxUpdateLatestFormat().arg(VERSION_FULL_STRING));
            } else {
                showDialog_(
                    Tr::nxUpdateOutOfDateFormat()
                        .arg(VERSION_FULL_STRING)
                        .arg(latest));
            }
        });
    }

} // namespace Internal

inline void exec()
{
    static QNetworkAccessManager manager{};
    auto request = QNetworkRequest(QUrl(Internal::GITHUB_API_URL_));
    auto reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply] {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            Internal::showDialog_(
                Tr::nxUpdateFailReleaseFetchFormat().arg(reply->errorString()));
            return;
        }

        QJsonParseError parse_error{};
        auto document = QJsonDocument::fromJson(reply->readAll(), &parse_error);

        if (parse_error.error != QJsonParseError::NoError) {
            Internal::showDialog_(
                Tr::nxUpdateFailJsonParseFormat().arg(
                    parse_error.errorString()));
            return;
        }

        auto releases = document.array();

        if (releases.isEmpty()) {
            Internal::showDialog_(Tr::nxUpdateFailNoReleasesFound());
            return;
        }

        auto latest_release = releases[0].toObject();
        auto assets = latest_release.value("assets").toArray();
        auto version_txt_url = Internal::versionTxtUrl_(assets);

        if (version_txt_url.isEmpty()) {
            Internal::showDialog_(Tr::nxUpdateFailMissingAsset());
            return;
        }

        Internal::fetchVersionFile_(&manager, version_txt_url);
    });
}

} // namespace Fernanda::UpdateDialog
