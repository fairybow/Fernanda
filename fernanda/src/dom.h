// dom.h, fernanda

#pragma once

#include "io.h"

#include <type_traits>

#include <QDomDocument>
#include <QDomElement>
#include <QUuid>
#include <QVector>

class Dom
{

public:
    enum class ChildRenames {
        InPlace,
        Move
    };
    enum class Doc {
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

    void set(QString xmlDoc);
    const QString string(Doc doc = Doc::Current);
    bool hasChanges();
    void move(QString pivotKey, QString fulcrumKey, Io::Move pos);
    void rename(QString newName, QString key);
    void add(QString newName, Path::Type type, QString parentKey);
    void cut(QString key);
    QVector<Io::ArcRename> cuts();
    QVector<Io::ArcRename> renames(Finalize finalize = Finalize::No);

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
                result = Path::getName(filterPath(found_element));
            break;
        case Element::ParentDirKey:
            if constexpr (std::is_same<T, QString>::value)
            {
                auto parent = found_element.parentNode();
                if (isDir(parent))
                    result = parent.toElement().attribute("key");
                else if (isRoot(parent))
                    result = nullptr;
                else
                    result = element<QString>(parent.toElement().attribute("key"), Element::ParentDirKey);
            }
            break;
        case Element::ParentDirPath:
            if constexpr (std::is_same<T, QString>::value)
            {
                auto parent = found_element.parentNode();
                if (isDir(parent) || isRoot(parent))
                    result = filterPath(parent.toElement());
                else
                    result = element<QString>(parent.toElement().attribute("key"), Element::ParentDirPath);
            }
            break;
        case Element::OrigPath:
            if constexpr (std::is_same<T, QString>::value)
                result = filterPath(found_element, Filter::OrigToNullptr);
            break;
        case Element::Path:
            if constexpr (std::is_same<T, QString>::value)
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
                target.setAttribute("expanded", QString(value ? "true" : "false"));
            break;
        case Write::Rename:
            if constexpr (std::is_same<T, QString>::value)
                target.setAttribute("rename", value);
            break;
        }
    }

private:
    QDomDocument self;
    QDomDocument initialSelf;
    QDomDocument cutElements;

    QDomElement element_recursor(QDomElement node, QString key, QDomElement result = QDomElement());
    QVector<QDomElement> elements(QDomDocument doc);
    QVector<QDomElement> elements_recursor(QDomElement node, QVector<QDomElement> result = QVector<QDomElement>());
    QVector<QDomElement> elementsByAttribute(QString attribute, QString value = nullptr);
    QVector<QDomElement> elementsByAttribute_recursor(QDomElement node, QString attribute, QString value = nullptr, QVector<QDomElement> result = QVector<QDomElement>());
    void movePaths(QString& newPivotPath, QString& newPivotParentPath, QString pivotName, QString fulcrumKey);
    QVector<Io::ArcRename> prepareChildRenames_recursor(QDomElement node, QString stemPathParent, ChildRenames renameType = ChildRenames::Move, QVector<Io::ArcRename> result = QVector<Io::ArcRename>());
    QString filterPath(QDomElement elem, Filter filter = Filter::RenameToOrig);

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
        return isThis(nodeOrElement, "dir");
    }

    template<typename T>
    inline bool isFile(T nodeOrElement)
    {
        return isThis(nodeOrElement, "file");
    }

    template<typename T>
    inline bool isRoot(T nodeOrElement)
    {
        return isThis(nodeOrElement, "root");
    }
};

// dom.h, fernanda