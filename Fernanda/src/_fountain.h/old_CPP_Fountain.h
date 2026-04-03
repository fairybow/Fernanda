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

/// Old:

/*
 * Fountain.h — a platform-agnostic C++ library for parsing, paginating, and
 * rendering Fountain screenplays (https://fountain.io). Translated from the
 * original Objective-C implementation by Nima Yousefi and John August
 * (https://github.com/nyousefi/Fountain)
 *
 * https://github.com/nyousefi/Fountain/blob/master/Fountain/FNElement.h
 * https://github.com/nyousefi/Fountain/blob/master/Fountain/FastFountainParser.h
 * https://github.com/nyousefi/Fountain/blob/master/Fountain/FNHTMLScript.h
 * https://github.com/nyousefi/Fountain/blob/master/Fountain/FNPaginator.h
 *
 * MIT License
 *
 * Copyright (c) 2026 fairybow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Fountain {

// The **Parser** splits raw Fountain text into two separate outputs: title page
// entries (key-value metadata like title, author, contact) and body elements
// (scene headings, action, dialogue, etc.). These are fully independent data
// sets
//
// The **Paginator** operates on body elements only, distributing them across
// pages of a given size, handling page-break logic for scene headings, dialogue
// splitting (with MORE/CONT'D), and parenthetical break rules. The paginator
// has no knowledge of title pages. Page dimensions default to US Letter
// (612x792) in PostScript points at 72 points/inch. This is an abstract
// coordinate system for layout math, not screen pixels. High-DPI scaling is a
// display-layer concern
//
// The **Renderer** converts parsed data into a single HTML document. Title page
// and body are rendered separately (`renderTitlePage_` and `renderBody_`), then
// concatenated. The renderer emits flow HTML with CSS classes (it does NOT
// perform spatial layout for the title page (e.g., vertically centering the
// title or anchoring contact info to the bottom). That is a rendering-surface
// concern left to the consuming application)
//
// The only platform dependency is `Paginator::MeasureTextFn`, a callback that
// measures text height given a string, max width, and line height. The
// consuming application provides this using its own font metrics engine (e.g.,
// QFontMetrics in Qt, NSLayoutManager on Apple). If no callback is provided,
// the Renderer skips pagination and emits a continuous document
//
// Re: title page layout: The Fountain spec recommends that title, credit, and
// author are centered vertically on the page, with contact/copyright/date
// anchored to the bottom-left. This spatial arrangement requires either CSS
// absolute positioning or coordinate-based drawing, both of which are
// platform-specific. This library provides the structured data
// (`Parser::titlePage()`) and structured HTML (`Renderer::renderTitlePage_()`).
// The consuming application is responsible for spatial placement. This matches
// the original Obj-C library's design (FNHTMLScript emits flow HTML for the
// title page, and FNPaginator only paginates body elements)
//
// Re: page count accuracy: Page counts may differ roughly 5 to 10% from other
// implementations due to font metric differences across platforms. The original
// was calibrated for Courier 12pt on macOS via NSLayoutManager. Fine-tuning
// requires adjusting the MeasureTextFn implementation or the element
// width/margin constants to match the target font and platform
//
// Usage example (Qt):
//
// clang-format off
// ```
// virtual QString renderToHtml(const QString& plainText) const override
// {
//     auto parser = Fountain::Parser(plainText.toStdString());
//     auto font = QFont("Courier Prime");
//     font.setPixelSize(12);
//
//     auto measure_fn = [font](
//                           const std::string& text,
//                           int maxWidth,
//                           int lineHeight) -> int {
//         QTextLayout layout(QString::fromStdString(text), font);
//         layout.beginLayout();
//
//         auto line_count = 0;
//
//         while (true) {
//             auto line = layout.createLine();
//             if (!line.isValid()) break;
//
//             line.setLineWidth(maxWidth);
//             ++line_count;
//         }
//
//         layout.endLayout();
//         return line_count * lineHeight;
//     };
//
//     auto renderer = Fountain::Renderer(parser, measure_fn);
//     return QString::fromStdString(renderer.html());
// }
// ```
// clang-format on

struct TitleEntry
{
    std::string key{};
    std::vector<std::string> values{};

    friend std::ostream& operator<<(std::ostream& out, const TitleEntry& entry)
    {
        out << "[ title key: " << entry.key << "; values:";

        for (const auto& v : entry.values) {
            out << " " << v;
        }

        return out << " ]";
    }
};

// TODO (maybe): If we can always trim excess whitespace on either side without
// losing any meaning, then we can ditch all of the `trim_` calls when adding
// elements and just add a ctor that trims
struct Element
{
    enum Type
    {
        Null = 0, // Shouldn't happen
        SceneHeading,
        Action,
        Character,
        Dialogue,
        Parenthetical,
        Transition,
        Lyrics,
        LyricsSpacer,
        PageBreak,
        Boneyard,
        Comment,
        SectionHeading,
        Synopsis
    };

    Type type{};
    std::string text{};
    bool textCentered = false;

    // Type specific:
    std::string sceneNumber{}; // e.g. "1A"
    bool isDualDialogueCharacter = false;
    int sectionDepth = -1; // 1-6

    friend std::ostream& operator<<(std::ostream& out, const Element& element)
    {
        out << "[ type: " << element.typeString() << "; text: " << element.text;

        if (element.textCentered) out << "; centered";
        if (element.isDualDialogueCharacter) out << "; dual";

        if (!element.sceneNumber.empty()) {
            out << "; scene#: " << element.sceneNumber;
        }

        if (element.sectionDepth >= 0) {
            out << "; depth: " << element.sectionDepth;
        }

        return out << " ]";
    }

    std::string typeString() const noexcept
    {
        switch (type) {
        default:
        case Element::Null:
            return "NULL"; // Shouldn't happen
        case Element::SceneHeading:
            return "SceneHeading";
        case Element::Action:
            return "Action";
        case Element::Character:
            return "Character";
        case Element::Dialogue:
            return "Dialogue";
        case Element::Parenthetical:
            return "Parenthetical";
        case Element::Transition:
            return "Transition";
        case Element::Lyrics:
            return "Lyrics";
        case Element::LyricsSpacer:
            return "LyricsSpacer";
        case Element::PageBreak:
            return "PageBreak";
        case Element::Boneyard:
            return "Boneyard";
        case Element::Comment:
            return "Comment";
        case Element::SectionHeading:
            return "SectionHeading";
        case Element::Synopsis:
            return "Synopsis";
        }
    }
};

// TODO: The multi-line boneyard still stores comment_text untrimmed (leading
// \n); the boneyard close check fires outside boneyard context (inherited ObjC
// bug); the boneyard close silently drops non-whitespace content on the */ line
// (inherited; by design?); multi-line [[notes]] aren't handled (also inherited,
// only single-line notes work; by design?)
class Parser
{
public:
    explicit Parser(std::string source) { parse_(std::move(source)); }

    const std::vector<TitleEntry>& titlePage() const { return titlePage_; }
    const std::vector<Element>& elements() const { return elements_; }

private:
    typedef unsigned char uchar;

    std::vector<TitleEntry> titlePage_{};
    std::vector<Element> elements_{};

    // --- Stringwork ---

    static void normalizeLineEndings_(std::string& s)
    {
        size_t write = 0;

        for (size_t read = 0; read < s.size(); ++read) {
            if (s[read] == '\r') {
                s[write++] = '\n';

                if (read + 1 < s.size() && s[read + 1] == '\n') {
                    ++read; // consume \r\n as single \n
                }

            } else {
                s[write++] = s[read];
            }
        }

        s.resize(write);
    }

    static void trimLeading_(std::string& s)
    {
        auto it = std::find_if_not(s.begin(), s.end(), [](uchar c) {
            return std::isspace(c);
        });

        s.erase(s.begin(), it);
    }

    static std::string trimmedLeading_(std::string_view s)
    {
        auto result = std::string(s);
        trimLeading_(result);

        return result;
    }

    static std::string_view trimmedLeadingView_(std::string_view s)
    {
        while (!s.empty() && std::isspace(static_cast<uchar>(s.front()))) {
            s.remove_prefix(1);
        }

        return s;
    }

    static void trim_(std::string& s)
    {
        auto end = std::find_if_not(s.rbegin(), s.rend(), [](uchar c) {
                       return std::isspace(c);
                   }).base();

        s.erase(end, s.end());

        auto start = std::find_if_not(s.begin(), s.end(), [](uchar c) {
            return std::isspace(c);
        });

        s.erase(s.begin(), start);
    }

    static std::string trimmed_(std::string_view s)
    {
        auto result = std::string(s);
        trim_(result);

        return result;
    }

    static std::string_view trimmedView_(std::string_view s)
    {
        while (!s.empty() && std::isspace(static_cast<uchar>(s.front()))) {
            s.remove_prefix(1);
        }

        while (!s.empty() && std::isspace(static_cast<uchar>(s.back()))) {
            s.remove_suffix(1);
        }

        return s;
    }

    static void toLower_(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](uchar c) {
            return static_cast<char>(std::tolower(c));
        });
    }

    static std::vector<std::string_view> splitLines_(const std::string& s)
    {
        std::vector<std::string_view> lines{};
        std::string_view sv(s);
        std::string::size_type start = 0;

        for (std::string::size_type i = 0; i < s.size(); ++i) {
            if (s[i] == '\n') {
                lines.push_back(sv.substr(start, i - start));
                start = i + 1;
            }
        }

        // Remaining content after the last newline (or entire string if no
        // newline). Always push, even if empty, to match ObjC's
        // componentsSeparatedByCharactersInSet behavior
        lines.push_back(sv.substr(start));
        return lines;
    }

    static void
    replaceAll_(std::string& s, const std::string& from, const std::string& to)
    {
        if (from.empty()) return;

        size_t pos = 0;

        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    // --- Pattern matching ---

    // True if the line contains no lowercase ASCII letters (`isUpper` might be
    // slightly misleading, if `s` isn't letters only)
    static bool hasNoLowercase_(std::string_view s)
    {
        return std::none_of(s.begin(), s.end(), [](uchar c) {
            return std::islower(c);
        });
    }

    // True if every char is whitespace and the string has at least `min` chars.
    // `exactLength` = `true` requires the length to be exactly `min` (used to
    // distinguish the 2-space "empty line within dialogue" (exactly 2) from the
    // 2+ space "whitespace-only action line" check that follows it)
    static bool isWhitespaceOfLength_(
        std::string_view s,
        size_t min,
        bool exactLength = false)
    {
        if (exactLength ? s.size() != min : s.size() < min) return false;

        return std::all_of(s.begin(), s.end(), [](uchar c) {
            return std::isspace(c);
        });
    }

    // Matches ObjC: ^\s*\*\/\s*$  (line ends with */ after optional whitespace)
    static bool endsWithBoneyardClose_(std::string_view line)
    {
        auto pos = line.size();

        while (pos > 0 && std::isspace(static_cast<uchar>(line[pos - 1]))) {
            --pos;
        }

        return pos >= 2 && line[pos - 2] == '*' && line[pos - 1] == '/';
    }

    // Matches: INT, EXT, EST, or compound I/E INT/EXT variants (with
    // optional dots after each part), followed by '.', '-', or whitespace
    static bool matchesSceneHeading_(std::string_view line)
    {
        auto size = line.size();
        if (size < 4) return false; // Shortest: "INT." or "EXT."

        auto upper = [](char c) -> char {
            return static_cast<char>(std::toupper(static_cast<uchar>(c)));
        };

        auto at = [&](size_t i) -> char {
            return (i < size) ? upper(line[i]) : '\0';
        };

        // Returns true if `line[pos]` is a valid scene heading delimiter
        auto is_delim = [&](size_t pos) -> bool {
            if (pos >= size) return false;
            auto c = line[pos];
            return c == '.' || c == '-' || std::isspace(static_cast<uchar>(c));
        };

        // Try to consume a slash-compound prefix (I/E, INT/EXT, etc.)
        // and return the position after the full prefix, or 0 on failure
        auto try_compound = [&]() -> size_t {
            // Left side: "I" or "INT", with optional trailing dot
            size_t pos = 0;

            if (at(0) == 'I') {
                pos = 1;
                if (at(1) == 'N' && at(2) == 'T') pos = 3;
                if (at(pos) == '.') ++pos;
            } else {
                return 0;
            }

            if (at(pos) != '/') return 0;
            ++pos;

            // Right side: "E" or "EXT", with optional trailing dot
            if (at(pos) == 'E') {
                ++pos;
                if (at(pos) == 'X' && at(pos + 1) == 'T') pos += 2;
                if (at(pos) == '.') ++pos;
            } else {
                return 0;
            }

            return pos;
        };

        // Try compound first (I/E, INT/EXT, etc.)
        if (auto pos = try_compound(); pos > 0 && is_delim(pos)) return true;

        // Simple prefixes: INT, EXT, EST
        if (at(0) == 'I' && at(1) == 'N' && at(2) == 'T' && is_delim(3))
            return true;
        if (at(0) == 'E' && at(1) == 'X' && at(2) == 'T' && is_delim(3))
            return true;
        if (at(0) == 'E' && at(1) == 'S' && at(2) == 'T' && is_delim(3))
            return true;

        return false;
    }

    // No lowercase ASCII except for an optional trailing "(cont'd)"
    static bool matchesCharacterCue_(std::string_view line)
    {
        if (line.empty()) return false;

        auto end = line.size();
        constexpr auto suffix = "(cont'd)";
        constexpr size_t suffix_len = 8;

        if (end >= suffix_len) {
            auto match = true;

            for (size_t i = 0; i < suffix_len; ++i) {
                if (line[end - suffix_len + i] != suffix[i]) {
                    match = false;
                    break;
                }
            }

            if (match) end -= suffix_len;
        }

        if (end == 0) return false; // Can't be just "(cont'd)"

        for (size_t i = 0; i < end; ++i) {
            if (std::islower(static_cast<uchar>(line[i]))) return false;
        }

        return true;
    }

    // Extract #scene-number# from end of text (possibly followed by
    // trailing whitespace), modifying text in place
    static std::string extractSceneNumber_(std::string& text)
    {
        // Walk backwards past trailing whitespace
        auto end = text.size();

        while (end > 0 && std::isspace(static_cast<uchar>(text[end - 1]))) {
            --end;
        }

        if (end == 0 || text[end - 1] != '#') return {};

        size_t close = end - 1;

        // Scan backwards for the opening '#' (content must not contain
        // '#' or newline, matching the original [^\n#]*?)
        size_t open = close;

        while (open > 0) {
            --open;

            if (text[open] == '#') {
                auto number = text.substr(open + 1, close - open - 1);
                text.resize(open);
                trim_(text);

                return number;
            }

            if (text[open] == '\n') return {};
        }

        return {};
    }

    // --- Parsing ---

    void parse_(std::string source)
    {
        normalizeLineEndings_(source);
        trimLeading_(source);
        source += "\n\n";

        // Everything before the first blank line is a candidate title page
        auto blank_pos = source.find("\n\n");
        if (blank_pos == std::string::npos) return;

        auto top_of_document = source.substr(0, blank_pos);
        if (parseTitlePage_(top_of_document)) source.erase(0, blank_pos);

        // Prepend a newline so the first real line is "preceded by a blank"
        source.insert(source.begin(), '\n');
        parseBody_(source);
    }

    bool parseTitlePage_(const std::string& topOfDocument)
    {
        auto lines = splitLines_(topOfDocument);
        auto found = false;
        std::string open_key{};
        std::vector<std::string> open_values{};

        for (const auto& line : lines) {
            // Try to parse as a key-value line: key starts with
            // non-whitespace, contains no ':', followed by ':'
            size_t colon = std::string_view::npos;

            if (!line.empty() && !std::isspace(static_cast<uchar>(line[0]))) {
                colon = line.find(':');

                // Key must be at least one char before the colon, and
                // must not start at position 0 with the colon itself
                if (colon == 0) colon = std::string_view::npos;
            }

            auto is_directive = false;
            std::string parsed_key{};
            std::string parsed_value{};

            if (colon != std::string_view::npos) {
                parsed_key = std::string(line.substr(0, colon));

                // Check for non-whitespace content after the colon
                auto value_start = colon + 1;

                // Skip whitespace after colon
                while (value_start < line.size()
                       && std::isspace(static_cast<uchar>(line[value_start]))) {
                    ++value_start;
                }

                if (value_start < line.size()) {
                    // Inline: "Key: Value"
                    parsed_value = std::string(line.substr(value_start));
                } else {
                    // Directive: "Key:" (value on subsequent indented lines)
                    is_directive = true;
                }
            }

            if (line.empty() || is_directive) {
                found = true;

                if (!open_key.empty()) {
                    titlePage_.push_back({ open_key, open_values });
                }

                if (is_directive) {
                    toLower_(parsed_key);
                    open_key = std::move(parsed_key);
                } else {
                    open_key.clear();
                }

                if (open_key == "author") open_key = "authors";
                open_values.clear();

            } else if (!parsed_key.empty() && !parsed_value.empty()) {
                found = true;

                if (!open_key.empty()) {
                    titlePage_.push_back({ open_key, open_values });
                    open_key.clear();
                    open_values.clear();
                }

                toLower_(parsed_key);
                if (parsed_key == "author") parsed_key = "authors";

                titlePage_.push_back({ parsed_key, { parsed_value } });
                open_key.clear();
                open_values.clear();

            } else if (found) {
                open_values.push_back(trimmed_(line));
            }
        }

        if (!found) return false;

        if (!open_key.empty() && open_values.empty() && titlePage_.empty())
            return false;

        if (!open_key.empty()) {
            titlePage_.push_back({ open_key, open_values });
        }

        return true;
    }

    void parseBody_(const std::string& contents)
    {
        auto lines = splitLines_(contents);

        size_t newlines_before = 0;
        auto is_comment_block = false;
        auto is_inside_dialogue_block = false;
        std::string comment_text{};

        for (size_t i = 0; i < lines.size(); ++i) {
            const auto& line = lines[i];

            // Lyrics
            if (!line.empty() && line[0] == '~') {
                if (!elements_.empty()
                    && elements_.back().type == Element::Lyrics
                    && newlines_before > 0) {
                    // Insert a blank lyric spacer between stanzas
                    elements_.push_back({ Element::LyricsSpacer });
                }

                elements_.push_back(
                    { Element::Lyrics, std::string(line.substr(1)) });
                newlines_before = 0;

                continue;
            }

            // Forced action
            if (!line.empty() && line[0] == '!') {
                elements_.push_back(
                    { Element::Action, std::string(line.substr(1)) });
                newlines_before = 0;

                continue;
            }

            // Forced character
            if (!line.empty() && line[0] == '@') {
                elements_.push_back(
                    { Element::Character, std::string(line.substr(1)) });
                newlines_before = 0;
                is_inside_dialogue_block = true;

                continue;
            }

            // Empty lines within dialogue (exactly 2 whitespace chars)
            if (is_inside_dialogue_block
                && isWhitespaceOfLength_(line, 2, /*exactLength*/ true)) {
                newlines_before = 0;

                if (!elements_.empty()
                    && elements_.back().type == Element::Dialogue) {
                    elements_.back().text += '\n';
                    elements_.back().text += line;
                } else {
                    elements_.push_back(
                        { Element::Dialogue, std::string(line) });
                }

                continue;
            }

            // Whitespace-only lines (2+ chars)
            if (isWhitespaceOfLength_(line, 2)) {
                elements_.push_back({ Element::Action, std::string(line) });
                newlines_before = 0;

                continue;
            }

            // Blank line
            if (line.empty() && !is_comment_block) {
                is_inside_dialogue_block = false;
                ++newlines_before;

                continue;
            }

            // Open boneyard
            if (line.size() >= 2 && line[0] == '/' && line[1] == '*') {
                if (endsWithBoneyardClose_(line)) {
                    // Single-line boneyard ("/* comment */")
                    std::string text(line);
                    replaceAll_(text, "/*", "");
                    replaceAll_(text, "*/", "");
                    trim_(text);
                    elements_.push_back({ Element::Boneyard, text });

                    is_comment_block = false;
                    newlines_before = 0;

                } else {
                    is_comment_block = true;
                    comment_text += '\n';
                }

                continue;
            }

            // Close boneyard
            // TODO: This fires even when is_comment_block is false. A normal
            // line ending in "*/" outside a boneyard context would be
            // misclassified. The ObjC original has the same bug
            if (endsWithBoneyardClose_(line)) {
                std::string text(line);
                replaceAll_(text, "*/", "");

                // ObjC only appends if remaining text is blank/whitespace
                // TODO: This silently drops non-whitespace content on the
                // closing line of a multi-line boneyard
                if (text.empty()
                    || std::all_of(text.begin(), text.end(), [](uchar c) {
                           return std::isspace(c);
                       })) {
                    trim_(text);
                    comment_text += text;
                }

                is_comment_block = false;

                elements_.push_back({ Element::Boneyard, comment_text });
                comment_text.clear();
                newlines_before = 0;

                continue;
            }

            // Inside boneyard
            if (is_comment_block) {
                comment_text += std::string(line);
                comment_text += '\n';

                continue;
            }

            // Page break (3+ consecutive '=' signs, optional trailing
            // whitespace)
            if (line.size() >= 3 && line[0] == '=') {
                size_t pos = 0;

                while (pos < line.size() && line[pos] == '=') {
                    ++pos;
                }

                if (pos >= 3
                    && std::all_of(line.begin() + pos, line.end(), [](uchar c) {
                           return std::isspace(c);
                       })) {
                    elements_.push_back(
                        { Element::PageBreak, std::string(line) });
                    newlines_before = 0;

                    continue;
                }
            }

            // Synopsis ('=' at start, but not page break)
            {
                auto trimmed = trimmedView_(line);

                if (!trimmed.empty() && trimmed[0] == '=') {
                    // Strip leading whitespace + the single '='
                    size_t pos = 0;

                    while (pos < line.size()
                           && std::isspace(static_cast<uchar>(line[pos]))) {
                        ++pos;
                    }

                    if (pos < line.size() && line[pos] == '=') ++pos;

                    // Skip whitespace after the '=', too
                    while (pos < line.size()
                           && std::isspace(static_cast<uchar>(line[pos]))) {
                        ++pos;
                    }

                    elements_.push_back(
                        { Element::Synopsis, std::string(line.substr(pos)) });

                    continue;
                }
            }

            // Comment ("[[text]]" on a single line)
            if (newlines_before > 0) {
                auto trimmed = trimmedView_(line);

                if (trimmed.size() >= 4 && trimmed[0] == '['
                    && trimmed[1] == '[' && trimmed[trimmed.size() - 1] == ']'
                    && trimmed[trimmed.size() - 2] == ']') {
                    auto inner =
                        std::string(trimmed.substr(2, trimmed.size() - 4));

                    // ObjC requires no ']' inside the content.
                    if (inner.find(']') == std::string::npos) {
                        trim_(inner);
                        elements_.push_back({ Element::Comment, inner });
                        continue;
                    }
                }
            }

            // Section heading ('#' at start)
            {
                auto trimmed = trimmedView_(line);
                if (!trimmed.empty() && trimmed[0] == '#') {
                    newlines_before = 0;

                    // Count depth by number of '#' characters (fixes ObjC bug
                    // where depth included leading whitespace)
                    auto depth = 0;
                    size_t pos = 0;

                    while (pos < trimmed.size() && trimmed[pos] == '#') {
                        ++depth;
                        ++pos;
                    }

                    std::string text(trimmed.substr(pos));
                    trim_(text);
                    if (text.empty()) continue; // malformed

                    Element element{ Element::SectionHeading, text };
                    element.sectionDepth = depth;
                    elements_.push_back(element);

                    continue;
                }
            }

            // Forced scene heading ('.' but not '..')
            if (line.size() > 1 && line[0] == '.' && line[1] != '.') {
                newlines_before = 0;

                std::string text(line.substr(1));
                trim_(text);
                auto scene_number = extractSceneNumber_(text);

                Element element{ Element::SceneHeading, text };
                if (!scene_number.empty()) element.sceneNumber = scene_number;
                elements_.push_back(element);

                continue;
            }

            // Scene heading (INT./EXT./EST. etc.)
            if (newlines_before > 0 && matchesSceneHeading_(line)) {
                newlines_before = 0;

                std::string text(line);
                auto scene_number = extractSceneNumber_(text);

                Element element{ Element::SceneHeading, text };
                if (!scene_number.empty()) element.sceneNumber = scene_number;
                elements_.push_back(element);

                continue;
            }

            // Transition (all-caps ending in "TO:")
            if (newlines_before > 0 && hasNoLowercase_(line)
                && line.ends_with("TO:")) {
                newlines_before = 0;
                elements_.push_back({ Element::Transition, std::string(line) });
                continue;
            }

            // Known transitions
            {
                auto trimmed_leading = trimmedLeadingView_(line);

                if (newlines_before > 0
                    && (trimmed_leading == "FADE OUT."
                        || trimmed_leading == "CUT TO BLACK."
                        || trimmed_leading == "FADE TO BLACK.")) {
                    newlines_before = 0;
                    elements_.push_back(
                        { Element::Transition, std::string(line) });

                    continue;
                }
            }

            // Forced transition ('>' / centered text '>...<')
            if (!line.empty() && line[0] == '>') {
                if (line.size() > 1 && line.back() == '<') {
                    // Centered text (strip the '>' and '<' markers)
                    std::string text(line.substr(1));
                    trim_(text);

                    if (!text.empty() && text.back() == '<') {
                        text.pop_back();
                        trim_(text);
                    }

                    Element element{ Element::Action, text };
                    element.textCentered = true;
                    elements_.push_back(element);

                } else {
                    // Forced transition
                    std::string text(line.substr(1));
                    trim_(text);
                    elements_.push_back({ Element::Transition, text });
                }

                newlines_before = 0;
                continue;
            }

            // Character cue
            if (newlines_before > 0 && matchesCharacterCue_(line)) {
                // Only a character if next line is non-empty (so peek)
                if (i + 1 < lines.size() && !lines[i + 1].empty()) {
                    newlines_before = 0;

                    std::string text(line);
                    auto is_dual = false;

                    // Dual dialogue (trailing '^')
                    auto trimmed_line = trimmedView_(line);

                    if (trimmed_line.ends_with('^')) {
                        is_dual = true;

                        // Strip trailing \s*\^\s*
                        auto end = text.size();

                        while (end > 0
                               && std::isspace(
                                   static_cast<uchar>(text[end - 1]))) {
                            --end;
                        }

                        if (end > 0 && text[end - 1] == '^') --end;

                        while (end > 0
                               && std::isspace(
                                   static_cast<uchar>(text[end - 1]))) {
                            --end;
                        }

                        text.resize(end);

                        // Walk back and mark the previous Character as dual too
                        for (auto it = elements_.rbegin();
                             it != elements_.rend();
                             ++it) {
                            if (it->type == Element::Character) {
                                it->isDualDialogueCharacter = true;
                                break;
                            }
                        }
                    }

                    Element element{ Element::Character, std::move(text) };
                    element.isDualDialogueCharacter = is_dual;
                    elements_.push_back(std::move(element));

                    is_inside_dialogue_block = true;

                    continue;
                }
            }

            // Dialogue/Parenthetical
            if (is_inside_dialogue_block) {
                // Parenthetical (starts with '(' after optional whitespace)
                auto trimmed_line = trimmedLeadingView_(line);

                if (newlines_before == 0 && !trimmed_line.empty()
                    && trimmed_line[0] == '(') {
                    elements_.push_back(
                        { Element::Parenthetical, std::string(line) });
                } else {
                    // Merge consecutive dialogue lines into one element
                    if (!elements_.empty()
                        && elements_.back().type == Element::Dialogue) {
                        elements_.back().text += '\n';
                        elements_.back().text += line;
                    } else {
                        elements_.push_back(
                            { Element::Dialogue, std::string(line) });
                    }
                }

                continue;
            }

            // Continuation (merge with previous element)
            if (newlines_before == 0 && !elements_.empty()) {
                auto& prev = elements_.back();

                // A scene heading followed immediately by text (no blank line)
                // was never a real scene heading, so demote to action
                if (prev.type == Element::SceneHeading) {
                    prev.type = Element::Action;
                }

                prev.text += '\n';
                prev.text += line;
                newlines_before = 0;

                continue;
            }

            // Action (fallback)
            elements_.push_back({ Element::Action, trimmed_(line) });
            newlines_before = 0;
        }
    }
};

// TODO: Do we need to make sure we don't append "CONT'D" when already exists on
// the end of character name?
class Paginator
{
public:
    // (text, maxWidthPx, lineHeightPx) -> heightPx
    using MeasureTextFn = std::function<
        int(const std::string& text, int maxWidth, int lineHeight)>;

    explicit Paginator(
        const std::vector<Element>& elements,
        MeasureTextFn measureTextFn,
        int pageWidthPx = 612,
        int pageHeightPx = 792)
        : elements_(elements)
        , measureTextFn_(std::move(measureTextFn))
        , pageWidth_(pageWidthPx)
        , pageHeight_(pageHeightPx)
    {
        paginate_();
    }

    size_t numberOfPages() const noexcept { return pages_.size(); }

    const std::vector<Element>& pageAt(size_t index) const
    {
        static const std::vector<Element> empty{};
        return (index < pages_.size()) ? pages_[index] : empty;
    }

    const std::vector<std::vector<Element>>& pages() const { return pages_; }

private:
    const std::vector<Element>& elements_;
    MeasureTextFn measureTextFn_;
    int pageWidth_;
    int pageHeight_;

    std::vector<std::vector<Element>> pages_{};

    // --- Element metrics (screenplay formatting constants) ---

    // Vertical space before element (in multiples of line height)
    static int spaceBefore_(Element::Type type)
    {
        switch (type) {
        case Element::SceneHeading:
            return 2;
        case Element::Action:
        case Element::Character:
        case Element::Transition:
            return 1;
        default:
            return 0;
        }
    }

    // Column width for element type (in pixels)
    static int widthFor_(Element::Type type)
    {
        switch (type) {
        case Element::Action:
        case Element::SceneHeading:
        case Element::Transition:
            return 430;
        case Element::Character:
        case Element::Dialogue:
            return 250;
        case Element::Parenthetical:
            return 212;
        default:
            return 430;
        }
    }

    // Text height via the caller-provided callback
    int measureHeight_(
        const std::string& text,
        Element::Type type,
        int lineHeight) const
    {
        return measureTextFn_(text, widthFor_(type), lineHeight);
    }

    // --- Helpers ---

    static void flushTempElements_(
        std::vector<Element>& currentPage,
        std::vector<Element>& tempElements)
    {
        for (auto& element : tempElements) {
            currentPage.push_back(std::move(element));
        }

        tempElements.clear();
    }

    static Element makeElement_(Element::Type type, const std::string& text)
    {
        Element element{};
        element.type = type;
        element.text = text;

        return element;
    }

    // --- Pattern matching ---

    // Split dialogue text at sentence boundaries (.?! followed by whitespace)
    static std::vector<std::string> splitSentences_(const std::string& text)
    {
        std::vector<std::string> sentences{};
        size_t start = 0;

        for (size_t i = 0; i < text.size(); ++i) {
            auto c = text[i];

            if (c == '.' || c == '?' || c == '!') {
                // Consume consecutive punctuation (.?!)
                while (i + 1 < text.size()
                       && (text[i + 1] == '.' || text[i + 1] == '?'
                           || text[i + 1] == '!')) {
                    ++i;
                }

                // Consume trailing whitespace
                while (
                    i + 1 < text.size()
                    && std::isspace(static_cast<unsigned char>(text[i + 1]))) {
                    ++i;
                }

                sentences.push_back(text.substr(start, i + 1 - start));
                start = i + 1;
            }
        }

        if (sentences.empty()) sentences.push_back(text);

        return sentences;
    }

    // --- Pagination ---

    void paginate_()
    {
        constexpr auto one_inch_px = 72;
        auto max_page_height =
            pageHeight_ - static_cast<int>(std::round(one_inch_px * 2.0));
        auto line_height = 12; // Courier 12pt
        auto block_height = 0;
        auto current_y = 0;
        auto previous_dual_height = -1;
        std::vector<Element> current_page{};
        std::vector<Element> temp_elements{};

        for (size_t i = 0; i < elements_.size(); ++i) {
            const auto& element = elements_[i];

            // --- Page break (flush and start new page) ---
            if (element.type == Element::PageBreak) {
                flushTempElements_(current_page, temp_elements);
                current_page.push_back(element);
                pages_.push_back(std::move(current_page));
                current_page.clear();
                current_y = 0;

                continue;
            }

            // --- Measure this element ---
            auto space = spaceBefore_(element.type) * line_height;
            auto height =
                measureHeight_(element.text, element.type, line_height);

            if (height <= 0) continue;

            block_height += height;

            // Only add space before if not at the top of the page
            if (!current_page.empty()) block_height += space;

            // --- Scene heading (keep with following element) ---
            if (element.type == Element::SceneHeading
                && i + 1 < elements_.size()) {
                const auto& next = elements_[i + 1];
                auto next_height =
                    measureHeight_(next.text, next.type, line_height);

                // If heading + next element won't fit, force a break before
                if ((block_height + current_y + next_height >= max_page_height)
                    && (next_height >= line_height)) {
                    temp_elements.push_back(
                        makeElement_(Element::PageBreak, ""));
                }

                temp_elements.push_back(element);
                continue;
            }

            // --- Character cue (accumulate entire dialogue block) ---
            if (element.type == Element::Character
                && i + 1 < elements_.size()) {

                size_t j = i + 1;
                temp_elements.push_back(element);
                auto is_end = false;

                while (!is_end) {
                    if (j < elements_.size()) {
                        const auto& next = elements_[j];

                        if (next.type == Element::Dialogue
                            || next.type == Element::Parenthetical) {
                            block_height += measureHeight_(
                                next.text,
                                next.type,
                                line_height);
                            temp_elements.push_back(next);

                            ++j;

                        } else {
                            break;
                        }
                    } else {
                        is_end = true;
                    }
                }

                // Advance i past the elements we consumed. j points to the
                // first unconsumed element (or past end). The loop will ++i, so
                // set to j - 1
                i = j - 1;

                // Dual dialogue height adjustment
                if (element.isDualDialogueCharacter
                    && previous_dual_height < 0) {
                    previous_dual_height = block_height;
                } else if (element.isDualDialogueCharacter) {
                    block_height =
                        std::abs(previous_dual_height - block_height);
                    previous_dual_height = -1;
                }

            } else {
                temp_elements.push_back(element);
            }

            // --- Check if we've overflowed the page ---
            auto total_height = block_height + current_y;

            if (total_height > max_page_height) {
                // Attempt to split a dialogue block across pages
                if (!temp_elements.empty()
                    && temp_elements[0].type == Element::Character
                    && (total_height - max_page_height) >= line_height * 4) {
                    splitDialogueBlock_(
                        max_page_height,
                        current_y,
                        line_height,
                        current_page,
                        temp_elements,
                        block_height);

                    current_y = block_height;

                } else {
                    // Simple overflow (push current page and start fresh)
                    pages_.push_back(std::move(current_page));
                    current_page.clear();

                    current_y = block_height - space;
                    block_height = 0;
                }
            } else {
                current_y = total_height;
            }

            block_height = 0;

            // Flush temp to current page
            flushTempElements_(current_page, temp_elements);
        }

        // Flush any remaining elements
        flushTempElements_(current_page, temp_elements);
        if (!current_page.empty()) pages_.push_back(std::move(current_page));
    }

    // --- Dialogue block splitting ---

    void splitDialogueBlock_(
        int maxPageHeight,
        int currentY,
        int lineHeight,
        std::vector<Element>& currentPage,
        std::vector<Element>& tempElements,
        int& blockHeight)
    {
        auto max_temp = static_cast<int>(tempElements.size());

        // Find which element in the block spills over the page
        auto block_index = -1;
        auto partial_height = 0;
        auto page_overflow = (blockHeight + currentY) - maxPageHeight;

        while (partial_height < page_overflow && block_index < max_temp - 1) {
            ++block_index;

            auto& e = tempElements[block_index];
            auto h = measureHeight_(e.text, e.type, lineHeight);
            auto s = spaceBefore_(e.type) * lineHeight;

            partial_height += h + s;
        }

        if (block_index <= 0) {
            // Can't split meaningfully, just push to next page
            pages_.push_back(std::move(currentPage));
            currentPage.clear();
            blockHeight = 0;

            return;
        }

        auto& spiller = tempElements[block_index];

        if (spiller.type == Element::Parenthetical) {
            // Break before the parenthetical (only if not index 1)
            if (block_index > 1) {
                splitAtParenthetical_(
                    block_index,
                    max_temp,
                    lineHeight,
                    currentPage,
                    tempElements,
                    blockHeight);
            }
        } else {
            // Dialogue spills (try splitting at sentence boundaries)
            splitAtDialogue_(
                block_index,
                max_temp,
                maxPageHeight,
                currentY,
                lineHeight,
                currentPage,
                tempElements,
                blockHeight);
        }
    }

    void splitAtParenthetical_(
        int blockIndex,
        int maxTemp,
        int lineHeight,
        std::vector<Element>& currentPage,
        std::vector<Element>& tempElements,
        int& blockHeight)
    {
        // Add elements before the parenthetical to current page
        for (auto z = 0; z < blockIndex; ++z) {
            currentPage.push_back(tempElements[z]);
        }

        // (MORE) at bottom of page
        currentPage.push_back(makeElement_(Element::Character, "(MORE)"));

        // Close the page
        pages_.push_back(std::move(currentPage));
        currentPage.clear();

        // CHARACTER (CONT'D) at top of next page
        auto& contd = tempElements[0];
        contd.text += " (CONT'D)";
        blockHeight = measureHeight_(contd.text, contd.type, lineHeight);
        currentPage.push_back(contd);

        // Add remaining elements (parenthetical onward)
        for (auto z = blockIndex; z < maxTemp; ++z) {
            currentPage.push_back(tempElements[z]);
            blockHeight += measureHeight_(
                tempElements[z].text,
                tempElements[z].type,
                lineHeight);
        }

        tempElements.clear();
    }

    void splitAtDialogue_(
        int blockIndex,
        int maxTemp,
        int maxPageHeight,
        int currentY,
        int lineHeight,
        std::vector<Element>& currentPage,
        std::vector<Element>& tempElements,
        int& blockHeight)
    {
        auto distance_to_bottom = maxPageHeight - currentY - (lineHeight * 2);

        // Not enough room for a meaningful split (push whole block to next
        // page)
        if (distance_to_bottom < lineHeight * 5) {
            pages_.push_back(std::move(currentPage));
            currentPage.clear();
            blockHeight = 0;

            return;
        }

        // Measure height of everything before the spilling dialogue
        auto height_before_dialogue = 0;
        for (int z = 0; z < blockIndex; ++z) {
            height_before_dialogue += spaceBefore_(tempElements[z].type);
            height_before_dialogue += measureHeight_(
                tempElements[z].text,
                tempElements[z].type,
                lineHeight);
        }

        // Split the spilling dialogue at sentence boundaries
        auto& spiller = tempElements[blockIndex];
        auto sentences = splitSentences_(spiller.text);
        auto max_sentences = static_cast<int>(sentences.size());

        auto dialogue_height = height_before_dialogue;
        auto sentence_index = -1;
        std::string dialogue_before_break{};

        while (dialogue_height < distance_to_bottom
               && sentence_index < max_sentences - 1) {
            ++sentence_index;

            auto candidate = dialogue_before_break + sentences[sentence_index];
            auto h = measureHeight_(candidate, Element::Dialogue, lineHeight);

            dialogue_height = h;

            if (dialogue_height < distance_to_bottom)
                dialogue_before_break = candidate;
        }

        // Build the pre-break dialogue element
        auto pre_break = makeElement_(Element::Dialogue, dialogue_before_break);

        if (!pre_break.text.empty()) {
            // Add elements up to the spilling one + partial dialogue
            for (auto z = 0; z < blockIndex; ++z) {
                currentPage.push_back(tempElements[z]);
            }

            currentPage.push_back(pre_break);
            currentPage.push_back(makeElement_(Element::Character, "(MORE)"));

            pages_.push_back(std::move(currentPage));
            currentPage.clear();

        } else {
            // Nothing fit before the break, so push page as-is
            pages_.push_back(std::move(currentPage));
            currentPage.clear();

            // Carry over elements between character cue and spiller
            for (auto z = 1; z < blockIndex; ++z) {
                currentPage.push_back(tempElements[z]);
            }
        }

        // --- Next page ---
        blockHeight = 0;

        // CHARACTER (CONT'D)
        auto contd = makeElement_(
            Element::Character,
            tempElements[0].text + " (CONT'D)");
        blockHeight += measureHeight_(contd.text, contd.type, lineHeight);
        currentPage.push_back(contd);

        // Post-break dialogue (remaining sentences)
        if (sentence_index < 0) sentence_index = 0;

        std::string dialogue_after_break{};
        for (auto z = sentence_index; z < max_sentences; ++z) {
            dialogue_after_break += sentences[z];
        }

        auto post_break = makeElement_(Element::Dialogue, dialogue_after_break);
        blockHeight +=
            measureHeight_(post_break.text, post_break.type, lineHeight);
        currentPage.push_back(post_break);

        // Add remaining elements after the spiller
        for (auto z = blockIndex + 1; z < maxTemp; ++z) {
            currentPage.push_back(tempElements[z]);
            blockHeight += measureHeight_(
                tempElements[z].text,
                tempElements[z].type,
                lineHeight);
        }

        tempElements.clear();
    }
};

class Renderer
{
public:
    explicit Renderer(
        const Parser& parser,
        Paginator::MeasureTextFn measureTextFn = nullptr)
        : parser_(parser)
        , measureTextFn_(std::move(measureTextFn))
    {
    }

    std::string html() const
    {
        std::string out{};

        out += "<!DOCTYPE html>\n";
        out += "<html>\n";
        out += "<head>\n";
        out += "<style type='text/css'>\n";
        out += CSS_;
        out += "</style>\n";
        out += "</head>\n";
        out += "<body>\n<article>\n<section>\n";
        renderTitlePage_(out);
        renderBody_(out);
        out += "</section>\n</article>\n</body>\n";
        out += "</html>";

        return out;
    }

private:
    const Parser& parser_;
    Paginator::MeasureTextFn measureTextFn_;

    // --- HTML ---

    static void escapeHtml_(const std::string& text, std::string& out)
    {
        for (auto c : text) {
            switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            default:
                out += c;
                break;
            }
        }
    }

    static const char* cssClass_(Element::Type type)
    {
        switch (type) {
        case Element::SceneHeading:
            return "scene-heading";
        case Element::Action:
            return "action";
        case Element::Character:
            return "character";
        case Element::Dialogue:
            return "dialogue";
        case Element::Parenthetical:
            return "parenthetical";
        case Element::Transition:
            return "transition";
        case Element::Lyrics:
        case Element::LyricsSpacer:
            return "lyrics";
        case Element::PageBreak:
            return "page-break";
        case Element::Boneyard:
            return "boneyard";
        case Element::Comment:
            return "comment";
        case Element::SectionHeading:
            return "section-heading";
        case Element::Synopsis:
            return "synopsis";

        default:
        case Element::Null:
            return "NULL";
        }
    }

    // --- Pattern matching ---

    static bool
    matchesAt_(const std::string& text, size_t pos, const char* prefix)
    {
        for (size_t i = 0; prefix[i] != '\0'; ++i) {
            if (pos + i >= text.size() || text[pos + i] != prefix[i]) {
                return false;
            }
        }

        return true;
    }

    static bool isValidContent_(
        const std::string& text,
        size_t start,
        size_t end,
        bool rejectUnderscore)
    {
        if (start >= end) return false; // Must be non-empty (matches [^<>]+?)

        for (size_t i = start; i < end; ++i) {
            if (text[i] == '<' || text[i] == '>') return false;
            if (rejectUnderscore && text[i] == '_') return false;
        }

        return true;
    }

    // Scans `text` for opener/closer marker pairs and replaces them with HTML
    // tags. `marker2` is an alternate form (e.g., "***_" vs "_***") or nullptr
    // if only one form exists
    static void replaceMarkerPairs_(
        std::string& text,
        const char* marker1,
        const char* marker2,
        const char* openTag,
        const char* closeTag,
        bool rejectUnderscore = false)
    {
        // strlen
        auto len = [](const char* s) {
            size_t n = 0;

            while (s[n]) {
                ++n;
            }

            return n;
        };

        auto m1_len = len(marker1);
        auto m2_len = marker2 ? len(marker2) : size_t{ 0 };
        auto open_len = len(openTag);
        auto close_len = len(closeTag);

        size_t pos = 0;

        while (pos < text.size()) {
            size_t opener_len = 0;

            if (matchesAt_(text, pos, marker1)) {
                opener_len = m1_len;
            } else if (marker2 && matchesAt_(text, pos, marker2)) {
                opener_len = m2_len;
            }

            if (opener_len == 0) {
                ++pos;
                continue;
            }

            size_t content_start = pos + opener_len;
            auto found = false;

            for (size_t j = content_start; j < text.size(); ++j) {
                size_t closer_len = 0;

                if (matchesAt_(text, j, marker1)) {
                    closer_len = m1_len;
                } else if (marker2 && matchesAt_(text, j, marker2)) {
                    closer_len = m2_len;
                }

                if (closer_len > 0
                    && isValidContent_(
                        text,
                        content_start,
                        j,
                        rejectUnderscore)) {
                    auto content =
                        text.substr(content_start, j - content_start);
                    auto replacement =
                        std::string(openTag) + content + closeTag;

                    text.replace(pos, (j + closer_len) - pos, replacement);

                    pos += open_len + content.size() + close_len;
                    found = true;

                    break;
                }

                // Content can't cross HTML tag boundaries
                if (text[j] == '<' || text[j] == '>') break;
            }

            if (!found) pos += opener_len;
        }
    }

    static void stripNotes_(std::string& text)
    {
        size_t pos = 0;

        while ((pos = text.find("[[", pos)) != std::string::npos) {
            auto end = text.find("]]", pos + 2);

            if (end != std::string::npos) {
                text.erase(pos, (end + 2) - pos);
            } else {
                pos += 2;
            }
        }
    }

    // --- Inline formatting ---

    static void applyInlineFormatting_(const std::string& raw, std::string& out)
    {
        std::string text{};
        escapeHtml_(raw, text);

        // Replace newlines with <br>
        {
            std::string tmp{};
            tmp.reserve(text.size());

            for (auto& c : text) {
                (c == '\n') ? tmp += "<br>" : tmp += c;
            }

            text = std::move(tmp);
        }

        // Most specific markers first to prevent partial matches consuming
        // markers needed by more complex combinations

        replaceMarkerPairs_(
            text,
            "_***",
            "***_",
            "<strong><em><u>",
            "</u></em></strong>");
        replaceMarkerPairs_(
            text,
            "***",
            nullptr,
            "<strong><em>",
            "</em></strong>");
        replaceMarkerPairs_(text, "_**", "**_", "<strong><u>", "</u></strong>");
        replaceMarkerPairs_(text, "_*", "*_", "<em><u>", "</u></em>");
        replaceMarkerPairs_(text, "**", nullptr, "<strong>", "</strong>");
        replaceMarkerPairs_(text, "*", nullptr, "<em>", "</em>");
        replaceMarkerPairs_(text, "_", nullptr, "<u>", "</u>", true);

        stripNotes_(text);

        out += text;
    }

    // --- Rendering ---

    void renderTitlePage_(std::string& out) const
    {
        const auto& entries = parser_.titlePage();
        if (entries.empty()) return;

        std::map<std::string, std::string> fields{};

        for (const auto& entry : entries) {
            std::string joined{};

            for (size_t i = 0; i < entry.values.size(); ++i) {
                if (i > 0) joined += "<br>";
                escapeHtml_(entry.values[i], joined);
            }

            fields[entry.key] = joined;
        }

        out += "<div id='script-title'>\n";

        auto field = [&](const char* key,
                         const char* cls,
                         const char* fallback = nullptr) {
            if (auto it = fields.find(key); it != fields.end()) {
                out += "<p class='";
                out += cls;
                out += "'>";
                out += it->second;
                out += "</p>\n";

            } else if (fallback) {
                out += "<p class='";
                out += cls;
                out += "'>";
                out += fallback;
                out += "</p>\n";
            }
        };

        field("title", "title", "Untitled");

        if (fields.count("credit") || fields.count("authors")) {
            field("credit", "credit", "written by");
            field("authors", "authors", "Anonymous");
        }

        field("source", "source");
        field("draft date", "draft-date");
        field("contact", "contact");
        field("notes", "notes");
        field("copyright", "copyright");

        out += "</div>\n";
    }

    // Returns HTML for a single element, updating dual dialogue state
    void renderElement_(
        const Element& element,
        int& dualCount,
        std::string& out) const
    {
        // Boneyard, Comment, Synopsis, and SectionHeading are not rendered in
        // screenplay output per the Fountain spec. SectionHeading depth is
        // organizational metadata for outline/navigator features
        constexpr auto is_ignored = [](Element::Type t) {
            return t == Element::Boneyard || t == Element::Comment
                   || t == Element::Synopsis || t == Element::SectionHeading;
        };

        if (is_ignored(element.type)) return;

        if (element.type == Element::PageBreak) {
            out += "</section>\n<section>\n";
            return;
        }

        // Just a blank paragraph
        if (element.type == Element::LyricsSpacer) {
            out += "<p class='lyrics'>&nbsp;</p>\n";
            return;
        }

        // Dual dialogue handling
        if (element.type == Element::Character
            && element.isDualDialogueCharacter) {
            ++dualCount;

            if (dualCount == 1) {
                out += "<div class='dual-dialogue'>\n";
                out += "<div class='dual-dialogue-left'>\n";
            } else if (dualCount == 2) {
                out += "</div>\n<div class='dual-dialogue-right'>\n";
            }
        }

        // Close dual dialogue when we leave the dialogue block
        constexpr auto is_dialogue_type = [](Element::Type t) {
            return t == Element::Character || t == Element::Dialogue
                   || t == Element::Parenthetical;
        };

        if (dualCount >= 2 && !is_dialogue_type(element.type)) {
            dualCount = 0;
            out += "</div>\n</div>\n";
        }

        // Build the text content into a temporary (inline formatting needs a
        // mutable working copy)
        auto before_text = out.size();

        if (element.type == Element::SceneHeading
            && !element.sceneNumber.empty()) {
            out += "<span class='scene-number-left'>";
            escapeHtml_(element.sceneNumber, out);

            out += "</span>";
            applyInlineFormatting_(element.text, out);
            out += "<span class='scene-number-right'>";

            escapeHtml_(element.sceneNumber, out);
            out += "</span>";

        } else {
            applyInlineFormatting_(element.text, out);
        }

        // If inline formatting produced nothing, undo any dual-dialogue markup
        // we may have just appended (preserve original behavior where empty
        // text skips the <p> tag)
        if (out.size() == before_text) return;

        // Wrap in <p> with CSS class. We need to insert the opening tag before
        // the text content we just appended
        auto css_class = std::string(cssClass_(element.type));
        if (element.textCentered) css_class += " center";

        auto open_tag = "<p class='" + css_class + "'>";
        out.insert(before_text, open_tag);
        out += "</p>\n";
    }

    void renderBody_(std::string& out) const
    {
        auto dual_count = 0;

        if (measureTextFn_) {
            Paginator paginator(parser_.elements(), measureTextFn_);

            for (size_t p = 0; p < paginator.numberOfPages(); ++p) {
                out += "<p class='page-break'>";
                out += std::to_string(p + 1);
                out += ".</p>\n";

                for (const auto& element : paginator.pageAt(p)) {
                    renderElement_(element, dual_count, out);
                }
            }

        } else {
            for (const auto& element : parser_.elements()) {
                renderElement_(element, dual_count, out);
            }
        }

        // Close any dangling dual dialogue
        if (dual_count >= 2) out += "</div>\n</div>\n";
    }

    static constexpr const char* CSS_ = R"CSS(
* {
    -webkit-touch-callout: none;
    -webkit-user-select: none;
}
html {
    margin: 0;
    padding: 0;
}
body {
    background-color: #fff;
    color: #3e3e3e;
    font: 14px/1.3em 'Courier Prime', 'Courier', monospace;
    padding: 0;
    margin: 0;
}
article {
    padding: 40px 0;
    margin: 0;
}
section {
    padding: 0 0 0 40px;
    width: 465px;
    margin-right: auto;
    margin-left: auto;
}
p {
    margin: 1.3em auto;
    word-wrap: break-word;
    padding: 0 10px;
}
p.page-break {
    text-align: right;
    border-top: 1px solid #ccc;
    padding-top: 20px;
    margin-top: 20px;
}
body > p:first-child {
    margin-top: 0;
}
.scene-heading, .transition, .character {
    text-transform: uppercase;
}
.transition {
    text-align: right;
}
.character {
    margin: 1.3em auto 0;
    width: 180px;
}
.dialogue {
    margin: 0 auto;
    width: 310px;
}
.parenthetical {
    margin: 0 auto;
    width: 250px;
}
.scene-heading {
    margin-top: 2.6em;
    font-weight: bold;
    position: relative;
    padding-right: 40px;
}
.scene-number-left {
    float: left;
    margin-left: -50px;
}
.scene-number-right {
    position: absolute;
    right: 0;
    top: 0;
}
#script-title {
    overflow: hidden;
    display: block;
    /*padding-bottom: 2.6em;
    margin-bottom: 2.6em;*/
    /*TODO: ^ May want to leave spacing like this a display concern */
    /*TODO: Additionally, may want to provide two levels of CSS - essential
    and optional and/or a callback for providing CSS to renderer */
}
#script-title .title {
    text-align: center;
    margin: 1.3em 0;
    text-decoration: underline;
    font-weight: bold;
    text-transform: uppercase;
}
#script-title .credit {
    text-align: center;
}
#script-title .authors {
    text-align: center;
}
#script-title .source {
    text-align: center;
    padding-top: 1.3em;
}
#script-title .notes {
    padding-top: 2.6em;
    white-space: pre-line;
}
.center {
    text-align: center !important;
}
hr {
    height: 0px;
    border: none;
    border-bottom: 1px solid #ccc;
}
.dual-dialogue {
    overflow: hidden;
}
.dual-dialogue .dual-dialogue-left,
.dual-dialogue .dual-dialogue-right {
    width: 228px;
    float: left;
}
.dual-dialogue p {
    width: auto;
}
.dual-dialogue .character {
    padding-left: 40px;
}
.dual-dialogue .parenthetical {
    padding-left: 40px;
}
.lyrics {
    font-style: italic;
}
)CSS";
};

// Qt test parse:

// auto data = Io::read("path/to/file.fountain");
// auto parser = Fountain::Parser(QString(data).toStdString());
//
// for (const auto& t : parser.titlePage()) {
//     std::ostringstream oss{};
//     oss << t;
//     qDebug().noquote() << QString::fromStdString(oss.str());
// }
//
// for (const auto& e : parser.elements()) {
//     std::ostringstream oss{};
//     oss << e;
//     qDebug().noquote() << QString::fromStdString(oss.str());
// }

} // namespace Fountain
