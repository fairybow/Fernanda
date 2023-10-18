#pragma once

#include "../common/TextRecord.hpp"

#include <QCache>
#include <QUuid>
#include <QVariant>

#include <mutex>

class DocumentsCache : public QCache<QUuid, TextRecord>
{
public:
	static void setMaxCost(int maxCost);
	static DocumentsCache& instance();
	void add(TextRecord* document);
	TextRecord* document(const QUuid& id);

	DocumentsCache(const DocumentsCache&) = delete;
	DocumentsCache(DocumentsCache&&) = delete;
	DocumentsCache& operator=(const DocumentsCache&) = delete;
	DocumentsCache& operator=(DocumentsCache&&) = delete;

private:
	using StdLockMutex = std::lock_guard<std::mutex>;

	DocumentsCache() : QCache(s_maxCost) {}

	static int s_maxCost;
	static bool s_initialized;
	std::mutex m_mutex;
};

using DocsCache = DocumentsCache;
