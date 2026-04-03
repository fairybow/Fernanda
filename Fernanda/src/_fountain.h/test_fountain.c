#include "fountain.h"
#include <stdio.h>
#include <string.h>

static const char* block_name(FN_BLOCKTYPE t) {
    switch (t) {
    case FN_BLOCK_DOC:              return "DOC";
    case FN_BLOCK_TITLE_PAGE:       return "TITLE_PAGE";
    case FN_BLOCK_TITLE_ENTRY:      return "TITLE_ENTRY";
    case FN_BLOCK_SCENE_HEADING:    return "SCENE_HEADING";
    case FN_BLOCK_ACTION:           return "ACTION";
    case FN_BLOCK_CHARACTER:        return "CHARACTER";
    case FN_BLOCK_DIALOGUE:         return "DIALOGUE";
    case FN_BLOCK_PARENTHETICAL:    return "PARENTHETICAL";
    case FN_BLOCK_TRANSITION:       return "TRANSITION";
    case FN_BLOCK_LYRICS:           return "LYRICS";
    case FN_BLOCK_LYRICS_SPACER:    return "LYRICS_SPACER";
    case FN_BLOCK_PAGE_BREAK:       return "PAGE_BREAK";
    case FN_BLOCK_BONEYARD:         return "BONEYARD";
    case FN_BLOCK_COMMENT:          return "COMMENT";
    case FN_BLOCK_SECTION_HEADING:  return "SECTION_HEADING";
    case FN_BLOCK_SYNOPSIS:         return "SYNOPSIS";
    case FN_BLOCK_DUAL_DIALOGUE:    return "DUAL_DIALOGUE";
    default:                        return "???";
    }
}

static const char* span_name(FN_SPANTYPE t) {
    switch (t) {
    case FN_SPAN_EMPHASIS:  return "EM";
    case FN_SPAN_STRONG:    return "STRONG";
    case FN_SPAN_UNDERLINE: return "U";
    case FN_SPAN_NOTE:      return "NOTE";
    default:                return "???";
    }
}

static int depth = 0;

static void indent(void) {
    for (int i = 0; i < depth; i++) printf("  ");
}

static int on_enter_block(FN_BLOCKTYPE type, void* detail, void* ud) {
    (void)ud;
    indent();
    printf("+ %s", block_name(type));

    if (type == FN_BLOCK_TITLE_ENTRY && detail) {
        FN_BLOCK_TITLE_ENTRY_DETAIL* d = detail;
        printf(" [key=%.*s]", d->key_size, d->key);
    }
    if (type == FN_BLOCK_SCENE_HEADING && detail) {
        FN_BLOCK_SCENE_HEADING_DETAIL* d = detail;
        if (d->scene_number)
            printf(" [scene#=%.*s]", d->scene_number_size, d->scene_number);
    }
    if (type == FN_BLOCK_CHARACTER && detail) {
        FN_BLOCK_CHARACTER_DETAIL* d = detail;
        if (d->is_dual_dialogue)
            printf(" [DUAL]");
    }
    if (type == FN_BLOCK_ACTION && detail) {
        FN_BLOCK_ACTION_DETAIL* d = detail;
        if (d->is_centered)
            printf(" [centered]");
    }
    if (type == FN_BLOCK_SECTION_HEADING && detail) {
        FN_BLOCK_SECTION_HEADING_DETAIL* d = detail;
        printf(" [depth=%d]", d->depth);
    }

    printf("\n");
    depth++;
    return 0;
}

static int on_leave_block(FN_BLOCKTYPE type, void* detail, void* ud) {
    (void)detail; (void)ud;
    depth--;
    indent();
    printf("- %s\n", block_name(type));
    return 0;
}

static int on_enter_span(FN_SPANTYPE type, void* ud) {
    (void)ud;
    indent();
    printf("<%s>\n", span_name(type));
    return 0;
}

static int on_leave_span(FN_SPANTYPE type, void* ud) {
    (void)ud;
    indent();
    printf("</%s>\n", span_name(type));
    return 0;
}

static int on_text(FN_TEXTTYPE type, const FN_CHAR* text, FN_SIZE size,
                   void* ud) {
    (void)ud;
    indent();
    if (type == FN_TEXT_SOFTBREAK) {
        printf("[SOFTBREAK]\n");
    } else {
        printf("TEXT: \"%.*s\"\n", size, text);
    }
    return 0;
}

int main(void) {
    const char* input =
        "Title: Big Fish\n"
        "Credit: written by\n"
        "Author: John August\n"
        "\n"
        "# ACT ONE\n"
        "\n"
        "= Setting up the story.\n"
        "\n"
        "EXT. A RIVER - DAY #1#\n"
        "\n"
        "FADE IN:\n"
        "\n"
        "A dogfish swims upstream.\n"
        "\n"
        "EDWARD (V.O.)\n"
        "There are some fish that *cannot* be caught.\n"
        "(whispering)\n"
        "It's not that they're faster or **stronger**.\n"
        "\n"
        "WILL ^"  "\n"
        "I've heard this story.\n"
        "\n"
        ">THE END<\n"
        "\n"
        "===\n";

    FN_PARSER parser;
    memset(&parser, 0, sizeof(parser));
    parser.enter_block = on_enter_block;
    parser.leave_block = on_leave_block;
    parser.enter_span = on_enter_span;
    parser.leave_span = on_leave_span;
    parser.text = on_text;

    int ret = fn_parse(input, (FN_SIZE)strlen(input), &parser, NULL,
                       FN_FLAG_ALL_META);

    printf("\nReturn: %d\n", ret);
    return ret;
}
