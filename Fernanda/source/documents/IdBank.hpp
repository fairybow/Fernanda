#pragma once

#include <QUuid>
#include <QVector>

#include <algorithm>
#include <filesystem>
#include <map>

struct IdBank
{
public:
	using StdFsPath = std::filesystem::path;
	using PathToIdMap = std::map<StdFsPath, QUuid>;

	QUuid fromPath(const StdFsPath& path)
	{
		QUuid id;
		auto it = m_pathsToIds.find(path);
		(it != m_pathsToIds.end())
			? id = it->second
			: id = recordNew(path);

		return id;
	}

	QUuid recordNew(StdFsPath path = StdFsPath())
	{
		auto id = QUuid::createUuid();
		if (!path.empty())
			m_pathsToIds[path] = id;
		m_bank << id;

		return id;
	}

	void associate(const StdFsPath& path, const QUuid& id)
	{
		m_pathsToIds[path] = id;
	}

	StdFsPath path(const QUuid& id)
	{
		StdFsPath path;
		auto it = std::find_if(
			m_pathsToIds.begin(),
			m_pathsToIds.end(), [&id](const auto& pair) {
				return pair.second == id;
			});
		if (it != m_pathsToIds.end())
			path = it->first;

		return path;
	}

	void remove(const QUuid& id)
	{
		auto extant_path = path(id);
		if (m_pathsToIds.contains(extant_path))
			m_pathsToIds.erase(extant_path);
		m_bank.removeOne(id);

		m_trash << id;
	}

	bool contains(const QUuid& id) const
	{
		return m_bank.contains(id);
	}

	bool isEmpty() const { return m_bank.isEmpty(); }
	bool hasPaths() const { return !m_pathsToIds.empty(); }
	bool hasTrash() const { return !m_trash.empty(); }

	PathToIdMap paths() const { return m_pathsToIds; }
	QVector<QUuid> bank() const { return m_bank; }
	QVector<QUuid> trash() const { return m_trash; }

private:
	PathToIdMap m_pathsToIds;
	QVector<QUuid> m_bank;
	QVector<QUuid> m_trash;
};
