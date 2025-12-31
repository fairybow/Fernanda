/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QString>

#include "Coco/Bool.h"

#include "Debug.h"
#include "Fnx.h"

namespace Fernanda {

class FnxModelCache
{
public:
    COCO_BOOL(OnError);

    FnxModelCache() = default;
    virtual ~FnxModelCache() { TRACER; }

    void clear(OnError onError = OnError::No)
    {
        nextId_ = 1;
        keyToId_.clear();
        idToElement_.clear();
        structure_.clear();
        if (onError) CRITICAL("FnxModelCache cleared due to error!");
    }

    void cache(const QDomElement& element) { (void)idOf(element); }

    // Returns element for ID, or null element if not found. Does NOT validate
    // element is still in DOM!
    QDomElement elementAt(quintptr id) const
    {
        return idToElement_.value(id, {});
    }

    bool containsKey(const QString& key) const
    {
        return keyToId_.contains(key);
    }

    // TODO: Unused
    bool containsId(quintptr id) const { return idToElement_.contains(id); }

    // Returns stable key for element: UUID for user content, tag name for
    // structural elements (fnx, notebook, trash). Returns empty string for
    // null elements
    static QString keyOf(const QDomElement& element)
    {
        if (element.isNull()) return {};

        // Structural elements use tag name as key (no UUID)
        auto tag = element.tagName();
        if (tag == Fnx::Xml::DOCUMENT_ELEMENT_TAG
            || tag == Fnx::Xml::NOTEBOOK_TAG || tag == Fnx::Xml::TRASH_TAG) {
            return tag;
        }

        // User elements require UUID
        auto uuid = Fnx::Xml::uuid(element);
        if (uuid.isEmpty()) WARN("FnxModelCache: Missing UUID!");
        return uuid;
    }

    // Gets existing ID OR creates new one
    quintptr idOf(const QDomElement& element)
    {
        if (element.isNull()) return 0;

        auto key = keyOf(element);

        auto it = keyToId_.find(key);
        if (it != keyToId_.end()) return it.value();

        // Create new ID
        auto id = nextId_++;
        keyToId_.insert(key, id);
        idToElement_.insert(id, element);
        return id;
    }

    // Get ID for key without creating (returns 0 if not found)
    quintptr idForKey(const QString& key) const
    {
        return keyToId_.value(key, 0);
    }

    // Returns row (index within parent), or -1 if unknown. Populates parent's
    // children cache if needed
    int rowOf(const QDomElement& element)
    {
        if (element.isNull()) return -1;

        auto key = keyOf(element);

        // Fast path: row already cached
        auto it = structure_.find(key);
        if (it != structure_.end() && it->row >= 0) return it->row;

        // Slow path: populate parent's children (which sets our row)
        auto parent = element.parentNode().toElement();
        if (parent.isNull()) return -1;

        ensurePopulated_(parent, keyOf(parent));

        // Re-check our entry (population should have set it)
        it = structure_.find(key);
        return (it != structure_.end()) ? it->row : -1;
    }

    // Returns child count, populating cache if needed
    int childCount(const QDomElement& parent)
    {
        if (parent.isNull()) return 0;

        auto key = keyOf(parent);
        auto& entry = ensurePopulated_(parent, key);
        return entry.children.size();
    }

    // Returns nth child, or null element if out of bounds
    QDomElement childAt(const QDomElement& parent, int index)
    {
        if (parent.isNull() || index < 0) return {};

        auto key = keyOf(parent);
        auto& entry = ensurePopulated_(parent, key);

        if (index >= entry.children.size()) return {};
        return entry.children[index];
    }

    // Recursively removes all descendants AND element from cache. Call before
    // removing from DOM, so children can be traversed!
    void purgeSubtree(const QDomElement& root)
    {
        if (root.isNull()) return;
        purgeSubtreeRecursive_(root);
    }

    // Marks a parent's children list as unpopulated without clearing row data.
    // Use after bulk DOM removal where purgeSubtree was called on children
    void invalidateChildren(const QDomElement& parent)
    {
        auto key = keyOf(parent);
        auto it = structure_.find(key);

        if (it != structure_.end()) {
            it->children.clear();
            it->populated = false;
        }
    }

    // Record that child was appended/inserted to parent. Position -1 means
    // appended to end
    void recordInsertion(
        const QDomElement& parent,
        const QDomElement& child,
        int position = -1)
    {
        auto parent_key = keyOf(parent);
        auto it = structure_.find(parent_key);

        // If parent's children aren't cached, nothing to update
        if (it == structure_.end() || !it->populated) return;

        auto& children = it->children;
        int pos = (position < 0 || position > children.size()) ? children.size()
                                                               : position;

        children.insert(pos, child);
        auto child_key = keyOf(child);
        structure_[child_key].row = pos;

        // Update rows of following siblings
        updateRowsFrom_(parent_key, pos + 1);
    }

    // Record that child was removed from parent
    void recordRemoval(const QDomElement& parent, const QDomElement& child)
    {
        auto parent_key = keyOf(parent);
        auto it = structure_.find(parent_key);

        // If parent's children aren't cached, nothing to update
        if (it == structure_.end() || !it->populated) return;

        auto& children = it->children;
        auto index = children.indexOf(child);

        if (index < 0) {
            WARN("FnxModelCache::recordRemoval: child not in parent's cache!");
            return;
        }

        // Remove from children list
        children.removeAt(index);

        // Invalidate removed child's row
        auto child_key = keyOf(child);
        auto child_it = structure_.find(child_key);
        if (child_it != structure_.end()) { child_it->row = -1; }

        // Update rows of following siblings
        updateRowsFrom_(parent_key, index);
    }

    // Record element move. Call AFTER DOM modification.
    // newPosition is the position in newParent AFTER the element was inserted!
    void recordMove(
        const QDomElement& oldParent,
        const QDomElement& newParent,
        const QDomElement& element,
        int newPosition)
    {
        // Handle as removal + insertion
        // Note: For same-parent moves, order matters but the indices are
        // already adjusted by caller (model computes dest_row_for_dom)

        auto old_parent_key = keyOf(oldParent);
        auto new_parent_key = keyOf(newParent);
        auto element_key = keyOf(element);

        // Update old parent
        auto old_it = structure_.find(old_parent_key);
        if (old_it != structure_.end() && old_it->populated) {
            auto old_index = old_it->children.indexOf(element);
            if (old_index >= 0) {
                old_it->children.removeAt(old_index);
                updateRowsFrom_(old_parent_key, old_index);
            }
        }

        // Update new parent
        auto new_it = structure_.find(new_parent_key);
        if (new_it != structure_.end() && new_it->populated) {
            auto& children = new_it->children;
            int pos = (newPosition < 0 || newPosition > children.size())
                          ? children.size()
                          : newPosition;

            children.insert(pos, element);
            structure_[element_key].row = pos;
            updateRowsFrom_(new_parent_key, pos + 1);
        } else {
            // New parent not cached. Just update element's row to unknown
            structure_[element_key].row = -1;
        }
    }

private:
    // ID allocation: 0 = invalid/root element
    // IDs start at 1 and increment
    // IDs are stable across DOM modifications when using UUID-based tracking
    mutable quintptr nextId_ = 1;
    mutable QHash<QString, quintptr> keyToId_{};
    mutable QHash<quintptr, QDomElement> idToElement_{};

    struct Entry
    {
        QList<QDomElement> children{};
        int row = -1; // This element's position in its parent
        bool populated = false; // Whether children list is valid
    };

    QHash<QString, Entry> structure_{};

    // Non-copyable (contains mutable state tied to specific DOM)
    FnxModelCache(const FnxModelCache&) = delete;
    FnxModelCache& operator=(const FnxModelCache&) = delete;

    Entry& ensureEntry_(const QString& key)
    {
        auto it = structure_.find(key);
        if (it == structure_.end()) it = structure_.insert(key, {});
        return it.value();
    }

    Entry& ensurePopulated_(const QDomElement& parent, const QString& key)
    {
        auto it = structure_.find(key);

        if (it != structure_.end() && it->populated) return it.value();

        // Need to populate
        if (it == structure_.end()) { it = structure_.insert(key, {}); }

        auto& entry = it.value();
        entry.children.clear();
        entry.children.reserve(16); // Reasonable default

        auto child = parent.firstChildElement();
        auto row = 0;

        while (!child.isNull()) {
            entry.children << child;

            // Set child's row in its own entry
            auto child_key = keyOf(child);
            ensureEntry_(child_key).row = row;
            child = child.nextSiblingElement();

            ++row;
        }

        entry.populated = true;
        return entry;
    }

    void updateRowsFrom_(const QString& parentKey, int startIndex)
    {
        auto it = structure_.find(parentKey);
        if (it == structure_.end() || !it->populated) return;

        const auto& children = it->children;
        for (auto i = startIndex; i < children.size(); ++i) {
            auto child_key = keyOf(children[i]);
            auto child_it = structure_.find(child_key);
            if (child_it != structure_.end()) child_it->row = i;
        }
    }

    void purgeSubtreeRecursive_(const QDomElement& element)
    {
        // First recurse into children (while we can still access them)
        auto child = element.firstChildElement();
        while (!child.isNull()) {
            purgeSubtreeRecursive_(child);
            child = child.nextSiblingElement();
        }

        // Then remove this element from caches
        auto key = keyOf(element);

        // Remove from ID cache
        auto id_it = keyToId_.find(key);
        if (id_it != keyToId_.end()) {
            idToElement_.remove(id_it.value());
            keyToId_.erase(id_it);
        }

        // Remove from structure cache
        structure_.remove(key);
    }
};

} // namespace Fernanda
