// res.h, Fernanda

#pragma once

#include "path.h"

#include <algorithm>

#include <QDirIterator>
#include <QVector>

namespace Res
{
    namespace Fs = std::filesystem;

    struct DataPair {
        Fs::path path;
        QString label;
    };

    inline void collectResources(QDirIterator& iterator, QVector<DataPair>& listOfPathPairs)
    {
        while (iterator.hasNext())
        {
            iterator.next();
            auto q_path = iterator.filePath();
            auto label = Path::getName<QString>(q_path);
            listOfPathPairs << DataPair{ Path::toFs(q_path), Path::getName<QString>(q_path) };
        }
    }

    inline const QVector<DataPair> iterateResources(Fs::path path, QStringList extensions, Fs::path dataPath)
    {
        QVector<DataPair> dataAndLabels;
        for (auto& extension : extensions)
        {
            QDirIterator assets(Path::toQString(path), QStringList() << extension, QDir::Files, QDirIterator::Subdirectories);
            if (QDir(dataPath).exists())
            {
                QDirIterator user_assets(Path::toQString(dataPath), QStringList() << extension, QDir::Files, QDirIterator::Subdirectories);
                collectResources(user_assets, dataAndLabels);
            }
            collectResources(assets, dataAndLabels);
        }
        std::sort(dataAndLabels.begin(), dataAndLabels.end(), [](auto& lhs, auto& rhs)
            {
                return lhs.label < rhs.label;
            });
        return dataAndLabels;
    }
}

// res.h, Fernanda
