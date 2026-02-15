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
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "Tr.h"
#include "Version.h"

// An UpdateService might make sense later, if we want to check on startup, run
// periodic background checks, and/or broadcast a Bus signal so multiple
// elements (status bar badge, menu item indicator, etc.) can react
// independently. Also, might make since if we want to implement cancellation
// and retry logic (really, anything beyond user clicking a Check... option and
// waiting to display it)
namespace Fernanda::UpdateDialog {

namespace Internal {

    constexpr auto GITHUB_API_URL_ =
        "https://api.github.com/repos/fairybow/Fernanda/releases";
    constexpr auto GITHUB_RELEASE_URL_ =
        "https://github.com/fairybow/Fernanda/releases";
    constexpr auto RELEASE_TAG_KEY_ = "tag_name";

    inline QString message_(QNetworkReply* reply)
    {
        // TODO: Handle any specific error cases in a switch?
        if (reply->error() != QNetworkReply::NoError) {
            return Tr::nxUpdateBodyErrorFormat().arg(reply->errorString());
        }

        auto data = reply->readAll();
        QJsonParseError parse_error{};
        auto document = QJsonDocument::fromJson(data, &parse_error);

        if (parse_error.error != QJsonParseError::NoError) {
            return Tr::nxUpdateBodyErrorFormat().arg(parse_error.errorString());
        }

        if (document.isNull()) {
            return Tr::nxUpdateBodyErrorFormat().arg(Tr::nxUpdateNullJsonArg());
        }

        auto list = document.toVariant().toList();

        if (list.isEmpty()) {
            return Tr::nxUpdateBodyErrorFormat().arg(
                Tr::nxUpdateEmptyListArg());
        }

        auto map = list[0].toMap();
        auto tag_value = map.value(RELEASE_TAG_KEY_).toString();

        if (tag_value.isEmpty()) {
            return Tr::nxUpdateBodyErrorFormat().arg(
                Tr::nxUpdateEmptyValueArg());
        }

        // TODO: Ensure we name the release appropriately!
        if (tag_value == VERSION_FULL_STRING) {
            return Tr::nxUpdateBodyLatestFormat().arg(VERSION_FULL_STRING);
        } else {
            return Tr::nxUpdateBodyOodFormat()
                .arg(VERSION_FULL_STRING)
                .arg(tag_value);
        }
    }

} // namespace Internal

inline void exec()
{
    static QNetworkAccessManager manager{};
    auto request = QNetworkRequest(QUrl(Internal::GITHUB_API_URL_));
    auto reply = manager.get(request);

    reply->connect(reply, &QNetworkReply::finished, [reply] {
        reply->deleteLater();

        QMessageBox box{};

        box.setWindowModality(Qt::ApplicationModal);
        box.setMinimumSize(
            400,
            200); // TODO: Constants for a universal min h/w for all dialogs?
        box.setWindowTitle(Tr::nxUpdateTitle());
        box.setText(Internal::message_(reply));
        box.setTextInteractionFlags(Qt::NoTextInteraction);

        auto ok = box.addButton(Tr::ok(), QMessageBox::AcceptRole);
        auto visit = box.addButton(
            Tr::nxUpdateReleasesButton(),
            QMessageBox::ActionRole);

        QObject::connect(visit, &QPushButton::clicked, [] {
            QDesktopServices::openUrl(QUrl(Internal::GITHUB_RELEASE_URL_));
        });

        box.setDefaultButton(ok);
        box.setEscapeButton(ok);

        // TODO: Move to open/show
        box.exec();
    });
}

} // namespace Fernanda::UpdateDialog
