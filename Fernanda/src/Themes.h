/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <utility>

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QString>

#include "Coco/Path.h"

#include "Debug.h"
#include "Io.h"
#include "Qss.h"

namespace Fernanda {

// TODO: I do not like how separate yet not separate the theme classes are. Fix
// it! Somehow! Base class (See: StyleModule::sortThemes_)?

namespace Themes {

    struct ParseResult
    {
        QString name{};
        QHash<QString, QString> assignments{};
        bool valid = false;
    };

    // A valid theme has a name and at least one assignment (`{ "name": "Name",
    // "values": { "key": "value" } }`)
    inline ParseResult parse(const Coco::Path& path)
    {
        ParseResult result{};

        auto data = Io::read(path);

        QJsonParseError parse_error{};
        auto document = QJsonDocument::fromJson(data, &parse_error);

        // TODO: debug WARNs!
        if (parse_error.error != QJsonParseError::NoError) return result;
        if (!document.isObject()) return result;

        auto root = document.object();
        result.name = root["name"].toString();

        auto values = root["values"].toObject();
        for (auto it = values.begin(); it != values.end(); ++it)
            result.assignments[it.key()] = it.value().toString();

        result.valid = !result.name.isEmpty() && !result.assignments.isEmpty();

        return result;
    }

} // namespace Themes

class WindowTheme
{
public:
    static constexpr auto EXT = ".fernanda_window";

    // Needed for invalid theme!
    WindowTheme() = default;

    WindowTheme(const Coco::Path& path)
        : path_(path)
    {
        auto parsed = Themes::parse(path);
        if (!parsed.valid) return;

        // Though a theme requires both a name and values array, since this
        // (name_) doesn't get set if parsing is invalid, this is sufficient to
        // stand in for "is valid" generally
        name_ = parsed.name;

        // Extract iconColor separately (used by ProxyStyle, not QSS)
        if (parsed.assignments.contains("iconColor")) {
            iconColor_ = QColor(parsed.assignments.take("iconColor"));
        }

        static const auto templ = Io::read(":/themes/Window.qss.template");
        qss_ = Qss::render(templ, parsed.assignments);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    bool isValid() const noexcept { return !name_.isEmpty(); }

    QColor iconColor() const noexcept { return iconColor_; }
    QString qss() const noexcept { return qss_; }

private:
    Coco::Path path_;

    QString name_{};
    QColor iconColor_{};
    QString qss_{};
};

class EditorTheme
{
public:
    static constexpr auto EXT = ".fernanda_editor";

    // Needed for invalid theme!
    EditorTheme() = default;

    EditorTheme(const Coco::Path& path)
        : path_(path)
    {
        auto parsed = Themes::parse(path);
        if (!parsed.valid) return;

        // Though a theme requires both a name and values array, since this
        // (name_) doesn't get set if parsing is invalid, this is sufficient to
        // stand in for "is valid" generally
        name_ = parsed.name;

        static const auto templ = Io::read(":/themes/Editor.qss.template");
        qss_ = Qss::render(templ, parsed.assignments);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    bool isValid() const noexcept { return !name_.isEmpty(); }

    QString qss() const noexcept { return qss_; }

private:
    Coco::Path path_;

    QString name_{};
    QString qss_{};
};

} // namespace Fernanda
