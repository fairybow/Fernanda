#pragma once

#include "../common/Path.hpp"

#include <QActionGroup>
#include <QDirIterator>
#include <QString>
#include <QVariant>
#include <QVector>

#include <algorithm>
#include <filesystem>
#include <functional>

class ActionGroup : public QActionGroup
{
public:
	using Slot = std::function<void()>;
	using StdFsPath = std::filesystem::path;
	using StdFsPathList = QVector<StdFsPath>;

	using QActionGroup::QActionGroup;

	struct Bespoke {
		QVariant data;
		QString label = QString();
	};

	using BespokeList = QVector<Bespoke>;

	static Bespoke bespoke(QVariant data, QString label = QString());
	static ActionGroup* fromQrc(const QStringList& qrcPaths, QStringList extensions,
		StdFsPathList systemPaths = {}, QObject* parent = nullptr, Slot slot = nullptr);
	static ActionGroup* fromQrc(const QString& qrcPath, QString extension,
		StdFsPath systemPath = StdFsPath(), QObject* parent = nullptr, Slot slot = nullptr);
	static ActionGroup* fromQrc(const QStringList& qrcPaths, QStringList extensions,
		StdFsPath systemPath = StdFsPath(), QObject* parent = nullptr, Slot slot = nullptr);
	static ActionGroup* fromBespoke(BespokeList entries, QObject* parent = nullptr,
		Slot slot = nullptr);

private:
	static void addActionToGroup(ActionGroup* actionGroup, const QString& label,
		const QVariant& data, QObject* parent, Slot slot);
	static void checkExtensions(QStringList& extensions);
	static void alphabetize(ActionGroup* actionGroup);
};
