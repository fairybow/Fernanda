// dom.h, Fernanda

#pragma once

#include "io.h"

#include <QDomDocument>
#include <QDomElement>
#include <QUuid>
#include <QVector>

class Dom
{
    using StdFsPath = std::filesystem::path;

public:
    enum class ChildRenames {
        InPlace,
        Move
    };
    enum class Document {
        Current,
        Cuts,
        Initial
    };
    enum class Element {
        Element,
        Name,
        ParentDirKey,
        ParentDirPath,
        OrigPath,
        Path
    };
    enum class Filter {
        OrigToNullptr,
        RenameToOrig
    };
    enum class Finalize {
        No,
        Yes
    };
    enum class Write {
        Expanded,
        Rename
    };

    const QString attributeKey = QStringLiteral("key");
    const QString attributeRelativePath = QStringLiteral("relative_path");
    const QString attributeExpanded = QStringLiteral("expanded");
    const QString tagDir = QStringLiteral("directory");
    const QString tagFile = QStringLiteral("file");
    const QString tagRoot = QStringLiteral("root");

    void set(QString xmlDocument);
    const QString string(Document document = Document::Current);
    bool hasChanges();
    void move(QString pivotKey, QString fulcrumKey, Io::Move position);
    void rename(QString newName, QString key);
    void add(QString newName, Path::Type type, QString parentKey);
    QStringList cut(QString key);
    QVector<Io::ArchiveRename> cuts();
    QVector<Io::ArchiveRename> renames(Finalize finalize = Finalize::No);
    QVector<QDomElement> elements(QDomDocument document = QDomDocument());
    QVector<QDomElement> elements(QString attribute, QString value = nullptr);

    template<typename T>
    inline T element(QString key, Element property = Element::Element)
    {
        QDomElement found_element;
        auto root = self.documentElement();
        auto next_node = root.firstChildElement();
        while (!next_node.isNull())
        {
            auto elem = element_recursor(next_node, key);
            if (!elem.isNull())
                found_element = elem;
            next_node = next_node.nextSiblingElement();
        }
        T result{};
        switch (property) {
        case Element::Element:
            if constexpr (std::is_same<T, QDomElement>::value)
                result = found_element;
            break;
        case Element::Name:
            if constexpr (std::is_same<T, QString>::value)
                result = Path::getName<QString>(filterPath(found_element));
            break;
        case Element::ParentDirKey:
            if constexpr (std::is_same<T, QString>::value)
            {
                auto parent = found_element.parentNode();
                if (isDir(parent))
                    result = parent.toElement().attribute(attributeKey);
                else if (isRoot(parent))
                    result = nullptr;
                else
                    result = element<QString>(parent.toElement().attribute(attributeKey), Element::ParentDirKey);
            }
            break;
        case Element::ParentDirPath:
            if constexpr (std::is_same<T, StdFsPath>::value)
            {
                auto parent = found_element.parentNode();
                if (isDir(parent) || isRoot(parent))
                    result = filterPath(parent.toElement());
                else
                    result = element<StdFsPath>(parent.toElement().attribute(attributeKey), Element::ParentDirPath);
            }
            break;
        case Element::OrigPath:
            if constexpr (std::is_same<T, StdFsPath>::value)
                result = filterPath(found_element, Filter::OrigToNullptr);
            break;
        case Element::Path:
            if constexpr (std::is_same<T, StdFsPath>::value)
                result = filterPath(found_element);
            break;
        }
        return result;
    }

    template<typename T>
    inline void write(QString key, T value, Write property)
    {
        QDomElement target = element<QDomElement>(key);
        switch (property) {
        case Write::Expanded:
            if constexpr (std::is_same<T, bool>::value)
                target.setAttribute(attributeExpanded, QString(value ? "true" : "false"));
            break;
        case Write::Rename:
            if constexpr (std::is_same<T, QString>::value)
                target.setAttribute(attributeRename, value);
            break;
        }
    }

private:
    QDomDocument self;
    QDomDocument initialSelf;
    QDomDocument cutElements;

    const QString attributeRename = QStringLiteral("rename");

    QDomElement element_recursor(QDomElement node, QString key, QDomElement result = QDomElement());
    QVector<QDomElement> elements_recursor(QDomElement node, QVector<QDomElement> result = QVector<QDomElement>());
    QVector<QDomElement> elementsByAttribute_recursor(QDomElement node, QString attribute, QString value = nullptr, QVector<QDomElement> result = QVector<QDomElement>());
    void movePaths(StdFsPath& newPivotPath, StdFsPath& newPivotParentPath, QString pivotName, QString fulcrumKey);
    QStringList childKeys_recursor(QDomElement node, QStringList result = QStringList());
    QVector<Io::ArchiveRename> prepareChildRenames_recursor(QDomElement node, StdFsPath stemPathParent, ChildRenames renameType = ChildRenames::Move, QVector<Io::ArchiveRename> result = QVector<Io::ArchiveRename>());
    StdFsPath filterPath(QDomElement elem, Filter filter = Filter::RenameToOrig);

    template<typename T>
    inline bool isThis(T nodeOrElement, QString nodeOrTagName)
    {
        auto result = false;
        if constexpr (std::is_same<T, QDomElement>::value)
        {
            if (nodeOrElement.tagName() == nodeOrTagName)
                result = true;
        }
        if constexpr (std::is_same<T, QDomNode>::value)
        {
            if (nodeOrElement.isElement() && nodeOrElement.nodeName() == nodeOrTagName)
                result = true;
        }
        return result;
    }

    template<typename T>
    inline bool isDir(T nodeOrElement)
    {
        return isThis(nodeOrElement, tagDir);
    }

    template<typename T>
    inline bool isFile(T nodeOrElement)
    {
        return isThis(nodeOrElement, tagFile);
    }

    template<typename T>
    inline bool isRoot(T nodeOrElement)
    {
        return isThis(nodeOrElement, tagRoot);
    }
};

// dom.h, Fernanda
