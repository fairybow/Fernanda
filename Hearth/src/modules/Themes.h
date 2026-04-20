/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <utility>

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QString>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Io.h"
#include "modules/Qss.h"

namespace Hearth {

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
    WindowTheme() { qss_ = Qss::render(templateQss_(), {}); }

    WindowTheme(const Coco::Path& path)
        : path_(path)
    {
        auto parsed = Themes::parse(path);
        if (!parsed.valid) return;

        name_ = parsed.name;

        // Extract iconColor separately (used by StyleContext, not QSS)
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

    QString templateQss_() const
    {
        static const auto data = QString::fromUtf8(
            Io::read(":/themes/Window.fernanda_qss_template"));
        return data;
    }
};

class EditorTheme
{
public:
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

    QString templateQss_() const
    {
        static const auto data = QString::fromUtf8(
            Io::read(":/themes/Editor.fernanda_qss_template"));
        return data;
    }
};

} // namespace Hearth
