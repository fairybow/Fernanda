#pragma once

#include "../common/HtmlString.hpp"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QVariantMap>

#include <utility>

class VersionChecker : public QObject
{
	Q_OBJECT

public:
	static inline const QString check(const QString& user, const QString& repo, const QString& version, QWidget* parent)
	{
		auto urls = makeGitHubUrls(user, repo);
		QString text = {
			HtmlString::heading("Version", 3)
			% HtmlString::bold("Current version:")
			/ version
		};
		auto map = latestVersion(urls.first, parent);
		if (!map.isEmpty()) {
			auto latest = map["tag_name"].toString();
			if (latest == QString(version))
				text += "You have the latest version.";
			else {
				QString message = {
					HtmlString::bold("New version:")
					/ latest
					% "You do not have the latest version."
					% HtmlString::bold("Download:")
					/ HtmlString::link(urls.second)
				};
				text %= message;
			}
		}
		else {
			QString message = {
				"Unable to verify version."
				% HtmlString::bold("Check:")
				/ HtmlString::link(urls.second)
			};
			text %= message;
		}
		return text;
	}

private:
	static inline std::pair<QUrl, QUrl> makeGitHubUrls(const QString& user, const QString& repo)
	{
		auto releases_api = QUrl("https://api.github.com/repos/" + user + "/" + repo + "/releases");
		auto releases = QUrl("https://github.com/" + user + "/" + repo + "/releases");
		return std::make_pair(releases_api, releases);
	}

	static inline QVariantMap latestVersion(const QUrl& url, QWidget* parent)
	{
		QEventLoop loop;
		auto manager = new QNetworkAccessManager(parent);
		auto request = QNetworkRequest(url);
		auto reply = manager->get(request);
		connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		QProgressDialog progress_dialog("Checking...", QString(), 0, 0, parent);
		progress_dialog.setCancelButton(nullptr);
		progress_dialog.setWindowModality(Qt::WindowModal);
		progress_dialog.show();
		loop.exec();
		progress_dialog.hide();
		QVariantMap map;
		if (reply->error() == QNetworkReply::NoError) {
			auto json_document = QJsonDocument::fromJson(reply->readAll());
			auto variant_list = json_document.toVariant().toList();
			map = variant_list[0].toMap();
		}
		reply->deleteLater();
		manager->deleteLater();
		return map;
	}
};
