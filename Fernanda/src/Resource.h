/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Resource.h, Fernanda

#pragma once

#include "Path.h"

#include <QDirIterator>
#include <QVector>

#include <algorithm>

namespace Resource
{
    namespace StdFs = std::filesystem;

    struct DataPair {
        StdFs::path path;
        QString label;
    };

    inline void collect(QDirIterator& iterator, QVector<DataPair>& listOfPathPairs)
    {
        while (iterator.hasNext())
        {
            iterator.next();
            auto q_path = iterator.filePath();
            auto label = Path::getName<QString>(q_path);
            listOfPathPairs << DataPair{ Path::toStdFs(q_path), Path::getName<QString>(q_path) };
        }
    }

    inline const QVector<DataPair> iterate(StdFs::path path, QStringList extensions, StdFs::path dataPath)
    {
        QVector<DataPair> dataAndLabels;
        for (auto& extension : extensions)
        {
            QDirIterator assets(Path::toQString(path), QStringList() << extension, QDir::Files, QDirIterator::Subdirectories);
            if (QDir(dataPath).exists())
            {
                QDirIterator user_assets(Path::toQString(dataPath), QStringList() << extension, QDir::Files, QDirIterator::Subdirectories);
                collect(user_assets, dataAndLabels);
            }
            collect(assets, dataAndLabels);
        }
        std::sort(dataAndLabels.begin(), dataAndLabels.end(), [](auto& lhs, auto& rhs)
            {
                return lhs.label < rhs.label;
            });
        return dataAndLabels;
    }
}

// Resource.h, Fernanda
