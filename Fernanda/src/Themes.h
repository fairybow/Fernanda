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

// TODO: I do not like how separate yet not separate the theme classes are. Fix
// it! Somehow! Base class (See: StyleModule::sortThemes_)?

namespace Themes {

    struct ParseResult
    {
        QString name{};
        QJsonObject values{};
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
        result.values = root["values"].toObject();
        result.valid = !result.name.isEmpty() && !result.values.isEmpty();

        return result;
    }

    inline QPalette buildPalette(
        std::initializer_list<std::pair<QPalette::ColorRole, QColor>>
            assignments)
    {
        QPalette palette{};

        for (auto& [role, color] : assignments)
            if (color.isValid()) palette.setColor(role, color);

        return palette;
    }

} // namespace Themes

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

        name_ = parsed.name;
        auto& v = parsed.values;

        base_ = v["base"].toString();
        text_ = v["text"].toString();
        highlight_ = v["highlight"].toString();
        highlightedText_ = v["highlightedText"].toString();
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    bool isValid() const noexcept { return !name_.isEmpty(); }

    QPalette palette() const
    {
        return Themes::buildPalette(
            { { QPalette::Base, base_ },
              { QPalette::Text, text_ },
              { QPalette::Highlight, highlight_ },
              { QPalette::HighlightedText, highlightedText_ } });
    }

private:
    Coco::Path path_;

    QString name_{};

    // Palette

    QColor base_{};
    QColor text_{};
    QColor highlight_{};
    QColor highlightedText_{};
};

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

        name_ = parsed.name;
        auto& v = parsed.values;

        iconColor_ = v["iconColor"].toString();

        window_ = v["window"].toString();
        windowText_ = v["windowText"].toString();
        base_ = v["base"].toString();
        text_ = v["text"].toString();
        button_ = v["button"].toString();
        buttonText_ = v["buttonText"].toString();
        highlight_ = v["highlight"].toString();
        highlightedText_ = v["highlightedText"].toString();
        light_ = v["light"].toString();
        midlight_ = v["midlight"].toString();
        mid_ = v["mid"].toString();
        dark_ = v["dark"].toString();
        shadow_ = v["shadow"].toString();
        brightText_ = v["brightText"].toString();
        link_ = v["link"].toString();
        linkVisited_ = v["linkVisited"].toString();
        alternateBase_ = v["alternateBase"].toString();
        tooltipBase_ = v["tooltipBase"].toString();
        tooltipText_ = v["tooltipText"].toString();
        placeholderText_ = v["placeholderText"].toString();
        accent_ = v["accent"].toString();
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    bool isValid() const noexcept { return !name_.isEmpty(); }

    QColor iconColor() const noexcept { return iconColor_; }

    QString menuStyleSheet() const
    {
        QPalette fallback{};

        return QString(
                   "QMenuBar {"
                   "    background-color: %1;"
                   "    color: %2;"
                   "}"
                   "QMenuBar::item:selected {"
                   "    background-color: %3;"
                   "    color: %4;"
                   "}"
                   "QMenu {"
                   "    background-color: %1;"
                   "    color: %2;"
                   "}"
                   "QMenu::item:selected {"
                   "    background-color: %3;"
                   "    color: %4;"
                   "}"
                   "QMenu::separator {"
                   "    background-color: %5;"
                   "    height: 1px;"
                   "    margin: 4px 8px;"
                   "}")
            .arg(
                window_.isValid() ? window_.name()
                                  : fallback.color(QPalette::Window).name(),
                windowText_.isValid()
                    ? windowText_.name()
                    : fallback.color(QPalette::WindowText).name(),
                highlight_.isValid()
                    ? highlight_.name()
                    : fallback.color(QPalette::Highlight).name(),
                highlightedText_.isValid()
                    ? highlightedText_.name()
                    : fallback.color(QPalette::HighlightedText).name(),
                mid_.isValid() ? mid_.name()
                               : fallback.color(QPalette::Mid).name());
    }

    QPalette palette() const
    {
        return Themes::buildPalette(
            { { QPalette::Window, window_ },
              { QPalette::WindowText, windowText_ },
              { QPalette::Base, base_ },
              { QPalette::Text, text_ },
              { QPalette::Button, button_ },
              { QPalette::ButtonText, buttonText_ },
              { QPalette::Highlight, highlight_ },
              { QPalette::HighlightedText, highlightedText_ },
              { QPalette::Light, light_ },
              { QPalette::Midlight, midlight_ },
              { QPalette::Mid, mid_ },
              { QPalette::Dark, dark_ },
              { QPalette::Shadow, shadow_ },
              { QPalette::BrightText, brightText_ },
              { QPalette::Link, link_ },
              { QPalette::LinkVisited, linkVisited_ },
              { QPalette::AlternateBase, alternateBase_ },
              { QPalette::ToolTipBase, tooltipBase_ },
              { QPalette::ToolTipText, tooltipText_ },
              { QPalette::PlaceholderText, placeholderText_ },
              { QPalette::Accent, accent_ } });
    }

private:
    Coco::Path path_;

    QString name_{};

    // Non-palette

    QColor iconColor_{};

    // Palette

    QColor window_{};
    QColor windowText_{};
    QColor base_{};
    QColor text_{};
    QColor button_{};
    QColor buttonText_{};
    QColor highlight_{};
    QColor highlightedText_{};
    QColor light_{};
    QColor midlight_{};
    QColor mid_{};
    QColor dark_{};
    QColor shadow_{};
    QColor brightText_{};
    QColor link_{};
    QColor linkVisited_{};
    QColor alternateBase_{};
    QColor tooltipBase_{};
    QColor tooltipText_{};
    QColor placeholderText_{};
    QColor accent_{};
};

} // namespace Fernanda
