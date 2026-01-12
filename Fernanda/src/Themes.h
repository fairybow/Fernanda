/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QByteArray>
#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPalette>
#include <QString>

#include "Coco/Path.h"

#include "Debug.h"
#include "Io.h"

namespace Fernanda {

class EditorTheme
{
public:
    static constexpr auto EXT = ".fernanda_editor";

    EditorTheme() = default;

    EditorTheme(const Coco::Path& path)
        : path_(path)
    {
        parse_(path);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }

    bool isValid() const noexcept
    {
        return background_.isValid() && font_.isValid() && selection_.isValid()
               && selectedFont_.isValid();
    }

    QPalette palette() const
    {
        QPalette palette{};
        palette.setColor(QPalette::Base, background_);
        palette.setColor(QPalette::Text, font_);
        palette.setColor(QPalette::Highlight, selection_);
        palette.setColor(QPalette::HighlightedText, selectedFont_);
        return palette;
    }

private:
    Coco::Path path_{};

    static constexpr auto NAME_ = "name";
    QString name_{};

    static constexpr auto VALUES_ = "values";

    static constexpr auto VAR_BG_ = "backgroundColor";
    QColor background_{};
    static constexpr auto VAR_FONT_ = "fontColor";
    QColor font_{};
    static constexpr auto VAR_SELECTION_ = "selectionBgColor";
    QColor selection_{};
    static constexpr auto VAR_SELECTED_FONT_ = "selectionFontColor";
    QColor selectedFont_{};

    void parse_(const Coco::Path& path)
    {
        // Example editor theme file contents:
        /*{
          "name": "Pocket",
          "values": {
            "backgroundColor": "#b6bc9f",
            "fontColor": "#1b211b",
            "selectionFontColor": "#b6bc9f",
            "selectionBgColor": "#1b211b"
          }
        }
        */

        auto data = Io::read(path);

        QJsonParseError parse_error{};
        auto document = QJsonDocument::fromJson(data, &parse_error);

        // TODO: debug WARN (maybe INFO, if "no theme" option relies on invalid
        // theme
        if (parse_error.error != QJsonParseError::NoError) return;
        if (!document.isObject()) return;

        auto root = document.object();

        name_ = root[NAME_].toString();

        auto values = root[VALUES_].toObject();
        if (values.isEmpty()) return;

        background_ = values[VAR_BG_].toString();
        font_ = values[VAR_FONT_].toString();
        selection_ = values[VAR_SELECTION_].toString();
        selectedFont_ = values[VAR_SELECTED_FONT_].toString();
    }
};

class WindowTheme
{
    //...
};

} // namespace Fernanda
