#pragma once

#include "DocumentsCache.h"

int DocumentsCache::s_maxCost = 100;
bool DocumentsCache::s_initialized = false;

void DocumentsCache::setMaxCost(int maxCost)
{
	if (s_initialized) return;
	s_maxCost = maxCost;
}

DocumentsCache& DocumentsCache::instance()
{
	static DocumentsCache self;
	s_initialized = true;
	return self;
}

void DocumentsCache::add(TextRecord* document)
{
	StdLockMutex lock(m_mutex);
	if (!document) return;

	auto key = document->data().toUuid();
	if (!key.isNull())
		insert(key, document);
}

TextRecord* DocumentsCache::document(const QUuid& id)
{
	StdLockMutex lock(m_mutex);
	return object(id);
}
