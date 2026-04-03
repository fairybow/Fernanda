/*
 * fountain-html.c -- Fountain to HTML renderer
 *
 * See fountain-html.h for API documentation and license.
 */

#include "fountain-html.h"

#include <stdio.h>
#include <string.h>


/* ==============================
 * Internal types
 * ============================== */

#define FN_HTML_BUFFER_SIZE 8192

typedef struct FN_HTML_CTX {
    void (*process_output)(const FN_CHAR*, FN_SIZE, void*);
    void* userdata;
    unsigned renderer_flags;

    /* Output buffer */
    FN_CHAR buffer[FN_HTML_BUFFER_SIZE];
    FN_SIZE buf_used;

    /* Dual dialogue state */
    int dual_char_count;    /* Characters seen inside current DUAL_DIALOGUE */

    /* Scene heading detail (held between enter/leave to emit right-side
     * scene number on close) */
    const FN_CHAR* scene_number;
    FN_SIZE scene_number_size;

    /* Title entry key (held between enter/leave for CSS class) */
    const FN_CHAR* title_key;
    FN_SIZE title_key_size;

    /* Skip flags (derived from renderer_flags and block context) */
    int skip;               /* If non-zero, suppress all output */
} FN_HTML_CTX;


/* ==============================
 * Output helpers
 * ============================== */

static void fn_html_flush_(FN_HTML_CTX* ctx)
{
    if (ctx->buf_used > 0) {
        ctx->process_output(ctx->buffer, ctx->buf_used, ctx->userdata);
        ctx->buf_used = 0;
    }
}

static void out(FN_HTML_CTX* ctx, const char* s, FN_SIZE len)
{
    if (ctx->skip) return;

    /* If it fits in the buffer, just copy. */
    if (ctx->buf_used + len <= FN_HTML_BUFFER_SIZE) {
        memcpy(ctx->buffer + ctx->buf_used, s, len);
        ctx->buf_used += len;
        return;
    }

    /* Flush current buffer, then handle the new data. */
    fn_html_flush_(ctx);

    if (len >= FN_HTML_BUFFER_SIZE) {
        /* Larger than the buffer: pass through directly. */
        ctx->process_output(s, len, ctx->userdata);
    } else {
        memcpy(ctx->buffer, s, len);
        ctx->buf_used = len;
    }
}

#define OUT_LIT(ctx, lit) out((ctx), (lit), sizeof(lit) - 1)

//static void out_s(FN_HTML_CTX* ctx, const char* s)
//{
//    out(ctx, s, (FN_SIZE)strlen(s));
//}

static void out_escaped(FN_HTML_CTX* ctx, const FN_CHAR* s, FN_SIZE len)
{
    if (ctx->skip)
        return;

    FN_SIZE last = 0;
    for (FN_SIZE i = 0; i < len; i++) {
        const char* esc = NULL;
        FN_SIZE esc_len = 0;

        switch (s[i]) {
        case '&':  esc = "&amp;";  esc_len = 5; break;
        case '<':  esc = "&lt;";   esc_len = 4; break;
        case '>':  esc = "&gt;";   esc_len = 4; break;
        case '"':  esc = "&quot;"; esc_len = 6; break;
        default:   continue;
        }

        if (i > last)
            out(ctx, s + last, i - last);
        out(ctx, esc, esc_len);
        last = i + 1;
    }

    if (last < len)
        out(ctx, s + last, len - last);
}


/* ==============================
 * CSS class mapping
 * ============================== */

/* Map a title entry key to its CSS class. Most keys map 1:1; "draft date"
 * becomes "draft-date". */
static void out_title_css_class(FN_HTML_CTX* ctx,
                                const FN_CHAR* key, FN_SIZE key_size)
{
    /* "draft date" -> "draft-date" */
    if (key_size == 10 && memcmp(key, "draft date", 10) == 0) {
        OUT_LIT(ctx, "draft-date");
        return;
    }

    /* Everything else: use key directly (already lowercased by parser). */
    out(ctx, key, key_size);
}


/* ==============================
 * SAX callbacks
 * ============================== */

static int html_enter_block(FN_BLOCKTYPE type, void* detail, void* ud)
{
    FN_HTML_CTX* ctx = (FN_HTML_CTX*)ud;

    switch (type) {

    case FN_BLOCK_DOC:
        /* No output; fn_html() emits the article/section wrapper. */
        break;

    case FN_BLOCK_TITLE_PAGE:
        if (ctx->renderer_flags & FN_HTML_FLAG_SKIP_TITLE) {
            ctx->skip = 1;
            break;
        }
        OUT_LIT(ctx, "<div id='script-title'>\n");
        break;

    case FN_BLOCK_TITLE_ENTRY:
    {
        FN_BLOCK_TITLE_ENTRY_DETAIL* d = (FN_BLOCK_TITLE_ENTRY_DETAIL*)detail;
        ctx->title_key = d->key;
        ctx->title_key_size = d->key_size;

        OUT_LIT(ctx, "<p class='");
        out_title_css_class(ctx, d->key, d->key_size);
        OUT_LIT(ctx, "'>");
        break;
    }

    case FN_BLOCK_SCENE_HEADING:
    {
        FN_BLOCK_SCENE_HEADING_DETAIL* d =
            (FN_BLOCK_SCENE_HEADING_DETAIL*)detail;
        ctx->scene_number = d->scene_number;
        ctx->scene_number_size = d->scene_number_size;

        OUT_LIT(ctx, "<p class='scene-heading'>");

        if (d->scene_number) {
            OUT_LIT(ctx, "<span class='scene-number-left'>");
            out_escaped(ctx, d->scene_number, d->scene_number_size);
            OUT_LIT(ctx, "</span>");
        }
        break;
    }

    case FN_BLOCK_ACTION:
    {
        FN_BLOCK_ACTION_DETAIL* d = (FN_BLOCK_ACTION_DETAIL*)detail;
        if (d && d->is_centered)
            OUT_LIT(ctx, "<p class='action center'>");
        else
            OUT_LIT(ctx, "<p class='action'>");
        break;
    }

    case FN_BLOCK_CHARACTER:
    {
        /* Inside dual dialogue, the second CHARACTER switches from
         * left to right panel. */
        FN_BLOCK_CHARACTER_DETAIL* d = (FN_BLOCK_CHARACTER_DETAIL*)detail;
        if (d && d->is_dual_dialogue) {
            ctx->dual_char_count++;
            if (ctx->dual_char_count == 2)
                OUT_LIT(ctx, "</div>\n<div class='dual-dialogue-right'>\n");
        }
        OUT_LIT(ctx, "<p class='character'>");
        break;
    }

    case FN_BLOCK_DIALOGUE:
        OUT_LIT(ctx, "<p class='dialogue'>");
        break;

    case FN_BLOCK_PARENTHETICAL:
        OUT_LIT(ctx, "<p class='parenthetical'>");
        break;

    case FN_BLOCK_TRANSITION:
        OUT_LIT(ctx, "<p class='transition'>");
        break;

    case FN_BLOCK_LYRICS:
        OUT_LIT(ctx, "<p class='lyrics'>");
        break;

    case FN_BLOCK_LYRICS_SPACER:
        OUT_LIT(ctx, "<p class='lyrics'>&nbsp;</p>\n");
        break;

    case FN_BLOCK_PAGE_BREAK:
        OUT_LIT(ctx, "</section>\n<section>\n");
        break;

    case FN_BLOCK_DUAL_DIALOGUE:
        ctx->dual_char_count = 0;
        OUT_LIT(ctx, "<div class='dual-dialogue'>\n");
        OUT_LIT(ctx, "<div class='dual-dialogue-left'>\n");
        break;

    /* Metadata blocks: skip in HTML output (they aren't part of the
     * rendered screenplay). They won't reach here unless the consumer
     * passed the corresponding FN_FLAG_* to fn_parse(). */
    case FN_BLOCK_BONEYARD:
    case FN_BLOCK_COMMENT:
    case FN_BLOCK_SECTION_HEADING:
    case FN_BLOCK_SYNOPSIS:
        ctx->skip = 1;
        break;
    }

    return 0;
}

static int html_leave_block(FN_BLOCKTYPE type, void* detail, void* ud)
{
    FN_HTML_CTX* ctx = (FN_HTML_CTX*)ud;
    (void)detail;

    switch (type) {

    case FN_BLOCK_DOC:
        break;

    case FN_BLOCK_TITLE_PAGE:
        ctx->skip = 0;
        if (!(ctx->renderer_flags & FN_HTML_FLAG_SKIP_TITLE))
            OUT_LIT(ctx, "</div>\n");
        break;

    case FN_BLOCK_TITLE_ENTRY:
        OUT_LIT(ctx, "</p>\n");
        break;

    case FN_BLOCK_SCENE_HEADING:
        if (ctx->scene_number) {
            OUT_LIT(ctx, "<span class='scene-number-right'>");
            out_escaped(ctx, ctx->scene_number, ctx->scene_number_size);
            OUT_LIT(ctx, "</span>");
        }
        ctx->scene_number = NULL;
        ctx->scene_number_size = 0;
        OUT_LIT(ctx, "</p>\n");
        break;

    case FN_BLOCK_ACTION:
    case FN_BLOCK_CHARACTER:
    case FN_BLOCK_DIALOGUE:
    case FN_BLOCK_PARENTHETICAL:
    case FN_BLOCK_TRANSITION:
    case FN_BLOCK_LYRICS:
        OUT_LIT(ctx, "</p>\n");
        break;

    case FN_BLOCK_LYRICS_SPACER:
    case FN_BLOCK_PAGE_BREAK:
        /* Already fully emitted on enter. */
        break;

    case FN_BLOCK_DUAL_DIALOGUE:
        OUT_LIT(ctx, "</div>\n</div>\n");
        ctx->dual_char_count = 0;
        break;

    case FN_BLOCK_BONEYARD:
    case FN_BLOCK_COMMENT:
    case FN_BLOCK_SECTION_HEADING:
    case FN_BLOCK_SYNOPSIS:
        ctx->skip = 0;
        break;
    }

    return 0;
}

static int html_enter_span(FN_SPANTYPE type, void* ud)
{
    FN_HTML_CTX* ctx = (FN_HTML_CTX*)ud;

    switch (type) {
    case FN_SPAN_EMPHASIS:
        OUT_LIT(ctx, "<em>");
        break;
    case FN_SPAN_STRONG:
        OUT_LIT(ctx, "<strong>");
        break;
    case FN_SPAN_UNDERLINE:
        OUT_LIT(ctx, "<u>");
        break;
    case FN_SPAN_NOTE:      /* Notes are stripped in HTML output. */
                            ctx->skip = 1;          break;
    }

    return 0;
}

static int html_leave_span(FN_SPANTYPE type, void* ud)
{
    FN_HTML_CTX* ctx = (FN_HTML_CTX*)ud;

    switch (type) {
    case FN_SPAN_EMPHASIS:
        OUT_LIT(ctx, "</em>");
        break;
    case FN_SPAN_STRONG:
        OUT_LIT(ctx, "</strong>");
        break;
    case FN_SPAN_UNDERLINE:
        OUT_LIT(ctx, "</u>");
        break;
    case FN_SPAN_NOTE:      ctx->skip = 0;           break;
    }

    return 0;
}

static int html_text(FN_TEXTTYPE type, const FN_CHAR* text, FN_SIZE size,
                     void* ud)
{
    FN_HTML_CTX* ctx = (FN_HTML_CTX*)ud;

    switch (type) {
    case FN_TEXT_NORMAL:
        out_escaped(ctx, text, size);
        break;
    case FN_TEXT_SOFTBREAK:
        OUT_LIT(ctx, "<br>");
        break;
    }

    return 0;
}


/* ==============================
 * Dual dialogue: mid-block hook
 * ============================== */

/* We need to detect the second CHARACTER inside a DUAL_DIALOGUE to switch
 * from the left div to the right div. The enter_block callback for
 * CHARACTER increments dual_char_count. We hook this in the enter_block
 * handler above, but we need to emit the div switch BEFORE the <p> tag.
 *
 * Reworking: insert the dual-dialogue div logic into the CHARACTER case. */


/* ==============================
 * Entry point
 * ============================== */

int fn_html(const FN_CHAR* input, FN_SIZE input_size,
            void (*process_output)(const FN_CHAR*, FN_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    FN_HTML_CTX ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.process_output = process_output;
    ctx.userdata = userdata;
    ctx.renderer_flags = renderer_flags;

    FN_PARSER parser;
    memset(&parser, 0, sizeof(parser));
    parser.enter_block = html_enter_block;
    parser.leave_block = html_leave_block;
    parser.enter_span  = html_enter_span;
    parser.leave_span  = html_leave_span;
    parser.text        = html_text;

    /* Don't ask fn_parse to emit metadata blocks -- the HTML renderer
     * skips them anyway. But we DO need notes so we can strip them. */
    unsigned pf = parser_flags & ~(FN_FLAG_BONEYARD | FN_FLAG_COMMENTS
                                   | FN_FLAG_SECTIONS | FN_FLAG_SYNOPSES);

    /* Emit outer wrapper. */
    if (!(renderer_flags & FN_HTML_FLAG_SKIP_BODY))
        process_output("<article>\n<section>\n", 20, userdata);

    int ret = fn_parse(input, input_size, &parser, &ctx, pf);

    fn_html_flush_(&ctx);

    if (!(renderer_flags & FN_HTML_FLAG_SKIP_BODY))
        process_output("</section>\n</article>\n", 22, userdata);

    return ret;
}


/* ==============================
 * Default CSS
 * ============================== */

const char* fn_html_css(void)
{
    return
        "* {\n"
        "    -webkit-touch-callout: none;\n"
        "    -webkit-user-select: none;\n"
        "}\n"
        "html {\n"
        "    margin: 0;\n"
        "    padding: 0;\n"
        "}\n"
        "body {\n"
        "    background-color: #fff;\n"
        "    color: #3e3e3e;\n"
        "    font: 14px/1.3em 'Courier Prime', 'Courier', monospace;\n"
        "    padding: 0;\n"
        "    margin: 0;\n"
        "}\n"
        "article {\n"
        "    padding: 40px 0;\n"
        "    margin: 0;\n"
        "}\n"
        "section {\n"
        "    padding: 0 0 0 40px;\n"
        "    width: 465px;\n"
        "    margin-right: auto;\n"
        "    margin-left: auto;\n"
        "}\n"
        "p {\n"
        "    margin: 1.3em auto;\n"
        "    word-wrap: break-word;\n"
        "    padding: 0 10px;\n"
        "}\n"
        "p.page-break {\n"
        "    text-align: right;\n"
        "    border-top: 1px solid #ccc;\n"
        "    padding-top: 20px;\n"
        "    margin-top: 20px;\n"
        "}\n"
        "body > p:first-child {\n"
        "    margin-top: 0;\n"
        "}\n"
        ".scene-heading, .transition, .character {\n"
        "    text-transform: uppercase;\n"
        "}\n"
        ".transition {\n"
        "    text-align: right;\n"
        "}\n"
        ".character {\n"
        "    margin: 1.3em auto 0;\n"
        "    width: 180px;\n"
        "}\n"
        ".dialogue {\n"
        "    margin: 0 auto;\n"
        "    width: 310px;\n"
        "}\n"
        ".parenthetical {\n"
        "    margin: 0 auto;\n"
        "    width: 250px;\n"
        "}\n"
        ".scene-heading {\n"
        "    margin-top: 2.6em;\n"
        "    font-weight: bold;\n"
        "    position: relative;\n"
        "    padding-right: 40px;\n"
        "}\n"
        ".scene-number-left {\n"
        "    float: left;\n"
        "    margin-left: -50px;\n"
        "}\n"
        ".scene-number-right {\n"
        "    position: absolute;\n"
        "    right: 0;\n"
        "    top: 0;\n"
        "}\n"
        "#script-title {\n"
        "    overflow: hidden;\n"
        "    display: block;\n"
        "}\n"
        "#script-title .title {\n"
        "    text-align: center;\n"
        "    margin: 1.3em 0;\n"
        "    text-decoration: underline;\n"
        "    font-weight: bold;\n"
        "    text-transform: uppercase;\n"
        "}\n"
        "#script-title .credit {\n"
        "    text-align: center;\n"
        "}\n"
        "#script-title .authors {\n"
        "    text-align: center;\n"
        "}\n"
        "#script-title .source {\n"
        "    text-align: center;\n"
        "    padding-top: 1.3em;\n"
        "}\n"
        "#script-title .notes {\n"
        "    padding-top: 2.6em;\n"
        "    white-space: pre-line;\n"
        "}\n"
        ".center {\n"
        "    text-align: center !important;\n"
        "}\n"
        "hr {\n"
        "    height: 0px;\n"
        "    border: none;\n"
        "    border-bottom: 1px solid #ccc;\n"
        "}\n"
        ".dual-dialogue {\n"
        "    overflow: hidden;\n"
        "}\n"
        ".dual-dialogue .dual-dialogue-left,\n"
        ".dual-dialogue .dual-dialogue-right {\n"
        "    width: 228px;\n"
        "    float: left;\n"
        "}\n"
        ".dual-dialogue p {\n"
        "    width: auto;\n"
        "}\n"
        ".dual-dialogue .character {\n"
        "    padding-left: 40px;\n"
        "}\n"
        ".dual-dialogue .parenthetical {\n"
        "    padding-left: 40px;\n"
        "}\n"
        ".lyrics {\n"
        "    font-style: italic;\n"
        "}\n";
}
