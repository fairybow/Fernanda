#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

/// TODO MU: Draft (right now, keep one file)

// TODO: Move to own repo?
namespace Fountain {

// Translated from:
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FNElement.h
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FastFountainParser.h
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FNHTMLScript.h)

// TODO: Fountain::Paginator (we will need a printing layout; see
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FNPaginator.h)

struct TitleEntry
{
    std::string key{};
    std::vector<std::string> values{};

    friend std::ostream& operator<<(std::ostream& out, const TitleEntry& entry)
    {
        out << "[ title key: " << entry.key << "; values:";
        for (const auto& v : entry.values)
            out << " " << v;
        return out << " ]";
    }
};

// TODO (maybe): If we can always trim excess whitespace on either side without
// losing any meaning, then we can ditch all of the trim_ calls when adding
// elements and just add a ctor that trims
struct Element
{
    enum Type
    {
        Null = 0, // Shouldn't happen
        SceneHeading,
        Action,
        Character,
        Dialog,
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
    bool isDualDialogCharacter = false;
    int sectionDepth = -1; // 1-6

    friend std::ostream& operator<<(std::ostream& out, const Element& element)
    {
        out << "[ type: " << element.typeString() << "; text: " << element.text;

        if (element.textCentered) out << "; centered";
        if (element.isDualDialogCharacter) out << "; dual";

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
        case Element::Dialog:
            return "Dialog";
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
    explicit Parser(const std::string& source) { parse_(source); }

    const std::vector<TitleEntry>& titlePage() const { return titlePage_; }
    const std::vector<Element>& elements() const { return elements_; }

private:
    typedef unsigned char uchar;

    std::vector<TitleEntry> titlePage_{};
    std::vector<Element> elements_{};

    // --- Stringwork ---

    static std::string normalizeLineEndings_(const std::string& s)
    {
        std::string result{};
        result.reserve(s.size());

        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\r') {
                result += '\n';
                if (i + 1 < s.size() && s[i + 1] == '\n')
                    ++i; // consume \r\n as single \n
            } else {
                result += s[i];
            }
        }

        return result;
    }

    static std::string trimLeading_(const std::string& s)
    {
        auto it = std::find_if_not(s.begin(), s.end(), [](uchar c) {
            return std::isspace(c);
        });

        return { it, s.end() };
    }

    static std::string trim_(const std::string& s)
    {
        auto start = std::find_if_not(s.begin(), s.end(), [](uchar c) {
            return std::isspace(c);
        });

        if (start == s.end()) return {};

        auto end = std::find_if_not(s.rbegin(), s.rend(), [](uchar c) {
                       return std::isspace(c);
                   }).base();

        return { start, end };
    }

    static std::string toLower_(const std::string& s)
    {
        std::string result = s;

        std::transform(
            result.begin(),
            result.end(),
            result.begin(),
            [](uchar c) { return static_cast<char>(std::tolower(c)); });

        return result;
    }

    static std::vector<std::string> splitLines_(const std::string& s)
    {
        std::vector<std::string> lines{};
        std::string::size_type start = 0;

        for (std::string::size_type i = 0; i < s.size(); ++i) {
            if (s[i] == '\n') {
                lines.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }

        // Remaining content after the last newline (or entire string if no
        // newline). Always push, even if empty, to match ObjC's
        // componentsSeparatedByCharactersInSet behavior
        lines.push_back(s.substr(start));
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

    // --- Pattern recognition ---

    // True if the line contains no lowercase ASCII letters (`isUpper` might be
    // slightly misleading, if `s` isn't letters only)
    static bool hasNoLowercase_(const std::string& s)
    {
        return std::none_of(s.begin(), s.end(), [](uchar c) {
            return std::islower(c);
        });
    }

    enum class Exact
    {
        No = 0,
        Yes
    };

    // True if every char is whitespace and the string has at least `min` chars.
    // Exact::Yes requires the length to be exactly `min` (used to distinguish
    // the 2-space "empty line within dialogue" (exactly 2) from the 2+ space
    // "whitespace-only action line" check that follows it)
    static bool isWhitespaceOfLength_(
        const std::string& s,
        size_t min,
        Exact exactLength = Exact::No)
    {
        if ((exactLength == Exact::Yes) ? s.size() != min : s.size() < min)
            return false;

        return std::all_of(s.begin(), s.end(), [](uchar c) {
            return std::isspace(c);
        });
    }

    // Matches ObjC: ^\s*\*\/\s*$  (line ends with */ after optional whitespace)
    static bool endsWithBoneyardClose_(const std::string& line)
    {
        return trim_(line).ends_with("*/");
    }

    // Scene heading prefix (INT./EXT./EST./I./E. combinations)
    static bool matchesSceneHeading_(const std::string& line)
    {
        static const std::regex pattern(
            R"(^(INT|EXT|EST|(I|INT)\.?\/(E|EXT)\.?)[\.\-\s])",
            std::regex::icase);

        return std::regex_search(line, pattern);
    }

    // Character cue (no lowercase letters, optionally ending with "(cont'd)")
    static bool matchesCharacterCue_(const std::string& line)
    {
        static const std::regex pattern(R"(^[^a-z]+(\(cont'd\))?$)");
        return std::regex_match(line, pattern);
    }

    // Extract #scene-number# from end of text, modifying text in place. Returns
    // the scene number, or empty string if none found
    static std::string extractSceneNumber_(std::string& text)
    {
        static const std::regex pattern(R"(#([^\n#]*?)#\s*$)");
        std::smatch match{};

        if (std::regex_search(text, match, pattern)) {
            auto number_str = match[1].str();
            text = text.substr(0, static_cast<size_t>(match.position()));
            text = trim_(text);

            return number_str;
        }

        return {};
    }

    // --- Parsing ---

    void parse_(const std::string& source)
    {
        auto contents = normalizeLineEndings_(source);
        contents = trimLeading_(contents);
        contents += "\n\n";

        // Everything before the first blank line is a candidate title page
        auto blank_pos = contents.find("\n\n");
        if (blank_pos == std::string::npos) return;

        auto top_of_document = contents.substr(0, blank_pos);
        if (parseTitlePage_(top_of_document)) {
            contents = contents.substr(blank_pos);
        }

        // Prepend a newline so the first real line is "preceded by a blank"
        contents = "\n" + contents;
        parseBody_(contents);
    }

    bool parseTitlePage_(const std::string& topOfDocument)
    {
        // Inline: "Key: Value" on a single line
        static const std::regex inline_pattern(R"(^(\S[^:]+):\s*(\S.*$))");
        // Directive: "Key:" alone, value on subsequent indented lines
        static const std::regex directive_pattern(R"(^(\S[^:]+):\s*$)");

        auto lines = splitLines_(topOfDocument);
        auto found = false;
        std::string open_key{};
        std::vector<std::string> open_values{};

        for (const auto& line : lines) {

            std::smatch dir_match{};
            auto is_directive =
                !line.empty()
                && std::regex_match(line, dir_match, directive_pattern);

            if (line.empty() || is_directive) {
                found = true;

                if (!open_key.empty()) {
                    titlePage_.push_back({ open_key, open_values });
                }

                open_key = is_directive ? toLower_(dir_match[1].str()) : "";

                if (open_key == "author") open_key = "authors";
                open_values.clear();

            } else {
                std::smatch inline_match{};

                if (std::regex_match(line, inline_match, inline_pattern)) {
                    found = true;

                    if (!open_key.empty()) {
                        titlePage_.push_back({ open_key, open_values });
                        open_key.clear();
                        open_values.clear();
                    }

                    auto key = toLower_(inline_match[1].str());
                    auto value = inline_match[2].str();
                    if (key == "author") key = "authors";

                    titlePage_.push_back({ key, { value } });
                    open_key.clear();
                    open_values.clear();

                } else if (found) {
                    open_values.push_back(trim_(line));
                }
            }
        }

        if (!found) return false;

        // Edge case: a lone "Key:" with no values and nothing else on the title
        // page is not a valid title page
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

                elements_.push_back({ Element::Lyrics, line.substr(1) });
                newlines_before = 0;

                continue;
            }

            // Forced action
            if (!line.empty() && line[0] == '!') {
                elements_.push_back({ Element::Action, line.substr(1) });
                newlines_before = 0;

                continue;
            }

            // Forced character
            if (!line.empty() && line[0] == '@') {
                elements_.push_back({ Element::Character, line.substr(1) });
                newlines_before = 0;
                is_inside_dialogue_block = true;

                continue;
            }

            // Empty lines within dialogue (exactly 2 whitespace chars)
            if (is_inside_dialogue_block
                && isWhitespaceOfLength_(line, 2, Exact::Yes)) {
                newlines_before = 0;

                if (!elements_.empty()
                    && elements_.back().type == Element::Dialog) {
                    elements_.back().text += "\n" + line;
                } else {
                    elements_.push_back({ Element::Dialog, line });
                }

                continue;
            }

            // Whitespace-only lines (2+ chars)
            if (isWhitespaceOfLength_(line, 2)) {
                elements_.push_back({ Element::Action, line });
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
                    std::string text = line;
                    replaceAll_(text, "/*", "");
                    replaceAll_(text, "*/", "");
                    elements_.push_back({ Element::Boneyard, trim_(text) });

                    is_comment_block = false;
                    newlines_before = 0;
                } else {
                    is_comment_block = true;
                    comment_text += "\n";
                }

                continue;
            }

            // Close boneyard
            // TODO: This fires even when is_comment_block is false. A normal
            // line ending in "*/" outside a boneyard context would be
            // misclassified. The ObjC original has the same bug
            if (endsWithBoneyardClose_(line)) {
                std::string text = line;
                replaceAll_(text, "*/", "");

                // ObjC only appends if remaining text is blank/whitespace
                // TODO: This silently drops non-whitespace content on the
                // closing line of a multi-line boneyard
                if (text.empty()
                    || std::all_of(text.begin(), text.end(), [](uchar c) {
                           return std::isspace(c);
                       })) {
                    comment_text += trim_(text);
                }

                is_comment_block = false;

                elements_.push_back({ Element::Boneyard, comment_text });
                comment_text.clear();
                newlines_before = 0;

                continue;
            }

            // Inside boneyard
            if (is_comment_block) {
                comment_text += line + "\n";
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
                    elements_.push_back({ Element::PageBreak, line });
                    newlines_before = 0;

                    continue;
                }
            }

            // Synopsis ('=' at start, but not page break)
            {
                auto trimmed = trim_(line);

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
                        { Element::Synopsis, line.substr(pos) });

                    continue;
                }
            }

            // Comment ("[[text]]" on a single line)
            if (newlines_before > 0) {
                auto trimmed = trim_(line);

                if (trimmed.size() >= 4 && trimmed[0] == '['
                    && trimmed[1] == '[' && trimmed[trimmed.size() - 1] == ']'
                    && trimmed[trimmed.size() - 2] == ']') {
                    auto inner = trimmed.substr(2, trimmed.size() - 4);

                    // ObjC requires no ']' inside the content.
                    if (inner.find(']') == std::string::npos) {
                        elements_.push_back({ Element::Comment, trim_(inner) });
                        continue;
                    }
                }
            }

            // Section heading ('#' at start)
            {
                auto trimmed = trim_(line);
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

                    auto text = trim_(trimmed.substr(pos));
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

                auto text = trim_(line.substr(1));
                auto scene_number = extractSceneNumber_(text);

                Element element{ Element::SceneHeading, text };
                if (!scene_number.empty()) element.sceneNumber = scene_number;
                elements_.push_back(element);

                continue;
            }

            // Scene heading (INT./EXT./EST. etc.)
            if (newlines_before > 0 && matchesSceneHeading_(line)) {
                newlines_before = 0;

                std::string text = line;
                auto scene_number = extractSceneNumber_(text);

                Element element{ Element::SceneHeading, text };
                if (!scene_number.empty()) element.sceneNumber = scene_number;
                elements_.push_back(element);

                continue;
            }

            // Transition (all-caps ending in "TO:")
            if (hasNoLowercase_(line) && line.ends_with("TO:")) {
                newlines_before = 0;
                elements_.push_back({ Element::Transition, line });

                continue;
            }

            // Known transitions
            {
                auto trimmed_leading = trimLeading_(line);

                if (trimmed_leading == "FADE OUT."
                    || trimmed_leading == "CUT TO BLACK."
                    || trimmed_leading == "FADE TO BLACK.") {
                    newlines_before = 0;
                    elements_.push_back({ Element::Transition, line });

                    continue;
                }
            }

            // Forced transition ('>' / centered text '>...<')
            if (!line.empty() && line[0] == '>') {
                if (line.size() > 1 && line.back() == '<') {
                    // Centered text (strip the '>' and '<' markers)
                    auto text = trim_(line.substr(1));

                    if (!text.empty() && text.back() == '<') {
                        text.pop_back();
                        text = trim_(text);
                    }

                    Element element{ Element::Action, text };
                    element.textCentered = true;
                    elements_.push_back(element);

                } else {
                    // Forced transition
                    auto text = trim_(line.substr(1));
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

                    Element element{ Element::Character, line };

                    // Dual dialogue (trailing '^')
                    if (trim_(line).ends_with("^")) {
                        element.isDualDialogCharacter = true;

                        // Strip trailing \s*\^\s*
                        std::string text = line;
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

                        element.text = text.substr(0, end);

                        // Walk back and mark the previous Character as dual too
                        for (auto it = elements_.rbegin();
                             it != elements_.rend();
                             ++it) {
                            if (it->type == Element::Character) {
                                it->isDualDialogCharacter = true;
                                break;
                            }
                        }
                    }

                    elements_.push_back(element);
                    is_inside_dialogue_block = true;

                    continue;
                }
            }

            // Dialogue/Parenthetical
            if (is_inside_dialogue_block) {
                // Parenthetical (starts with '(' after optional whitespace)
                auto trimmed_line = trimLeading_(line);

                if (newlines_before == 0 && !trimmed_line.empty()
                    && trimmed_line[0] == '(') {
                    elements_.push_back({ Element::Parenthetical, line });
                } else {
                    // Merge consecutive dialogue lines into one element
                    if (!elements_.empty()
                        && elements_.back().type == Element::Dialog) {
                        elements_.back().text += "\n" + line;
                    } else {
                        elements_.push_back({ Element::Dialog, line });
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

                prev.text += "\n" + line;
                newlines_before = 0;

                continue;
            }

            // Action (fallback)
            elements_.push_back({ Element::Action, trim_(line) });
            newlines_before = 0;
        }
    }
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

class Renderer
{
public:
    explicit Renderer(const Parser& parser)
        : parser_(parser)
    {
    }

    std::string html() const {}

private:
    const Parser& parser_;

    std::string renderTitlePage_() const {}

    std::string renderBody_() const {}

    std::string renderElement_(const Element& element) const {}

    std::string applyInlineFormatting_(const std::string& text) const {}

    std::string cssClass_(Element::Type type) const {}

    static std::string escapeHtml_(const std::string& text) {}
};

} // namespace Fountain
