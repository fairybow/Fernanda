/*
 * fountain.c -- C Fountain screenplay parser
 * https://fountain.io
 *
 * See fountain.h for API documentation and license.
 */

#include "fountain.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
TODO:
- need default css fn at end?
- clean/organize
- clang format
*/


/* ==============================
 * Internal types and constants
 * ============================== */

typedef unsigned char UCHAR;

/* A line in the normalized input (offsets into ctx->text) */
typedef struct FN_LINE {
    FN_OFFSET beg;
    FN_OFFSET end;    /* exclusive */
} FN_LINE;

/* An element buffered in a dialogue group */
typedef struct FN_BUFFERED {
    FN_BLOCKTYPE type;
    FN_OFFSET text_beg;
    FN_OFFSET text_end;
    /* Type-specific: */
    int is_dual;
} FN_BUFFERED;

/* A complete dialogue group (Character + Dialogue/Parenthetical lines) */
typedef struct FN_GROUP {
    FN_BUFFERED* elems;
    unsigned n;
    unsigned alloc;
} FN_GROUP;

/* Inline mark: a resolved opener or closer position */
typedef struct FN_MARK {
    FN_OFFSET pos;
    unsigned len;           /* marker length in source chars */
    int is_opener;
    unsigned span_bits;     /* bitmask: 1 << FN_SPAN_* */
} FN_MARK;

#define SPAN_BIT(s)  (1u << (s))

/* Inline marker definition (processed longest-first) */
typedef struct FN_MARKER_DEF
{
    const char* pat1;
    const char* pat2; /* alternate form, or NULL */
    unsigned len1;
    unsigned len2;
    unsigned span_bits;
    int reject_underscore; /* content must not contain '_' */
    unsigned required_chars; /* bitmask: which chars must be present */
} FN_MARKER_DEF;

#define CHAR_STAR 1u
#define CHAR_UNDER 2u
#define CHAR_BRACK 4u

static const FN_MARKER_DEF MARKER_DEFS[] = {
    { "_***",
      "***_",
      4,
      4,
      SPAN_BIT(FN_SPAN_STRONG) | SPAN_BIT(FN_SPAN_EMPHASIS)
          | SPAN_BIT(FN_SPAN_UNDERLINE),
      0,
      CHAR_STAR | CHAR_UNDER },
    { "***",
      NULL,
      3,
      0,
      SPAN_BIT(FN_SPAN_STRONG) | SPAN_BIT(FN_SPAN_EMPHASIS),
      0,
      CHAR_STAR },
    { "_**",
      "**_",
      3,
      3,
      SPAN_BIT(FN_SPAN_STRONG) | SPAN_BIT(FN_SPAN_UNDERLINE),
      0,
      CHAR_STAR | CHAR_UNDER },
    { "_*",
      "*_",
      2,
      2,
      SPAN_BIT(FN_SPAN_EMPHASIS) | SPAN_BIT(FN_SPAN_UNDERLINE),
      0,
      CHAR_STAR | CHAR_UNDER },
    { "**", NULL, 2, 0, SPAN_BIT(FN_SPAN_STRONG), 0, CHAR_STAR },
    { "*", NULL, 1, 0, SPAN_BIT(FN_SPAN_EMPHASIS), 0, CHAR_STAR },
    { "_", NULL, 1, 0, SPAN_BIT(FN_SPAN_UNDERLINE), 1, CHAR_UNDER },
};

#define N_MARKER_DEFS  (sizeof(MARKER_DEFS) / sizeof(MARKER_DEFS[0]))

/* Parse context */
typedef struct FN_CTX {
    FN_CHAR* text;          /* normalized mutable copy */
    FN_SIZE size;
    const FN_PARSER* parser;
    void* userdata;
    unsigned flags;

    /* Line table */
    FN_LINE* lines;
    unsigned n_lines;
    unsigned alloc_lines;

    /* Dialogue group buffering (for dual dialogue detection) */
    FN_GROUP prev_group;
    FN_GROUP curr_group;
    int in_dual;            /* inside second half of dual dialogue */
    int in_dialogue;        /* inside a dialogue block (after Character) */
    FN_BLOCKTYPE last_block;/* type of the most recent lyrics block (for spacer) */

    /* Inline processing (reusable scratch space) */
    unsigned char* consumed;
    unsigned alloc_consumed;
    FN_MARK* marks;
    unsigned n_marks;
    unsigned alloc_marks;
} FN_CTX;


/* ==============================
 * Callback helpers
 * ============================== */

/* All callbacks propagate non-zero returns to abort parsing. */

#define CHECK(call)  do { int r_ = (call); if (r_ != 0) return r_; } while (0)

static int fn_enter_block_(FN_CTX* ctx, FN_BLOCKTYPE type, void* detail)
{
    if (ctx->parser->enter_block)
        return ctx->parser->enter_block(type, detail, ctx->userdata);
    return 0;
}

static int fn_leave_block_(FN_CTX* ctx, FN_BLOCKTYPE type, void* detail)
{
    if (ctx->parser->leave_block)
        return ctx->parser->leave_block(type, detail, ctx->userdata);
    return 0;
}

static int fn_enter_span_(FN_CTX* ctx, FN_SPANTYPE type)
{
    if (ctx->parser->enter_span)
        return ctx->parser->enter_span(type, ctx->userdata);
    return 0;
}

static int fn_leave_span_(FN_CTX* ctx, FN_SPANTYPE type)
{
    if (ctx->parser->leave_span)
        return ctx->parser->leave_span(type, ctx->userdata);
    return 0;
}

static int fn_text_(FN_CTX* ctx, FN_TEXTTYPE type, const FN_CHAR* s, FN_SIZE len)
{
    if (ctx->parser->text)
        return ctx->parser->text(type, s, len, ctx->userdata);
    return 0;
}


/* ==============================
 * Memory helpers
 * ============================== */

/* Grow an array to hold at least `needed` elements of `elem_size` bytes.
 * Returns 0 on success, -1 on allocation failure. */
static int fn_grow_(void** arr, unsigned* alloc, unsigned needed,
                    unsigned elem_size)
{
    if (needed <= *alloc)
        return 0;

    unsigned cap = *alloc ? *alloc : 16;
    while (cap < needed)
        cap *= 2;

    void* p = realloc(*arr, (size_t)cap * elem_size);
    if (!p)
        return -1;

    *arr = p;
    *alloc = cap;
    return 0;
}


/* ==============================
 * Character / string helpers
 * ============================== */

static int fn_isblank_(int c) { return c == ' ' || c == '\t'; }

static int fn_isspace_(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r'
        || c == '\f' || c == '\v';
}

static int fn_toupper_(int c)
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

static int fn_tolower_(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

/* True if s[off..off+len) matches pat (length plen) exactly. */
static int fn_match_(const FN_CHAR* s, FN_SIZE size, FN_OFFSET off,
                     const char* pat, unsigned plen)
{
    if (off + plen > size)
        return 0;
    for (unsigned i = 0; i < plen; i++) {
        if (s[off + i] != pat[i])
            return 0;
    }
    return 1;
}

/* Skip leading whitespace, return new offset. */
static FN_OFFSET fn_skip_ws_(const FN_CHAR* s, FN_SIZE size, FN_OFFSET off)
{
    while (off < size && fn_isspace_((UCHAR)s[off]))
        off++;
    return off;
}

/* Skip trailing whitespace, return new end (exclusive). */
static FN_OFFSET fn_rskip_ws_(const FN_CHAR* s, FN_OFFSET beg, FN_OFFSET end)
{
    while (end > beg && fn_isspace_((UCHAR)s[end - 1]))
        end--;
    return end;
}

/* Trimmed line boundaries (modifies beg/end in place). */
static void fn_trim_(const FN_CHAR* s, FN_OFFSET* beg, FN_OFFSET* end)
{
    *beg = fn_skip_ws_(s, *end, *beg);
    *end = fn_rskip_ws_(s, *beg, *end);
}


/* ==============================
 * Input normalization / lines
 * ============================== */

/* Copy input, normalize \r\n and \r to \n, trim leading whitespace,
 * append \n\n. Build line table. Returns 0 or -1. */
static int fn_init_input_(FN_CTX* ctx, const FN_CHAR* input, FN_SIZE input_size)
{
    /* Worst case: input_size + 3 (leading \n + trailing \n\n + NUL). */
    FN_SIZE alloc_size = input_size + 4;
    ctx->text = (FN_CHAR*)malloc(alloc_size);
    if (!ctx->text) return -1;

    /* Pre-size line table (~40 chars/line average for screenplays). */
    unsigned est_lines = (input_size / 40) + 16;
    if (fn_grow_(
            (void**)&ctx->lines,
            &ctx->alloc_lines,
            est_lines,
            sizeof(FN_LINE))
        != 0)
        return -1;

    /* Skip leading whitespace in input. */
    FN_SIZE r = 0;
    while (r < input_size && fn_isspace_((UCHAR)input[r]))
        r++;

    /* Normalize line endings and build line table in a single pass. */
    FN_SIZE w = 0;
    FN_OFFSET line_beg = 0;

#define RECORD_LINE_()                                                         \
    do {                                                                       \
        if (fn_grow_(                                                          \
                (void**)&ctx->lines,                                           \
                &ctx->alloc_lines,                                             \
                ctx->n_lines + 1,                                              \
                sizeof(FN_LINE))                                               \
            != 0)                                                              \
            return -1;                                                         \
        ctx->lines[ctx->n_lines].beg = line_beg;                               \
        ctx->lines[ctx->n_lines].end = w;                                      \
        ctx->n_lines++;                                                        \
    } while (0)

    for (; r < input_size; r++) {
        if (input[r] == '\r' || input[r] == '\n') {
            if (input[r] == '\r' && r + 1 < input_size && input[r + 1] == '\n')
                r++; /* consume \r\n as single newline */

            RECORD_LINE_();
            ctx->text[w++] = '\n';
            line_beg = w;
        } else {
            ctx->text[w++] = input[r];
        }
    }

    /* Append \n\n to ensure final line terminates cleanly. */
    RECORD_LINE_();
    ctx->text[w++] = '\n';
    line_beg = w;

    RECORD_LINE_();
    ctx->text[w++] = '\n';
    line_beg = w;

    /* Trailing line after final \n (matches original behavior). */
    RECORD_LINE_();

#undef RECORD_LINE_

    ctx->text[w] = '\0';
    ctx->size = w;

    return 0;
}


/* ==============================
 * Pattern matching
 * ============================== */

/* True if the region [beg, end) contains no lowercase ASCII. */
static int fn_has_no_lowercase_(const FN_CHAR* s, FN_OFFSET beg, FN_OFFSET end)
{
    for (FN_OFFSET i = beg; i < end; i++) {
        if (s[i] >= 'a' && s[i] <= 'z')
            return 0;
    }
    return 1;
}

/* True if every char in [beg, end) is whitespace, and length == `exact_len`
 * (or >= `exact_len` if `exact` is false). */
static int fn_is_ws_of_len_(const FN_CHAR* s, FN_OFFSET beg, FN_OFFSET end,
                            unsigned min_len, int exact)
{
    unsigned len = end - beg;
    if (exact ? len != min_len : len < min_len)
        return 0;
    for (FN_OFFSET i = beg; i < end; i++) {
        if (!fn_isspace_((UCHAR)s[i]))
            return 0;
    }
    return 1;
}

/* True if line ends with boneyard close marker ( optional-ws * / optional-ws ) */
static int fn_ends_with_boneyard_close_(const FN_CHAR* s, FN_OFFSET beg,
                                        FN_OFFSET end)
{
    FN_OFFSET p = end;
    while (p > beg && fn_isspace_((UCHAR)s[p - 1]))
        p--;
    return p >= beg + 2 && s[p - 2] == '*' && s[p - 1] == '/';
}

/* Scene heading: INT, EXT, EST, I/E, INT/EXT etc. followed by delimiter. */
static int fn_matches_scene_heading_(const FN_CHAR* s, FN_OFFSET beg,
                                     FN_OFFSET end)
{
    FN_SIZE len = end - beg;
    if (len < 4)
        return 0;

    #define AT(i) ((beg + (i) < end) \
        ? (char)fn_toupper_((UCHAR)s[beg + (i)]) : '\0')

    #define IS_DELIM(i) (beg + (i) < end \
        && (s[beg + (i)] == '.' || s[beg + (i)] == '-' \
            || fn_isblank_((UCHAR)s[beg + (i)])))

    /* Try compound: I/E, INT/EXT, etc. */
    {
        FN_OFFSET p = 0;
        if (AT(0) == 'I') {
            p = 1;
            if (AT(1) == 'N' && AT(2) == 'T') p = 3;
            if (AT(p) == '.') p++;
        }
        if (p > 0 && AT(p) == '/') {
            p++;
            if (AT(p) == 'E') {
                p++;
                if (AT(p) == 'X' && AT(p + 1) == 'T') p += 2;
                if (AT(p) == '.') p++;
            } else {
                p = 0;
            }
            if (p > 0 && IS_DELIM(p))
                return 1;
        }
    }

    /* Simple prefixes. */
    if (AT(0) == 'I' && AT(1) == 'N' && AT(2) == 'T' && IS_DELIM(3)) return 1;
    if (AT(0) == 'E' && AT(1) == 'X' && AT(2) == 'T' && IS_DELIM(3)) return 1;
    if (AT(0) == 'E' && AT(1) == 'S' && AT(2) == 'T' && IS_DELIM(3)) return 1;

    #undef AT
    #undef IS_DELIM

    return 0;
}

/* Character cue: no lowercase except optional trailing (cont'd). */
static int fn_matches_character_cue_(const FN_CHAR* s, FN_OFFSET beg,
                                     FN_OFFSET end)
{
    if (beg >= end)
        return 0;

    FN_OFFSET check_end = end;
    static const char contd[] = "(cont'd)";
    unsigned contd_len = 8;

    if (check_end - beg >= contd_len) {
        int match = 1;
        for (unsigned i = 0; i < contd_len; i++) {
            if (s[check_end - contd_len + i] != contd[i]) {
                match = 0;
                break;
            }
        }
        if (match)
            check_end -= contd_len;
    }

    if (check_end <= beg)
        return 0;

    return fn_has_no_lowercase_(s, beg, check_end);
}

/* Extract scene number (#num#) from end of text. Sets *num and *num_size
 * to the number content, and adjusts *text_end to exclude the #num# and
 * trailing whitespace. Returns 0 if no scene number found. */
static int fn_extract_scene_number_(const FN_CHAR* s, FN_OFFSET beg,
                                    FN_OFFSET* text_end,
                                    const FN_CHAR** num, FN_SIZE* num_size)
{
    FN_OFFSET p = fn_rskip_ws_(s, beg, *text_end);
    if (p <= beg || s[p - 1] != '#')
        return 0;

    FN_OFFSET close = p - 1;
    FN_OFFSET scan = close;

    while (scan > beg) {
        scan--;
        if (s[scan] == '#') {
            *num = s + scan + 1;
            *num_size = close - (scan + 1);
            *text_end = fn_rskip_ws_(s, beg, scan);
            return 1;
        }
        if (s[scan] == '\n')
            return 0;
    }

    return 0;
}


/* ==============================
 * Inline span processing
 * ============================== */

/* Ensure consumed/marks arrays are large enough. */
static int fn_inline_ensure_(FN_CTX* ctx, FN_SIZE size)
{
    if (fn_grow_((void**)&ctx->consumed, &ctx->alloc_consumed,
                 size, sizeof(unsigned char)) != 0)
        return -1;
    return 0;
}

static int fn_add_mark_(FN_CTX* ctx, FN_OFFSET pos, unsigned len,
                        int is_opener, unsigned span_bits)
{
    if (fn_grow_((void**)&ctx->marks, &ctx->alloc_marks,
                 ctx->n_marks + 1, sizeof(FN_MARK)) != 0)
        return -1;

    FN_MARK* m = &ctx->marks[ctx->n_marks++];
    m->pos = pos;
    m->len = len;
    m->is_opener = is_opener;
    m->span_bits = span_bits;
    return 0;
}

/* Scan for [[note]] pairs in a single line. */
static int fn_scan_notes_(FN_CTX* ctx, const FN_CHAR* s, FN_SIZE size,
                          unsigned char* consumed)
{
    for (FN_OFFSET i = 0; i + 1 < size; i++) {
        if (consumed[i])
            continue;
        if (s[i] != '[' || s[i + 1] != '[')
            continue;

        /* Found [[ -- look for ]] */
        FN_OFFSET close = 0;
        int found = 0;
        for (FN_OFFSET j = i + 2; j + 1 < size; j++) {
            if (s[j] == ']' && s[j + 1] == ']') {
                close = j;
                found = 1;
                break;
            }
        }
        if (!found)
            continue;

        if (ctx->flags & FN_FLAG_NOTES) {
            /* Mark [[ and ]] as consumed; content stays visible. */
            consumed[i] = 1; consumed[i + 1] = 1;
            consumed[close] = 1; consumed[close + 1] = 1;

            if (fn_add_mark_(ctx, i, 2, 1, SPAN_BIT(FN_SPAN_NOTE)) != 0)
                return -1;
            if (fn_add_mark_(ctx, close, 2, 0, SPAN_BIT(FN_SPAN_NOTE)) != 0)
                return -1;
        } else {
            /* Consume everything including content. */
            for (FN_OFFSET k = i; k < close + 2; k++)
                consumed[k] = 1;
        }

        i = close + 1;
    }

    return 0;
}

/* Check if content between opener and closer is valid (no < > in content,
 * optionally no _ in content). */
static int fn_valid_content_(const FN_CHAR* s, unsigned char* consumed,
                             FN_OFFSET beg, FN_OFFSET end,
                             int reject_underscore)
{
    int has_content = 0;
    for (FN_OFFSET i = beg; i < end; i++) {
        if (consumed[i])
            continue;
        if (s[i] == '<' || s[i] == '>')
            return 0;
        if (reject_underscore && s[i] == '_')
            return 0;
        has_content = 1;
    }
    return has_content;
}

/* Scan for one marker type across a single line. */
static int fn_scan_marker_(
    FN_CTX* ctx,
    const FN_CHAR* s,
    FN_SIZE size,
    unsigned char* consumed,
    const FN_MARKER_DEF* md)
{
    for (FN_OFFSET i = 0; i < size;) {
        if (consumed[i]) {
            i++;
            continue;
        }

        /* Try to match opener (pattern1 or pattern2). */
        unsigned opener_len = 0;
        if (fn_match_(s, size, i, md->pat1, md->len1))
            opener_len = md->len1;
        else if (md->pat2 && fn_match_(s, size, i, md->pat2, md->len2))
            opener_len = md->len2;

        if (opener_len == 0) {
            i++;
            continue;
        }

        /* Scan for matching closer, tracking content validity
         * incrementally to avoid re-scanning the region. */
        FN_OFFSET content_beg = i + opener_len;
        int found = 0;
        int has_content = 0;
        int invalid = 0;

        for (FN_OFFSET j = content_beg; j < size; j++) {
            if (consumed[j]) continue;

            char c = s[j];

            /* Check validity before testing for closer, so that a
             * closer immediately after invalid content is rejected. */
            if (c == '<' || c == '>') {
                invalid = 1;
                break;
            }
            if (md->reject_underscore && c == '_') {
                invalid = 1;
                break;
            }

            unsigned closer_len = 0;
            if (fn_match_(s, size, j, md->pat1, md->len1))
                closer_len = md->len1;
            else if (md->pat2 && fn_match_(s, size, j, md->pat2, md->len2))
                closer_len = md->len2;

            if (closer_len > 0 && has_content) {
                /* Mark opener and closer chars as consumed. */
                for (unsigned k = 0; k < opener_len; k++)
                    consumed[i + k] = 1;
                for (unsigned k = 0; k < closer_len; k++)
                    consumed[j + k] = 1;

                if (fn_add_mark_(ctx, i, opener_len, 1, md->span_bits) != 0)
                    return -1;
                if (fn_add_mark_(ctx, j, closer_len, 0, md->span_bits) != 0)
                    return -1;

                i = j + closer_len;
                found = 1;
                break;
            }

            has_content = 1;
        }

        if (!found) i += opener_len;
    }

    return 0;
}

/* Comparison for sorting marks by position. */
static int fn_mark_cmp_(const void* a, const void* b)
{
    const FN_MARK* ma = (const FN_MARK*)a;
    const FN_MARK* mb = (const FN_MARK*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return 1;
    /* Openers before closers at same position (shouldn't happen, but safe). */
    return mb->is_opener - ma->is_opener;
}

/* Emit span enter/leave for a mark's span_bits. Openers emit in order
 * UNDERLINE, STRONG, EMPHASIS; closers emit in reverse. */
static int fn_emit_spans_(FN_CTX* ctx, const FN_MARK* mark)
{
    static const FN_SPANTYPE open_order[] = {
        FN_SPAN_UNDERLINE, FN_SPAN_STRONG, FN_SPAN_EMPHASIS, FN_SPAN_NOTE
    };
    static const FN_SPANTYPE close_order[] = {
        FN_SPAN_NOTE, FN_SPAN_EMPHASIS, FN_SPAN_STRONG, FN_SPAN_UNDERLINE
    };

    const FN_SPANTYPE* order;
    unsigned count;

    if (mark->is_opener) {
        order = open_order;
        count = 4;
    } else {
        order = close_order;
        count = 4;
    }

    for (unsigned i = 0; i < count; i++) {
        if (!(mark->span_bits & SPAN_BIT(order[i])))
            continue;
        if (mark->is_opener)
            CHECK(fn_enter_span_(ctx, order[i]));
        else
            CHECK(fn_leave_span_(ctx, order[i]));
    }

    return 0;
}

static unsigned fn_char_presence_(const FN_CHAR* s, FN_SIZE size)
{
    unsigned bits = 0;
    for (FN_SIZE i = 0; i < size; i++) {
        if (s[i] == '*') bits |= 1;
        if (s[i] == '_') bits |= 2;
        if (s[i] == '[') bits |= 4;
    }
    return bits;
}

/* Process inline formatting for one line of text and emit callbacks.
 * The text is s[beg..end). */
static int fn_process_inline_line_(
    FN_CTX* ctx,
    const FN_CHAR* s,
    FN_OFFSET beg,
    FN_OFFSET end)
{
    FN_SIZE len = end - beg;
    if (len == 0) return 0;

    const FN_CHAR* line = s + beg;

    /* Ensure scratch arrays are large enough. */
    if (fn_inline_ensure_(ctx, len) != 0) return -1;

    memset(ctx->consumed, 0, len);
    ctx->n_marks = 0;

    /* Single pre-scan: which marker characters are present? */
    unsigned present = 0;
    for (FN_SIZE i = 0; i < len; i++) {
        if (line[i] == '*')
            present |= CHAR_STAR;
        else if (line[i] == '_')
            present |= CHAR_UNDER;
        else if (line[i] == '[')
            present |= CHAR_BRACK;
    }

    /* Pass 1: notes (before other markers). */
    if (present & CHAR_BRACK)
        CHECK(fn_scan_notes_(ctx, line, len, ctx->consumed));

    /* Pass 2+: inline markers, longest first. Skip defs whose
     * required characters aren't present in this line. */
    for (unsigned m = 0; m < N_MARKER_DEFS; m++) {
        if ((MARKER_DEFS[m].required_chars & present)
            == MARKER_DEFS[m].required_chars)
            CHECK(fn_scan_marker_(
                ctx,
                line,
                len,
                ctx->consumed,
                &MARKER_DEFS[m]));
    }

    /* Sort marks by position. */
    if (ctx->n_marks > 1)
        qsort(ctx->marks, ctx->n_marks, sizeof(FN_MARK), fn_mark_cmp_);

    /* Emit text and span callbacks. */
    FN_OFFSET pos = 0;
    unsigned mi = 0;

    while (pos < len) {
        if (mi < ctx->n_marks && ctx->marks[mi].pos <= pos) {
            FN_MARK* mk = &ctx->marks[mi];
            CHECK(fn_emit_spans_(ctx, mk));
            pos = mk->pos + mk->len;
            mi++;
            continue;
        }

        FN_OFFSET run_beg = pos;
        FN_OFFSET next_mark_pos =
            (mi < ctx->n_marks) ? ctx->marks[mi].pos : len;

        while (pos < next_mark_pos) {
            if (ctx->consumed[pos]) {
                if (pos > run_beg)
                    CHECK(fn_text_(
                        ctx,
                        FN_TEXT_NORMAL,
                        line + run_beg,
                        pos - run_beg));
                while (pos < next_mark_pos && ctx->consumed[pos])
                    pos++;
                run_beg = pos;
            } else {
                pos++;
            }
        }

        if (pos > run_beg)
            CHECK(fn_text_(ctx, FN_TEXT_NORMAL, line + run_beg, pos - run_beg));
    }

    return 0;
}

/* True if the range contains any inline marker characters. */
static int fn_has_inline_markers_(const FN_CHAR* s, FN_OFFSET beg, FN_OFFSET end)
{
    for (FN_OFFSET i = beg; i < end; i++) {
        if (s[i] == '*' || s[i] == '_' || s[i] == '[') return 1;
    }
    return 0;
}

/* Process inline formatting for a (possibly multi-line) block of text.
 * Newlines within the text are emitted as FN_TEXT_SOFTBREAK. */
static int fn_process_inline_(FN_CTX* ctx, FN_OFFSET beg, FN_OFFSET end)
{
    if (!fn_has_inline_markers_(ctx->text, beg, end)) {
        /* Fast path: no markers, emit lines with softbreaks. */
        FN_OFFSET line_beg = beg;
        for (FN_OFFSET i = beg; i <= end; i++) {
            if (i == end || ctx->text[i] == '\n') {
                if (line_beg < i)
                    CHECK(fn_text_(
                        ctx,
                        FN_TEXT_NORMAL,
                        ctx->text + line_beg,
                        i - line_beg));
                if (i < end) CHECK(fn_text_(ctx, FN_TEXT_SOFTBREAK, NULL, 0));
                line_beg = i + 1;
            }
        }
        return 0;
    }

    /* Slow path */
    FN_OFFSET line_beg = beg;

    for (FN_OFFSET i = beg; i <= end; i++) {
        if (i == end || ctx->text[i] == '\n') {
            if (line_beg < i)
                CHECK(fn_process_inline_line_(ctx, ctx->text, line_beg, i));

            /* Emit softbreak between lines, but not after the last line. */
            if (i < end)
                CHECK(fn_text_(ctx, FN_TEXT_SOFTBREAK, NULL, 0));

            line_beg = i + 1;
        }
    }

    return 0;
}


/* ==============================
 * Block emission
 * ============================== */

/* Emit a complete block: enter_block, inline-processed text, leave_block. */
static int fn_emit_block_(FN_CTX* ctx, FN_BLOCKTYPE type, void* detail,
                          FN_OFFSET text_beg, FN_OFFSET text_end)
{
    CHECK(fn_enter_block_(ctx, type, detail));

    if (text_beg < text_end)
        CHECK(fn_process_inline_(ctx, text_beg, text_end));

    CHECK(fn_leave_block_(ctx, type, detail));
    return 0;
}

/* Emit a block with plain text (no inline processing). Used for boneyard,
 * comments, page break, etc. */
static int fn_emit_block_plain_(FN_CTX* ctx, FN_BLOCKTYPE type, void* detail,
                                FN_OFFSET text_beg, FN_OFFSET text_end)
{
    CHECK(fn_enter_block_(ctx, type, detail));

    if (text_beg < text_end)
        CHECK(fn_text_(ctx, FN_TEXT_NORMAL, ctx->text + text_beg,
                       text_end - text_beg));

    CHECK(fn_leave_block_(ctx, type, detail));
    return 0;
}


/* ==============================
 * Dialogue group buffering
 * ============================== */

static int fn_group_add_(FN_GROUP* g, FN_BLOCKTYPE type,
                         FN_OFFSET beg, FN_OFFSET end, int is_dual)
{
    if (fn_grow_((void**)&g->elems, &g->alloc, g->n + 1,
                 sizeof(FN_BUFFERED)) != 0)
        return -1;

    FN_BUFFERED* e = &g->elems[g->n++];
    e->type = type;
    e->text_beg = beg;
    e->text_end = end;
    e->is_dual = is_dual;
    return 0;
}

static void fn_group_clear_(FN_GROUP* g)
{
    g->n = 0;
}

/* Emit all elements in a dialogue group via callbacks. If force_dual is set,
 * override is_dual_dialogue on the Character element. */
static int fn_emit_group_(FN_CTX* ctx, FN_GROUP* g, int force_dual)
{
    for (unsigned i = 0; i < g->n; i++) {
        FN_BUFFERED* e = &g->elems[i];
        void* detail = NULL;
        FN_BLOCK_CHARACTER_DETAIL char_detail;

        if (e->type == FN_BLOCK_CHARACTER) {
            char_detail.is_dual_dialogue = force_dual ? 1 : e->is_dual;
            detail = &char_detail;
        }

        CHECK(fn_emit_block_(ctx, e->type, detail, e->text_beg, e->text_end));
    }
    return 0;
}

/* Flush the previous dialogue group normally (non-dual). */
static int fn_flush_prev_group_(FN_CTX* ctx)
{
    if (ctx->prev_group.n == 0)
        return 0;

    CHECK(fn_emit_group_(ctx, &ctx->prev_group, 0));
    fn_group_clear_(&ctx->prev_group);
    return 0;
}

/* Complete the current dialogue group. Called when a blank line or
 * end-of-document is reached. */
static int fn_complete_curr_group_(FN_CTX* ctx)
{
    if (ctx->curr_group.n == 0)
        return 0;

    if (ctx->in_dual) {
        /* Second half of dual dialogue. Flush and close container. */
        CHECK(fn_emit_group_(ctx, &ctx->curr_group, 1));
        fn_group_clear_(&ctx->curr_group);
        CHECK(fn_leave_block_(ctx, FN_BLOCK_DUAL_DIALOGUE, NULL));
        ctx->in_dual = 0;
        return 0;
    }

    /* Normal: flush prev, move curr to prev. */
    CHECK(fn_flush_prev_group_(ctx));

    FN_GROUP tmp = ctx->prev_group;
    ctx->prev_group = ctx->curr_group;
    ctx->curr_group = tmp;
    ctx->curr_group.n = 0;

    return 0;
}

/* Flush all pending dialogue state (called at end of body or before
 * non-dialogue blocks). */
static int fn_flush_all_dialogue_(FN_CTX* ctx)
{
    if (ctx->curr_group.n > 0) {
        if (ctx->in_dual) {
            CHECK(fn_emit_group_(ctx, &ctx->curr_group, 1));
            fn_group_clear_(&ctx->curr_group);
            CHECK(fn_leave_block_(ctx, FN_BLOCK_DUAL_DIALOGUE, NULL));
            ctx->in_dual = 0;
        } else {
            CHECK(fn_complete_curr_group_(ctx));
        }
    }

    CHECK(fn_flush_prev_group_(ctx));
    return 0;
}


/* ==============================
 * Title page parsing
 * ============================== */

/* Parse title page entries from lines[0..]. Returns the line index where the
 * body starts. If no title page is found, returns 0. */
static int fn_parse_title_page_(FN_CTX* ctx, unsigned* body_start)
{
    /* Find the first blank line (marks end of title page candidate). */
    unsigned blank_line = 0;
    int found_blank = 0;
    for (unsigned i = 0; i < ctx->n_lines; i++) {
        if (ctx->lines[i].beg == ctx->lines[i].end) {
            blank_line = i;
            found_blank = 1;
            break;
        }
    }

    if (!found_blank) {
        *body_start = 0;
        return 0;
    }

    /* Try to parse lines[0..blank_line) as title entries. */
    int any_found = 0;
    unsigned key_len = 0;
    int have_open_key = 0;

    /* First pass: validate that this looks like a title page. */
    for (unsigned i = 0; i < blank_line; i++) {
        FN_LINE* ln = &ctx->lines[i];
        FN_SIZE len = ln->end - ln->beg;

        if (len == 0)
            continue;

        /* Check for key:value pattern (non-whitespace start, contains colon). */
        if (!fn_isspace_((UCHAR)ctx->text[ln->beg])) {
            int has_colon = 0;
            for (FN_OFFSET c = ln->beg; c < ln->end; c++) {
                if (ctx->text[c] == ':') {
                    if (c > ln->beg) has_colon = 1;
                    break;
                }
            }
            if (has_colon) {
                any_found = 1;
                continue;
            }
        }

        /* Indented line (continuation value) or blank. */
        if (any_found && fn_isspace_((UCHAR)ctx->text[ln->beg]))
            continue;

        if (!any_found) {
            /* First non-key line before finding any keys: not a title page. */
            *body_start = 0;
            return 0;
        }
    }

    if (!any_found) {
        *body_start = 0;
        return 0;
    }

    /* Second pass: emit title page callbacks. */
    CHECK(fn_enter_block_(ctx, FN_BLOCK_TITLE_PAGE, NULL));

    have_open_key = 0;
    FN_BLOCK_TITLE_ENTRY_DETAIL td;
    memset(&td, 0, sizeof(td));

    for (unsigned i = 0; i < blank_line; i++) {
        FN_LINE* ln = &ctx->lines[i];
        FN_SIZE len = ln->end - ln->beg;

        if (len == 0) {
            if (have_open_key) {
                CHECK(fn_leave_block_(ctx, FN_BLOCK_TITLE_ENTRY, &td));
                have_open_key = 0;
            }
            continue;
        }

        /* Try to parse as key:value. */
        FN_OFFSET colon = 0;
        int has_colon = 0;
        if (!fn_isspace_((UCHAR)ctx->text[ln->beg])) {
            for (FN_OFFSET c = ln->beg; c < ln->end; c++) {
                if (ctx->text[c] == ':') {
                    if (c > ln->beg) { colon = c; has_colon = 1; }
                    break;
                }
            }
        }

        if (has_colon) {
            /* Close previous entry if open. */
            if (have_open_key)
                CHECK(fn_leave_block_(ctx, FN_BLOCK_TITLE_ENTRY, &td));

            /* Build lowercased key in the source buffer (mutable copy). */
            for (FN_OFFSET k = ln->beg; k < colon; k++)
                ctx->text[k] = (FN_CHAR)fn_tolower_((UCHAR)ctx->text[k]);

            /* Normalize "author" to "authors". */
            key_len = colon - ln->beg;
            int is_author = (key_len == 6
                && ctx->text[ln->beg] == 'a' && ctx->text[ln->beg + 1] == 'u'
                && ctx->text[ln->beg + 2] == 't' && ctx->text[ln->beg + 3] == 'h'
                && ctx->text[ln->beg + 4] == 'o' && ctx->text[ln->beg + 5] == 'r');

            td.key = ctx->text + ln->beg;
            td.key_size = key_len;

            if (is_author) {
                /* Write "authors" into source. Safe: we have at least
                 * "author:" which is 7 chars; "authors" is also 7. */
                ctx->text[ln->beg + 6] = 's';
                td.key_size = 7;
            }

            CHECK(fn_enter_block_(ctx, FN_BLOCK_TITLE_ENTRY, &td));
            have_open_key = 1;

            /* Check for inline value after colon. */
            FN_OFFSET val_beg = fn_skip_ws_(ctx->text, ln->end, colon + 1);
            if (val_beg < ln->end) {
                /* Inline value: emit and close immediately. */
                CHECK(fn_text_(ctx, FN_TEXT_NORMAL, ctx->text + val_beg,
                               ln->end - val_beg));
                CHECK(fn_leave_block_(ctx, FN_BLOCK_TITLE_ENTRY, &td));
                have_open_key = 0;
            }
            /* Otherwise it's a directive (value on subsequent lines). */

        } else if (have_open_key) {
            /* Continuation value line (indented). Trim and emit. */
            FN_OFFSET vbeg = ln->beg;
            FN_OFFSET vend = ln->end;
            fn_trim_(ctx->text, &vbeg, &vend);
            if (vbeg < vend) {
                CHECK(fn_text_(ctx, FN_TEXT_SOFTBREAK, NULL, 0));
                CHECK(fn_text_(ctx, FN_TEXT_NORMAL, ctx->text + vbeg,
                               vend - vbeg));
            }
        }
    }

    if (have_open_key)
        CHECK(fn_leave_block_(ctx, FN_BLOCK_TITLE_ENTRY, &td));

    CHECK(fn_leave_block_(ctx, FN_BLOCK_TITLE_PAGE, NULL));

    *body_start = blank_line;
    return 0;
}


/* ==============================
 * Body parsing
 * ============================== */

static int fn_parse_body_(FN_CTX* ctx, unsigned start_line)
{
    /* Prepend a virtual blank line so the first real line is "preceded by
     * a blank". The C++ parser does this with `source.insert(begin, '\n')`.
     * We achieve it by starting newlines_before at 1. */
    unsigned newlines_before = 1;
    int is_comment_block = 0;
    FN_OFFSET comment_beg = 0;
    FN_OFFSET comment_end = 0;

    for (unsigned i = start_line; i < ctx->n_lines; i++) {
        FN_OFFSET lbeg = ctx->lines[i].beg;
        FN_OFFSET lend = ctx->lines[i].end;
        FN_SIZE llen = lend - lbeg;

        int no_lower = -1;
#define NO_LOWER_()                                                            \
    (no_lower >= 0 ? no_lower                                                  \
                   : (no_lower = fn_has_no_lowercase_(ctx->text, lbeg, lend)))

        /* ----- Lyrics ----- */
        if (llen > 0 && ctx->text[lbeg] == '~') {
            CHECK(fn_flush_all_dialogue_(ctx));

            if (ctx->last_block == FN_BLOCK_LYRICS && newlines_before > 0) {
                CHECK(fn_enter_block_(ctx, FN_BLOCK_LYRICS_SPACER, NULL));
                CHECK(fn_leave_block_(ctx, FN_BLOCK_LYRICS_SPACER, NULL));
            }

            CHECK(fn_emit_block_(ctx, FN_BLOCK_LYRICS, NULL,
                                 lbeg + 1, lend));
            ctx->last_block = FN_BLOCK_LYRICS;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Forced action ----- */
        if (llen > 0 && ctx->text[lbeg] == '!') {
            CHECK(fn_flush_all_dialogue_(ctx));

            FN_BLOCK_ACTION_DETAIL ad = { 0 };
            CHECK(fn_emit_block_(ctx, FN_BLOCK_ACTION, &ad,
                                 lbeg + 1, lend));
            ctx->last_block = FN_BLOCK_ACTION;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Forced character ----- */
        if (llen > 0 && ctx->text[lbeg] == '@') {
            CHECK(fn_flush_all_dialogue_(ctx));

            /* Start a new dialogue group. */
            CHECK(fn_complete_curr_group_(ctx));
            fn_group_clear_(&ctx->curr_group);
            if (fn_group_add_(&ctx->curr_group, FN_BLOCK_CHARACTER,
                              lbeg + 1, lend, 0) != 0)
                return -1;

            ctx->in_dialogue = 1;
            ctx->last_block = FN_BLOCK_CHARACTER;
            newlines_before = 0;
            continue;
        }

        /* ----- Empty line in dialogue (exactly 2 whitespace chars) ----- */
        if (ctx->in_dialogue
            && fn_is_ws_of_len_(ctx->text, lbeg, lend, 2, 1)) {
            /* Append to current dialogue in the group. */
            if (ctx->curr_group.n > 0) {
                FN_BUFFERED* last = &ctx->curr_group.elems[ctx->curr_group.n - 1];
                if (last->type == FN_BLOCK_DIALOGUE) {
                    /* Extend to include this line (the \n before it is part
                     * of the source). */
                    last->text_end = lend;
                } else {
                    if (fn_group_add_(&ctx->curr_group, FN_BLOCK_DIALOGUE,
                                      lbeg, lend, 0) != 0)
                        return -1;
                }
            }
            newlines_before = 0;
            continue;
        }

        /* ----- Whitespace-only action (2+ chars) ----- */
        if (fn_is_ws_of_len_(ctx->text, lbeg, lend, 2, 0)) {
            CHECK(fn_flush_all_dialogue_(ctx));

            FN_BLOCK_ACTION_DETAIL ad = { 0 };
            CHECK(fn_emit_block_plain_(ctx, FN_BLOCK_ACTION, &ad, lbeg, lend));
            ctx->last_block = FN_BLOCK_ACTION;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Blank line ----- */
        if (llen == 0 && !is_comment_block) {
            ctx->in_dialogue = 0;

            /* Complete any in-progress dialogue group. */
            if (ctx->curr_group.n > 0)
                CHECK(fn_complete_curr_group_(ctx));

            newlines_before++;
            continue;
        }

        /* ----- Open boneyard ----- */
        if (llen >= 2 && ctx->text[lbeg] == '/' && ctx->text[lbeg + 1] == '*') {
            if (fn_ends_with_boneyard_close_(ctx->text, lbeg, lend)) {
                /* Single-line boneyard. */
                CHECK(fn_flush_all_dialogue_(ctx));

                if (ctx->flags & FN_FLAG_BONEYARD) {
                    FN_OFFSET tb = lbeg + 2;
                    FN_OFFSET te = lend;
                    /* Strip trailing close marker. */
                    te = fn_rskip_ws_(ctx->text, tb, te);
                    if (te >= tb + 2 && ctx->text[te - 1] == '/'
                        && ctx->text[te - 2] == '*')
                        te -= 2;
                    fn_trim_(ctx->text, &tb, &te);
                    CHECK(fn_emit_block_plain_(ctx, FN_BLOCK_BONEYARD, NULL,
                                               tb, te));
                }

                is_comment_block = 0;
                ctx->last_block = FN_BLOCK_BONEYARD;
                newlines_before = 0;
            } else {
                is_comment_block = 1;
                comment_beg = lbeg + 2;
                comment_end = comment_beg;
            }
            continue;
        }

        /* ----- Close boneyard ----- */
        if (fn_ends_with_boneyard_close_(ctx->text, lbeg, lend)) {
            /* Accumulate last line content (before the * /) if blank. */
            FN_OFFSET te = lend;
            te = fn_rskip_ws_(ctx->text, lbeg, te);
            if (te >= lbeg + 2 && ctx->text[te - 1] == '/'
                && ctx->text[te - 2] == '*')
                te -= 2;

            is_comment_block = 0;

            CHECK(fn_flush_all_dialogue_(ctx));

            if (ctx->flags & FN_FLAG_BONEYARD) {
                /* comment_beg..comment_end has accumulated content. */
                FN_OFFSET tb = comment_beg;
                FN_OFFSET te2 = comment_end;
                fn_trim_(ctx->text, &tb, &te2);
                CHECK(fn_emit_block_plain_(ctx, FN_BLOCK_BONEYARD, NULL,
                                           tb, te2));
            }

            ctx->last_block = FN_BLOCK_BONEYARD;
            newlines_before = 0;
            continue;
        }

        /* ----- Inside boneyard ----- */
        if (is_comment_block) {
            comment_end = lend + 1; /* include trailing \n */
            continue;
        }

        /* ----- Page break (3+ consecutive '=') ----- */
        if (llen >= 3 && ctx->text[lbeg] == '=') {
            FN_OFFSET p = lbeg;
            while (p < lend && ctx->text[p] == '=')
                p++;

            if (p - lbeg >= 3) {
                /* Rest must be whitespace only. */
                int all_ws = 1;
                for (FN_OFFSET k = p; k < lend; k++) {
                    if (!fn_isspace_((UCHAR)ctx->text[k])) {
                        all_ws = 0;
                        break;
                    }
                }
                if (all_ws) {
                    CHECK(fn_flush_all_dialogue_(ctx));
                    CHECK(fn_enter_block_(ctx, FN_BLOCK_PAGE_BREAK, NULL));
                    CHECK(fn_leave_block_(ctx, FN_BLOCK_PAGE_BREAK, NULL));
                    ctx->last_block = FN_BLOCK_PAGE_BREAK;
                    newlines_before = 0;
                    ctx->in_dialogue = 0;
                    continue;
                }
            }
        }

        /* ----- Synopsis ('=' but not page break) ----- */
        {
            FN_OFFSET tbeg = lbeg;
            FN_OFFSET tend = lend;
            fn_trim_(ctx->text, &tbeg, &tend);

            if (tbeg < tend && ctx->text[tbeg] == '=') {
                CHECK(fn_flush_all_dialogue_(ctx));

                FN_OFFSET text_beg = tbeg + 1;
                text_beg = fn_skip_ws_(ctx->text, lend, text_beg);

                if (ctx->flags & FN_FLAG_SYNOPSES)
                    CHECK(fn_emit_block_plain_(ctx, FN_BLOCK_SYNOPSIS, NULL,
                                               text_beg, tend));

                ctx->last_block = FN_BLOCK_SYNOPSIS;
                newlines_before = 0;
                ctx->in_dialogue = 0;
                continue;
            }
        }

        /* ----- Comment ([[text]] on a single line) ----- */
        if (newlines_before > 0) {
            FN_OFFSET tbeg = lbeg;
            FN_OFFSET tend = lend;
            fn_trim_(ctx->text, &tbeg, &tend);

            if (tend - tbeg >= 4
                && ctx->text[tbeg] == '[' && ctx->text[tbeg + 1] == '['
                && ctx->text[tend - 1] == ']' && ctx->text[tend - 2] == ']') {
                /* Verify no ']' in inner content. */
                FN_OFFSET inner_beg = tbeg + 2;
                FN_OFFSET inner_end = tend - 2;
                int has_bracket = 0;
                for (FN_OFFSET k = inner_beg; k < inner_end; k++) {
                    if (ctx->text[k] == ']') { has_bracket = 1; break; }
                }

                if (!has_bracket) {
                    CHECK(fn_flush_all_dialogue_(ctx));

                    if (ctx->flags & FN_FLAG_COMMENTS) {
                        fn_trim_(ctx->text, &inner_beg, &inner_end);
                        CHECK(fn_emit_block_plain_(ctx, FN_BLOCK_COMMENT, NULL,
                                                   inner_beg, inner_end));
                    }

                    ctx->last_block = FN_BLOCK_COMMENT;
                    continue;
                }
            }
        }

        /* ----- Section heading (#) ----- */
        {
            FN_OFFSET tbeg = lbeg;
            FN_OFFSET tend = lend;
            fn_trim_(ctx->text, &tbeg, &tend);

            if (tbeg < tend && ctx->text[tbeg] == '#') {
                CHECK(fn_flush_all_dialogue_(ctx));

                int depth = 0;
                FN_OFFSET p = tbeg;
                while (p < tend && ctx->text[p] == '#') {
                    depth++;
                    p++;
                }

                FN_OFFSET text_beg = fn_skip_ws_(ctx->text, tend, p);
                if (text_beg < tend) {
                    if (ctx->flags & FN_FLAG_SECTIONS) {
                        FN_BLOCK_SECTION_HEADING_DETAIL sd;
                        sd.depth = depth;
                        CHECK(fn_emit_block_plain_(ctx, FN_BLOCK_SECTION_HEADING,
                                                   &sd, text_beg, tend));
                    }
                }

                ctx->last_block = FN_BLOCK_SECTION_HEADING;
                newlines_before = 0;
                ctx->in_dialogue = 0;
                continue;
            }
        }

        /* ----- Forced scene heading ('.' but not '..') ----- */
        if (llen > 1 && ctx->text[lbeg] == '.' && ctx->text[lbeg + 1] != '.') {
            CHECK(fn_flush_all_dialogue_(ctx));

            FN_OFFSET text_beg = lbeg + 1;
            FN_OFFSET text_end = lend;
            fn_trim_(ctx->text, &text_beg, &text_end);

            FN_BLOCK_SCENE_HEADING_DETAIL sd;
            memset(&sd, 0, sizeof(sd));
            fn_extract_scene_number_(ctx->text, text_beg, &text_end,
                                     &sd.scene_number,
                                     &sd.scene_number_size);

            CHECK(fn_emit_block_(ctx, FN_BLOCK_SCENE_HEADING, &sd,
                                 text_beg, text_end));
            ctx->last_block = FN_BLOCK_SCENE_HEADING;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Scene heading (INT./EXT./EST.) ----- */
        if (newlines_before > 0
            && fn_matches_scene_heading_(ctx->text, lbeg, lend)) {
            CHECK(fn_flush_all_dialogue_(ctx));

            FN_OFFSET text_end = lend;
            FN_BLOCK_SCENE_HEADING_DETAIL sd;
            memset(&sd, 0, sizeof(sd));
            fn_extract_scene_number_(ctx->text, lbeg, &text_end,
                                     &sd.scene_number,
                                     &sd.scene_number_size);

            CHECK(fn_emit_block_(ctx, FN_BLOCK_SCENE_HEADING, &sd,
                                 lbeg, text_end));
            ctx->last_block = FN_BLOCK_SCENE_HEADING;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Transition (all-caps ending in "TO:") ----- */
        if (newlines_before > 0 && llen >= 3 && ctx->text[lend - 1] == ':'
            && ctx->text[lend - 2] == 'O' && ctx->text[lend - 3] == 'T'
            && NO_LOWER_()) {
            CHECK(fn_flush_all_dialogue_(ctx));
            CHECK(fn_emit_block_(ctx, FN_BLOCK_TRANSITION, NULL, lbeg, lend));
            ctx->last_block = FN_BLOCK_TRANSITION;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Known transitions ----- */
        if (newlines_before > 0) {
            FN_OFFSET tbeg = fn_skip_ws_(ctx->text, lend, lbeg);
            FN_SIZE tlen = lend - tbeg;
            int is_known = 0;

            if (tlen == 9 && memcmp(ctx->text + tbeg, "FADE OUT.", 9) == 0)
                is_known = 1;
            else if (tlen == 13 && memcmp(ctx->text + tbeg, "CUT TO BLACK.", 13) == 0)
                is_known = 1;
            else if (tlen == 14 && memcmp(ctx->text + tbeg, "FADE TO BLACK.", 14) == 0)
                is_known = 1;

            if (is_known) {
                CHECK(fn_flush_all_dialogue_(ctx));
                CHECK(fn_emit_block_(ctx, FN_BLOCK_TRANSITION, NULL, lbeg, lend));
                ctx->last_block = FN_BLOCK_TRANSITION;
                newlines_before = 0;
                ctx->in_dialogue = 0;
                continue;
            }
        }

        /* ----- Forced transition ('>') or centered text ('>...<') ----- */
        if (llen > 0 && ctx->text[lbeg] == '>') {
            CHECK(fn_flush_all_dialogue_(ctx));

            if (llen > 1 && ctx->text[lend - 1] == '<') {
                /* Centered text. */
                FN_OFFSET text_beg = lbeg + 1;
                FN_OFFSET text_end = lend - 1;
                fn_trim_(ctx->text, &text_beg, &text_end);

                FN_BLOCK_ACTION_DETAIL ad;
                ad.is_centered = 1;
                CHECK(fn_emit_block_(ctx, FN_BLOCK_ACTION, &ad,
                                     text_beg, text_end));
            } else {
                /* Forced transition. */
                FN_OFFSET text_beg = lbeg + 1;
                FN_OFFSET text_end = lend;
                fn_trim_(ctx->text, &text_beg, &text_end);
                CHECK(fn_emit_block_(ctx, FN_BLOCK_TRANSITION, NULL,
                                     text_beg, text_end));
            }

            ctx->last_block = FN_BLOCK_TRANSITION;
            newlines_before = 0;
            ctx->in_dialogue = 0;
            continue;
        }

        /* ----- Character cue ----- */
        if (newlines_before > 0 && NO_LOWER_()
            && fn_matches_character_cue_(ctx->text, lbeg, lend)) {
            /* Only a character if next line is non-empty. */
            if (i + 1 < ctx->n_lines
                && ctx->lines[i + 1].beg < ctx->lines[i + 1].end) {

                FN_OFFSET text_beg = lbeg;
                FN_OFFSET text_end = lend;
                int is_dual = 0;

                /* Check for dual dialogue marker (trailing ^). */
                FN_OFFSET trimmed_end = fn_rskip_ws_(ctx->text, lbeg, lend);
                if (trimmed_end > lbeg
                    && ctx->text[trimmed_end - 1] == '^') {
                    is_dual = 1;
                    text_end = fn_rskip_ws_(ctx->text, lbeg, trimmed_end - 1);
                }

                if (is_dual && ctx->prev_group.n > 0) {
                    /* Dual dialogue: prev_group is the first half.
                     * Open the container and emit it now. */
                    CHECK(fn_enter_block_(ctx, FN_BLOCK_DUAL_DIALOGUE, NULL));
                    CHECK(fn_emit_group_(ctx, &ctx->prev_group, 1));
                    fn_group_clear_(&ctx->prev_group);
                    ctx->in_dual = 1;
                } else {
                    /* Normal: flush any previous group. */
                    CHECK(fn_flush_prev_group_(ctx));
                }

                fn_group_clear_(&ctx->curr_group);
                if (fn_group_add_(&ctx->curr_group, FN_BLOCK_CHARACTER,
                                  text_beg, text_end, is_dual) != 0)
                    return -1;

                ctx->in_dialogue = 1;
                ctx->last_block = FN_BLOCK_CHARACTER;
                newlines_before = 0;
                continue;
            }
        }

        /* ----- Dialogue / Parenthetical ----- */
        if (ctx->in_dialogue) {
            FN_OFFSET trimmed_beg = fn_skip_ws_(ctx->text, lend, lbeg);

            if (newlines_before == 0 && trimmed_beg < lend
                && ctx->text[trimmed_beg] == '(') {
                /* Parenthetical. */
                if (fn_group_add_(&ctx->curr_group, FN_BLOCK_PARENTHETICAL,
                                  lbeg, lend, 0) != 0)
                    return -1;
            } else {
                /* Dialogue. Merge consecutive dialogue lines. */
                if (ctx->curr_group.n > 0) {
                    FN_BUFFERED* last =
                        &ctx->curr_group.elems[ctx->curr_group.n - 1];
                    if (last->type == FN_BLOCK_DIALOGUE) {
                        last->text_end = lend;
                    } else {
                        if (fn_group_add_(&ctx->curr_group, FN_BLOCK_DIALOGUE,
                                          lbeg, lend, 0) != 0)
                            return -1;
                    }
                } else {
                    if (fn_group_add_(&ctx->curr_group, FN_BLOCK_DIALOGUE,
                                      lbeg, lend, 0) != 0)
                        return -1;
                }
            }

            ctx->last_block = FN_BLOCK_DIALOGUE;
            newlines_before = 0;
            continue;
        }

        /* ----- Continuation (no blank line, no pattern match) ----- */
        /* This is handled implicitly: since we emit blocks immediately
         * (or buffer dialogue groups), multi-line action is handled by
         * the fallback below accumulating text until a blank line.
         *
         * Scene heading demotion: if the previous block was a scene heading
         * and we reach here with newlines_before == 0, the scene heading
         * was already emitted. We can't un-emit it. For correctness, we'd
         * need to buffer scene headings too. For now, we match the common
         * case and accept this edge case deviation.
         *
         * TODO: Buffer one non-dialogue block for scene heading demotion. */

        /* ----- Action (fallback) ----- */
        {
            CHECK(fn_flush_all_dialogue_(ctx));

            FN_OFFSET text_beg = lbeg;
            FN_OFFSET text_end = lend;
            fn_trim_(ctx->text, &text_beg, &text_end);

            FN_BLOCK_ACTION_DETAIL ad = { 0 };
            CHECK(fn_emit_block_(ctx, FN_BLOCK_ACTION, &ad,
                                 text_beg, text_end));
            ctx->last_block = FN_BLOCK_ACTION;
            newlines_before = 0;
            ctx->in_dialogue = 0;
        }

        #undef NO_LOWER_
    }

    /* Flush any remaining dialogue state. */
    CHECK(fn_flush_all_dialogue_(ctx));

    return 0;
}


/* ==============================
 * Entry point
 * ============================== */

int fn_parse(const FN_CHAR* text, FN_SIZE size, const FN_PARSER* parser,
             void* userdata, unsigned flags)
{
    FN_CTX ctx;
    int ret = 0;

    memset(&ctx, 0, sizeof(ctx));
    ctx.parser = parser;
    ctx.userdata = userdata;
    ctx.flags = flags;

    /* Normalize input and build line table. */
    if (fn_init_input_(&ctx, text, size) != 0) {
        ret = -1;
        goto cleanup;
    }

    /* Open the document. */
    if ((ret = fn_enter_block_(&ctx, FN_BLOCK_DOC, NULL)) != 0)
        goto cleanup;

    /* Parse title page (may consume leading lines). */
    unsigned body_start = 0;
    if ((ret = fn_parse_title_page_(&ctx, &body_start)) != 0)
        goto cleanup;

    /* Parse body. */
    if ((ret = fn_parse_body_(&ctx, body_start)) != 0)
        goto cleanup;

    /* Close the document. */
    if ((ret = fn_leave_block_(&ctx, FN_BLOCK_DOC, NULL)) != 0)
        goto cleanup;

cleanup:
    free(ctx.text);
    free(ctx.lines);
    free(ctx.prev_group.elems);
    free(ctx.curr_group.elems);
    free(ctx.consumed);
    free(ctx.marks);
    return ret;
}
