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

    WindowTheme() { qss_ = Qss::render(templateQss_(), {}); }

    WindowTheme(const Coco::Path& path)
        : path_(path)
    {
        auto parsed = Themes::parse(path);
        if (!parsed.valid) return;

        name_ = parsed.name;

        // Extract iconColor separately (used by ProxyStyle, not QSS)
        if (parsed.assignments.contains("iconColor")) {
            iconColor_ = QColor(parsed.assignments.take("iconColor"));
        }

        qss_ = Qss::render(templateQss_(), parsed.assignments);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    QString qss() const noexcept { return qss_; }

    QColor iconColor() const noexcept { return iconColor_; }
    bool hasIconColor() const noexcept { return iconColor_.isValid(); }

private:
    Coco::Path path_;

    QString name_{};
    QColor iconColor_{};
    QString qss_{};

    QByteArray templateQss_() const
    {
        static const auto data =
            Io::read(":/themes/Window.fernanda_qss_template");
        return data;
    }
};

class EditorTheme
{
public:
    static constexpr auto EXT = ".fernanda_editor";

    EditorTheme() { qss_ = Qss::render(templateQss_(), {}); }

    EditorTheme(const Coco::Path& path)
        : path_(path)
    {
        auto parsed = Themes::parse(path);
        if (!parsed.valid) return;

        name_ = parsed.name;
        qss_ = Qss::render(templateQss_(), parsed.assignments);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    QString qss() const noexcept { return qss_; }

private:
    Coco::Path path_;

    QString name_{};
    QString qss_{};

    QByteArray templateQss_() const
    {
        static const auto data =
            Io::read(":/themes/Editor.fernanda_qss_template");
        return data;
    }
};

} // namespace Fernanda
