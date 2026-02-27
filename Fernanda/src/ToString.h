/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <string>

#include <QDomAttr>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QMapIterator>
#include <QMetaObject>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include "Coco/Concepts.h"
#include "Coco/Utility.h"

#define TO_STD_(Type, ArgName)                                                 \
    inline std::string toString(const Type& ArgName)                           \
    {                                                                          \
        return toQString(ArgName).toStdString();                               \
    }

namespace Fernanda {

// Ptr cannot be nullptr
template <typename T> inline QString ptrAddress(const T* ptr)
{
    return QString::asprintf("%p", ptr);
}

// Ptr cannot be nullptr
template <Coco::Concepts::QObjectDerived T>
inline QString qObjectPtrAddress(const T* ptr)
{
    return QString("%0(%1)")
        .arg(ptr->metaObject()->className())
        .arg(ptrAddress<T>(ptr));
}

// Ptr can be nullptr
template <Coco::Concepts::QObjectDerived T>
inline QString toQString(const T* ptr)
{
    return ptr ? qObjectPtrAddress<T>(ptr) : "nullptr";
}

inline QString toQString(const QModelIndex& index)
{
    if (!index.isValid()) return "QModelIndex(Invalid)";

    return QString("QModelIndex(row:%1, col:%2, %3)")
        .arg(index.row())
        .arg(index.column())
        .arg(ptrAddress(index.internalPointer()));
}

inline QString toQString(const QPoint& point)
{
    return QString("QPoint(x:%0, y:%1)").arg(point.x()).arg(point.y());
}

inline QString toQString(const QStringList& stringList)
{
    return stringList.join(", ");
}

inline QString toQString(const QDomElement& element)
{
    if (element.isNull()) return "QDomElement(Null)";

    auto tag = element.tagName();
    auto attrs = element.attributes();

    if (attrs.isEmpty()) { return QString("QDomElement(\"<%1>\")").arg(tag); }

    QStringList attr_list{};
    for (auto i = 0; i < attrs.count(); ++i) {
        auto attr = attrs.item(i).toAttr();
        attr_list << QString("%1='%2'").arg(attr.name()).arg(attr.value());
    }

    return QString("QDomElement(<%1 %2>)").arg(tag).arg(attr_list.join(" "));
}

// Like QVariant::toString, we won't generally print "QVariant(value)"
inline QString toQString(const QVariant& variant)
{
    if (!variant.isValid()) return "QVariant(Invalid)";
    if (variant.isNull()) return "QVariant(Null)";

    if (variant.canConvert<QDomElement>())
        return toQString(variant.value<QDomElement>());

    if (variant.canConvert<QObject*>())
        return toQString(variant.value<QObject*>());

    switch (variant.typeId()) {
        // TODO: Would need source file to do QVariantMap here
        /*case QMetaType::QVariantMap:
            return toQString(variant.value<QVariantMap>());*/

    case QMetaType::QModelIndex:
        return toQString(variant.value<QModelIndex>());

    case QMetaType::QPoint:
        return toQString(variant.value<QPoint>());

        // This doesn't work for subclasses apparently:
        /*case QMetaType::QObjectStar:
            return toQString(variant.value<QObject*>());*/

    case QMetaType::QStringList:
        return variant.value<QStringList>().join(", ");

    default:
        auto text = variant.toString();
        return text.isEmpty() ? "QVariant(Non-printable)" : text;
    }
}

inline QString toQString(const QVariantMap& variantMap)
{
    if (variantMap.isEmpty()) return "QVariantMap()";
    constexpr auto inner_format = "{ \"%0\", %1 }";
    constexpr auto outer_format = "QVariantMap(%0)";
    QStringList list{};
    QMapIterator<QString, QVariant> it(variantMap);

    while (it.hasNext()) {
        it.next();
        list << QString(inner_format).arg(it.key()).arg(toQString(it.value()));
    }

    return QString(outer_format).arg(list.join(", "));
}

// Std

// Ptr can be nullptr
template <Coco::Concepts::QObjectDerived T>
inline std::string toString(const T* ptr)
{
    return toQString<T>(ptr).toStdString();
}

TO_STD_(QModelIndex, index);
TO_STD_(QPoint, point);
TO_STD_(QStringList, stringList);
TO_STD_(QDomElement, element);
TO_STD_(QVariant, variant);
TO_STD_(QVariantMap, variantMap);

} // namespace Fernanda

#undef TO_STD_

// QVariant Output Test:

/*#include <QByteArray>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QChar>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLine>
#include <QLineF>
#include <QLocale>
#include <QModelIndex>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QVariantHash>
#include <QVariantList>
#include <QVariantMap>

void testVariantToString(const QString& typeName, const QVariant& variant)
{
    QString result = variant.toString();
    qDebug() << qUtf8Printable(QString("%1:").arg(typeName, -25))
             << qUtf8Printable(result.isEmpty() ? "<empty>" : result);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== String Types ===";
    testVariantToString("QString", QVariant(QString("Hello")));
    testVariantToString("QByteArray", QVariant(QByteArray("Byte Array")));
    testVariantToString("QChar", QVariant(QChar('A')));

    qDebug() << "\n=== Numeric Types ===";
    testVariantToString("Bool", QVariant(true));
    testVariantToString("Int", QVariant(55));
    testVariantToString("UInt", QVariant(55u));
    testVariantToString("Long", qVar(55L));
    testVariantToString("ULong", qVar(55UL));
    testVariantToString("LongLong", QVariant(55LL));
    testVariantToString("ULongLong", QVariant(55ULL));
    testVariantToString("Short", qVar(static_cast<short>(55)));
    testVariantToString(
        "UShort",
        qVar(static_cast<unsigned short>(55)));
    testVariantToString("Char", qVar(static_cast<char>('X')));
    testVariantToString(
        "SChar",
        qVar(static_cast<signed char>('Y')));
    testVariantToString(
        "UChar",
        qVar(static_cast<unsigned char>('Z')));
    testVariantToString("Float", QVariant(3.14f));
    testVariantToString("Double", QVariant(3.14159));

    qDebug() << "\n=== Date/Time Types ===";
    testVariantToString("QDate", QVariant(QDate(2025, 11, 12)));
    testVariantToString("QTime", QVariant(QTime(14, 30, 45)));
    testVariantToString("QDateTime", QVariant(QDateTime::currentDateTime()));

    qDebug() << "\n=== Simple Qt Types ===";
    testVariantToString("QUrl", QVariant(QUrl("https://example.com")));
    testVariantToString("QUuid", QVariant(QUuid::createUuid()));
    testVariantToString(
        "QLocale",
        QVariant(QLocale(QLocale::English, QLocale::UnitedStates)));

    qDebug() << "\n=== Geometric Types ===";
    testVariantToString("QPoint", QVariant(QPoint(10, 20)));
    testVariantToString("QPointF", QVariant(QPointF(10.5, 20.5)));
    testVariantToString("QSize", QVariant(QSize(100, 200)));
    testVariantToString("QSizeF", QVariant(QSizeF(100.5, 200.5)));
    testVariantToString("QRect", QVariant(QRect(10, 20, 100, 200)));
    testVariantToString("QRectF", QVariant(QRectF(10.5, 20.5, 100.5, 200.5)));
    testVariantToString("QLine", QVariant(QLine(0, 0, 100, 100)));
    testVariantToString("QLineF", QVariant(QLineF(0.0, 0.0, 100.5, 100.5)));

    qDebug() << "\n=== Container Types ===";
    testVariantToString(
        "QStringList",
        QVariant(QStringList{ "one", "two", "three" }));
    testVariantToString(
        "QVariantList",
        QVariant(QVariantList{ 1, "two", 3.0 }));
    testVariantToString(
        "QVariantMap",
        QVariant(QVariantMap{ { "key1", 1 }, { "key2", "value" } }));
    testVariantToString(
        "QVariantHash",
        QVariant(QVariantHash{ { "key1", 1 }, { "key2", "value" } }));

    qDebug() << "\n=== Model Types ===";
    testVariantToString("QModelIndex", qVar(QModelIndex()));

    qDebug() << "\n=== JSON Types ===";
    testVariantToString(
        "QJsonValue",
        qVar(QJsonValue("json string")));
    testVariantToString(
        "QJsonObject",
        qVar(QJsonObject{ { "key", "value" } }));
    testVariantToString(
        "QJsonArray",
        qVar(QJsonArray{ "one", "two", "three" }));

    QJsonDocument jsonDoc(QJsonObject{ { "key", "value" } });
    testVariantToString("QJsonDocument", qVar(jsonDoc));

    qDebug() << "\n=== CBOR Types ===";
    testVariantToString(
        "QCborValue",
        qVar(QCborValue("cbor string")));
    testVariantToString(
        "QCborArray",
        qVar(QCborArray{ "one", "two" }));
    testVariantToString(
        "QCborMap",
        qVar(QCborMap{ { 1, "value" } }));
    testVariantToString(
        "QCborSimpleType",
        qVar(QCborSimpleType::True));

    qDebug() << "\n=== XML Types ===";
    QDomDocument doc;
    QDomElement element = doc.createElement("test");
    element.setAttribute("attr", "value");
    testVariantToString("QDomElement", qVar(element));

    qDebug() << "\n=== Special Types ===";
    testVariantToString("Nullptr", qVar(nullptr));
    testVariantToString("Invalid", QVariant());
    testVariantToString("Null QString", QVariant(QString()));
    testVariantToString("QObjectStar", qVar(new QObject));

    return 0;
}*/

// Output:

/*=== String Types ===
QString                  : Hello
QByteArray               : Byte Array
QChar                    : A

=== Numeric Types ===
Bool                     : true
Int                      : 55
UInt                     : 55
Long                     : 55
ULong                    : 55
LongLong                 : 55
ULongLong                : 55
Short                    : 55
UShort                   : 55
Char                     : X
SChar                    : Y
UChar                    : Z
Float                    : 3.140000104904175
Double                   : 3.14159

=== Date/Time Types ===
QDate                    : 2025-11-12
QTime                    : 14:30:45.000
QDateTime                : 2025-11-12T22:06:24.208

=== Simple Qt Types ===
QUrl                     : https://example.com
QUuid                    : {d180b01f-c6f1-4a48-a0d6-c8a1b6ac6ae8}
QLocale                  : <empty>

=== Geometric Types ===
QPoint                   : <empty>
QPointF                  : <empty>
QSize                    : <empty>
QSizeF                   : <empty>
QRect                    : <empty>
QRectF                   : <empty>
QLine                    : <empty>
QLineF                   : <empty>

=== Container Types ===
QStringList              : <empty>
QVariantList             : <empty>
QVariantMap              : <empty>
QVariantHash             : <empty>

=== Model Types ===
QModelIndex              : <empty>

=== JSON Types ===
QJsonValue               : json string
QJsonObject              : <empty>
QJsonArray               : <empty>
QJsonDocument            : <empty>

=== CBOR Types ===
QCborValue               : cbor string
QCborArray               : <empty>
QCborMap                 : <empty>
QCborSimpleType          : 21

=== XML Types ===
QDomElement              : <empty>

=== Special Types ===
Nullptr                  : <empty>
Invalid                  : <empty>
Null QString             : <empty>
QObjectStar              : <empty>*/
