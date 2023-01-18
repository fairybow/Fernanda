// resource.h, Fernanda

#pragma once

#include "path.h"

#include <algorithm>

#include <QDirIterator>
#include <QVector>

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

// resource.h, Fernanda
