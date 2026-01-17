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

/// TODO STYLE: Base class (See: StyleModule::sortThemes_)? At the very least,
/// silo off common code!

class EditorTheme
{
public:
    static constexpr auto EXT = ".fernanda_editor";

    // Needed for invalid theme!
    EditorTheme() = default;

    EditorTheme(const Coco::Path& path)
        : path_(path)
    {
        parse_(path);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    bool isValid() const noexcept { return !name_.isEmpty(); }

    QPalette palette() const
    {
        QPalette palette{};

        if (base_.isValid()) palette.setColor(QPalette::Base, base_);
        if (text_.isValid()) palette.setColor(QPalette::Text, text_);
        if (highlight_.isValid())
            palette.setColor(QPalette::Highlight, highlight_);
        if (highlightedText_.isValid())
            palette.setColor(QPalette::HighlightedText, highlightedText_);

        return palette;
    }

private:
    Coco::Path path_;

    static constexpr auto NAME_ = "name";
    QString name_{};

    static constexpr auto VALUES_ = "values";

    // Palette

    static constexpr auto VAR_BASE_ = "base";
    QColor base_{};
    static constexpr auto VAR_TEXT_ = "text";
    QColor text_{};
    static constexpr auto VAR_HIGHLIGHT_ = "highlight";
    QColor highlight_{};
    static constexpr auto VAR_HIGHLIGHTED_TEXT_ = "highlightedText";
    QColor highlightedText_{};

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

        // TODO: debug WARNs!
        if (parse_error.error != QJsonParseError::NoError) return;
        if (!document.isObject()) return;

        auto root = document.object();

        name_ = root[NAME_].toString();

        auto values = root[VALUES_].toObject();
        if (values.isEmpty()) return;

        base_ = values[VAR_BASE_].toString();
        text_ = values[VAR_TEXT_].toString();
        highlight_ = values[VAR_HIGHLIGHT_].toString();
        highlightedText_ = values[VAR_HIGHLIGHTED_TEXT_].toString();
    }
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
        parse_(path);
    }

    Coco::Path path() const noexcept { return path_; }
    QString name() const noexcept { return name_; }
    bool isValid() const noexcept { return !name_.isEmpty(); }

    QColor iconColor() const noexcept { return iconColor_; }

    QPalette palette() const
    {
        QPalette palette{};

        if (window_.isValid()) palette.setColor(QPalette::Window, window_);
        if (windowText_.isValid())
            palette.setColor(QPalette::WindowText, windowText_);
        if (base_.isValid()) palette.setColor(QPalette::Base, base_);
        if (text_.isValid()) palette.setColor(QPalette::Text, text_);
        if (button_.isValid()) palette.setColor(QPalette::Button, button_);
        if (buttonText_.isValid())
            palette.setColor(QPalette::ButtonText, buttonText_);
        if (highlight_.isValid())
            palette.setColor(QPalette::Highlight, highlight_);
        if (highlightedText_.isValid())
            palette.setColor(QPalette::HighlightedText, highlightedText_);
        if (light_.isValid()) palette.setColor(QPalette::Light, light_);
        if (midlight_.isValid())
            palette.setColor(QPalette::Midlight, midlight_);
        if (mid_.isValid()) palette.setColor(QPalette::Mid, mid_);
        if (dark_.isValid()) palette.setColor(QPalette::Dark, dark_);
        if (shadow_.isValid()) palette.setColor(QPalette::Shadow, shadow_);
        if (brightText_.isValid())
            palette.setColor(QPalette::BrightText, brightText_);
        if (link_.isValid()) palette.setColor(QPalette::Link, link_);
        if (linkVisited_.isValid())
            palette.setColor(QPalette::LinkVisited, linkVisited_);
        if (alternateBase_.isValid())
            palette.setColor(QPalette::AlternateBase, alternateBase_);
        if (tooltipBase_.isValid())
            palette.setColor(QPalette::ToolTipBase, tooltipBase_);
        if (tooltipText_.isValid())
            palette.setColor(QPalette::ToolTipText, tooltipText_);
        if (placeholderText_.isValid())
            palette.setColor(QPalette::PlaceholderText, placeholderText_);
        if (accent_.isValid()) palette.setColor(QPalette::Accent, accent_);

        return palette;
    }

private:
    Coco::Path path_;

    static constexpr auto NAME_ = "name";
    QString name_{};

    static constexpr auto VALUES_ = "values";

    // Non-palette

    static constexpr auto VAR_ICON_COLOR_ = "iconColor";
    QColor iconColor_{};

    // Palette

    static constexpr auto VAR_WINDOW_ = "window";
    QColor window_{};
    static constexpr auto VAR_WINDOW_TEXT_ = "windowText";
    QColor windowText_{};
    static constexpr auto VAR_BASE_ = "base";
    QColor base_{};
    static constexpr auto VAR_TEXT_ = "text";
    QColor text_{};
    static constexpr auto VAR_BUTTON_ = "button";
    QColor button_{};
    static constexpr auto VAR_BUTTON_TEXT_ = "buttonText";
    QColor buttonText_{};
    static constexpr auto VAR_HIGHLIGHT_ = "highlight";
    QColor highlight_{};
    static constexpr auto VAR_HIGHLIGHTED_TEXT_ = "highlightedText";
    QColor highlightedText_{};
    static constexpr auto VAR_LIGHT_ = "light";
    QColor light_{};
    static constexpr auto VAR_MIDLIGHT_ = "midlight";
    QColor midlight_{};
    static constexpr auto VAR_MID_ = "mid";
    QColor mid_{};
    static constexpr auto VAR_DARK_ = "dark";
    QColor dark_{};
    static constexpr auto VAR_SHADOW_ = "shadow";
    QColor shadow_{};
    static constexpr auto VAR_BRIGHT_TEXT_ = "brightText";
    QColor brightText_{};
    static constexpr auto VAR_LINK_ = "link";
    QColor link_{};
    static constexpr auto VAR_LINK_VISITED_ = "linkVisited";
    QColor linkVisited_{};
    static constexpr auto VAR_ALTERNATE_BASE_ = "alternateBase";
    QColor alternateBase_{};
    static constexpr auto VAR_TOOLTIP_BASE_ = "tooltipBase";
    QColor tooltipBase_{};
    static constexpr auto VAR_TOOLTIP_TEXT_ = "tooltipText";
    QColor tooltipText_{};
    static constexpr auto VAR_PLACEHOLDER_TEXT_ = "placeholderText";
    QColor placeholderText_{};
    static constexpr auto VAR_ACCENT_ = "accent";
    QColor accent_{};

    void parse_(const Coco::Path& path)
    {
        // TODO: Example theme

        auto data = Io::read(path);

        QJsonParseError parse_error{};
        auto document = QJsonDocument::fromJson(data, &parse_error);

        // TODO: debug WARNs!
        if (parse_error.error != QJsonParseError::NoError) return;
        if (!document.isObject()) return;

        auto root = document.object();

        name_ = root[NAME_].toString();

        auto values = root[VALUES_].toObject();
        if (values.isEmpty()) return;

        iconColor_ = values[VAR_ICON_COLOR_].toString();

        window_ = values[VAR_WINDOW_].toString();
        windowText_ = values[VAR_WINDOW_TEXT_].toString();
        base_ = values[VAR_BASE_].toString();
        text_ = values[VAR_TEXT_].toString();
        button_ = values[VAR_BUTTON_].toString();
        buttonText_ = values[VAR_BUTTON_TEXT_].toString();
        highlight_ = values[VAR_HIGHLIGHT_].toString();
        highlightedText_ = values[VAR_HIGHLIGHTED_TEXT_].toString();
        light_ = values[VAR_LIGHT_].toString();
        midlight_ = values[VAR_MIDLIGHT_].toString();
        mid_ = values[VAR_MID_].toString();
        dark_ = values[VAR_DARK_].toString();
        shadow_ = values[VAR_SHADOW_].toString();
        brightText_ = values[VAR_BRIGHT_TEXT_].toString();
        link_ = values[VAR_LINK_].toString();
        linkVisited_ = values[VAR_LINK_VISITED_].toString();
        alternateBase_ = values[VAR_ALTERNATE_BASE_].toString();
        tooltipBase_ = values[VAR_TOOLTIP_BASE_].toString();
        tooltipText_ = values[VAR_TOOLTIP_TEXT_].toString();
        placeholderText_ = values[VAR_PLACEHOLDER_TEXT_].toString();
        accent_ = values[VAR_ACCENT_].toString();
    }
};

} // namespace Fernanda
