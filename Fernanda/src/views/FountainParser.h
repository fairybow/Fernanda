// Translated from:
// https://github.com/nyousefi/Fountain
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FNElement.h
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FastFountainParser.h

// TODO: Fountain::Renderer (to HTML; see
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FNHTMLScript.h)
// TODO: Fountain::Paginator (we will need a printing layout; see
// https://github.com/nyousefi/Fountain/blob/master/Fountain/FNPaginator.h)

#pragma once

#include <string>
#include <vector>

// TODO: Move to its own repo
// TODO: Keep header-only
namespace Fountain {

struct TitleEntry
{
    std::string key{};
    std::vector<std::string> values{};
};

struct Element
{
    enum Type
    {
        SceneHeading,
        Action,
        Character,
        Dialog,
        Parenthetical,
        Transition,
        Lyrics,
        PageBreak,
        Boneyard,
        Comment,
        SectionHeading,
        Synopsis
    };

    Type type{};
    std::string text{};
    bool isCentered = false;

    // Type specific:
    std::string sceneNumber{}; // e.g. "1A"
    bool isDualDialog = false;
    int sectionDepth = -1; // 1-6
};

class Parser
{
public:
    explicit Parser(const std::string& source) {}

    const std::vector<TitleEntry>& titlePage() const { return titlePage_; }
    const std::vector<Element>& elements() const { return elements_; }

private:
    std::vector<TitleEntry> titlePage_{};
    std::vector<Element> elements_{};

    void parse_(const std::string& source) {}
};

} // namespace Fountain
