/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <concepts>
#include <type_traits>

#include <QDomAttr>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QHashIterator>
#include <QLatin1StringView>
#include <QMapIterator>
#include <QMetaObject>
#include <QMetaType>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QVariant>
#include <QVariantHash>
#include <QVariantMap>

#include <Coco/Bool.h>
#include <Coco/Concepts.h>
#include <Coco/Path.h>

namespace Fernanda {

using namespace Qt::StringLiterals;

// Forward declarations for mutually-recursive overloads. QVariant can hold any
// of these, and the container overloads call back into toQString for values
QString toQString(const QVariant& variant);
QString toQString(const QVariantHash& variantHash);
QString toQString(const QVariantMap& variantMap);

// --- Strings ---

// Passthrough (implicitly shared, no copy)
inline QString toQString(const QString& s) { return s; }
inline QString toQString(QStringView s) { return s.toString(); }
inline QString toQString(QLatin1StringView s) { return s.toString(); }
inline QString toQString(const char* s) { return QString::fromUtf8(s); }

// --- Bool ---

inline QString toQString(bool b) { return b ? u"true"_s : u"false"_s; }

// --- Numerics ---

template <typename T>
    requires std::integral<T> && (!std::same_as<T, bool>)
             && (!std::same_as<T, char>)
inline QString toQString(T value)
{
    return QString::number(value);
}

template <std::floating_point T> inline QString toQString(T value)
{
    return QString::number(value);
}

// --- Pointers ---

// Ptr can be nullptr
template <typename T> inline QString toQString(const T* ptr)
{
    if (!ptr) return u"nullptr"_s;

    // TODO: Untested - check print output (implementation defined)
    return QString::asprintf(
        "%s(%p)",
        typeid(T).name(),
        static_cast<const void*>(ptr));
}

// Ptr can be nullptr. Overrides the generic pointer overload via partial
// ordering when T derives from QObject
template <Coco::Concepts::QObjectDerived T>
inline QString toQString(const T* ptr)
{
    if (!ptr) return u"nullptr"_s;

    return QString::asprintf(
        "%s(%p)",
        ptr->metaObject()->className(),
        static_cast<const void*>(ptr));
}

// --- Qt value types ---

inline QString toQString(const QModelIndex& index)
{
    if (!index.isValid()) return u"QModelIndex(Invalid)"_s;

    return QString::asprintf(
        "QModelIndex(row:%d, col:%d, %p)",
        index.row(),
        index.column(),
        index.internalPointer());
}

inline QString toQString(const QPoint& point)
{
    return QString::asprintf("QPoint(x:%d, y:%d)", point.x(), point.y());
}

inline QString toQString(const QStringList& list) { return list.join(u", "_s); }

inline QString toQString(const QDomElement& element)
{
    if (element.isNull()) return u"QDomElement(Null)"_s;

    auto tag = element.tagName();
    auto attrs = element.attributes();
    auto count = attrs.count();

    if (count == 0) {
        QString out{};
        out.reserve(tag.size() + 16); // "QDomElement(<>)" + tag
        out.append(u"QDomElement(<"_s);
        out.append(tag);
        out.append(u">)"_s);

        return out;
    }

    // Rough size estimate: tag + per-attr overhead + names/values
    qsizetype estimate = tag.size() + 16;

    for (auto i = 0; i < count; ++i) {
        auto attr = attrs.item(i).toAttr();
        estimate += attr.name().size() + attr.value().size() + 5; // " ='"
    }

    QString out{};
    out.reserve(estimate);
    out.append(u"QDomElement(<"_s);
    out.append(tag);

    for (auto i = 0; i < count; ++i) {
        auto attr = attrs.item(i).toAttr();
        out.append(u' ');
        out.append(attr.name());
        out.append(u"='"_s);
        out.append(attr.value());
        out.append(u'\'');
    }

    out.append(u">)"_s);
    return out;
}

// --- Variant containers ---

// Like QVariant::toString, we don't wrap printable values in "QVariant(...)"
inline QString toQString(const QVariant& variant)
{
    if (!variant.isValid()) return u"QVariant(Invalid)"_s;
    if (variant.isNull()) return u"QVariant(Null)"_s;

    // Check for QObject-derived pointer types via meta-type flags (the
    // documented-correct way). canConvert<QObject*>() + value<QObject*>() is
    // unreliable for subclasses
    if (variant.metaType().flags() & QMetaType::PointerToQObject) {
        return toQString(variant.value<QObject*>());
    }

    if (variant.canConvert<QDomElement>()) {
        return toQString(variant.value<QDomElement>());
    }

    switch (variant.typeId()) {
    case QMetaType::QVariantMap:
        return toQString(variant.value<QVariantMap>());

    case QMetaType::QVariantHash:
        return toQString(variant.value<QVariantHash>());

    case QMetaType::QModelIndex:
        return toQString(variant.value<QModelIndex>());

    case QMetaType::QPoint:
        return toQString(variant.value<QPoint>());

    case QMetaType::QStringList:
        return toQString(variant.value<QStringList>());

    default:
        auto text = variant.toString();
        return text.isEmpty() ? u"QVariant(Non-printable)"_s : text;
    }
}

inline QString toQString(const QVariantHash& variantHash)
{
    if (variantHash.isEmpty()) return u"QVariantHash()"_s;

    QString out{};
    out.reserve(64 + variantHash.size() * 32); // rough guess
    out.append(u"QVariantHash("_s);

    auto first = true;
    QHashIterator<QString, QVariant> it(variantHash);

    while (it.hasNext()) {
        it.next();
        if (!first) out.append(u", "_s);
        first = false;

        out.append(u"{ \""_s);
        out.append(it.key());
        out.append(u"\", "_s);
        out.append(toQString(it.value()));
        out.append(u" }"_s);
    }

    out.append(u')');
    return out;
}

inline QString toQString(const QVariantMap& variantMap)
{
    if (variantMap.isEmpty()) return u"QVariantMap()"_s;

    QString out{};
    out.reserve(64 + variantMap.size() * 32); // rough guess
    out.append(u"QVariantMap("_s);

    auto first = true;
    QMapIterator<QString, QVariant> it(variantMap);

    while (it.hasNext()) {
        it.next();
        if (!first) out.append(u", "_s);
        first = false;

        out.append(u"{ \""_s);
        out.append(it.key());
        out.append(u"\", "_s);
        out.append(toQString(it.value()));
        out.append(u" }"_s);
    }

    out.append(u')');
    return out;
}

// --- Coco types ---

inline QString toQString(const Coco::Path& path) { return path.toQString(); }

// TODO: Add Coco::Bool::toString/toQString
// Reproduces Coco::Bool's QDebug/formatter output: "TagName::Yes" or
// "TagName::No"
template <typename TagT> inline QString toQString(const Coco::Bool<TagT>& b)
{
    return QString::asprintf("%s::%s", TagT::name(), b ? "Yes" : "No");
}

} // namespace Fernanda

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
