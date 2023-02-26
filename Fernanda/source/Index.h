/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Index.h, Fernanda

#pragma once

#include <QModelIndex>
#include <QString>
#include <Qt>
#include <QVariant>

#include <type_traits>

namespace Index
{
	template<typename T>
	inline const T getData(QModelIndex index, int role = 0)
	{
		T result{};
		if constexpr (std::is_same<T, QString>::value)
		{
			index.isValid()
				? result = index.data(Qt::UserRole + role).toString()
				: result = nullptr;
		}
		if constexpr (std::is_same<T, bool>::value)
		{
			index.isValid()
				? result = index.data(Qt::UserRole + role).toBool()
				: result = false;
		}
		return result;
	}

	inline const QString type(QModelIndex index) { return getData<QString>(index); }
	inline const QString key(QModelIndex index) { return getData<QString>(index, 1); }
	inline const QString name(QModelIndex index) { return getData<QString>(index, 2); }
	inline bool isExpanded(QModelIndex index) { return getData<bool>(index, 3); }
	inline bool hasChildren(QModelIndex index) { return getData<bool>(index, 4); }
	inline bool isThis(QModelIndex index, QString indexType) { return (type(index) == indexType); }
	inline bool isDir(QModelIndex index) { return isThis(index, "directory"); }
	inline bool isFile(QModelIndex index) { return isThis(index, "file"); }
	inline bool isRoot(QModelIndex index) { return isThis(index, "root"); }
}

// Index.h, Fernanda
