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

/// Replace ^

// clang-format off

#ifndef FOUNTAIN_H
#define FOUNTAIN_H

#ifdef __cplusplus
    extern "C" {
#endif


typedef char        FN_CHAR;
typedef unsigned    FN_SIZE;
typedef unsigned    FN_OFFSET;


/* ***************************
 * Block-level element types *
 * ***************************/

/* Block represents a structural element of a Fountain screenplay. Some block
 * types have an associated detail structure providing additional metadata; see
 * the individual entries below.
 *
 * The parser emits enter_block/leave_block pairs for each element. Text
 * content between enter/leave is delivered via the text callback with
 * Fountain control characters (!, ., ~, @, >) already stripped.
 *
 * Nesting is minimal. FN_BLOCK_DOC wraps everything. FN_BLOCK_TITLE_PAGE
 * wraps title entries. FN_BLOCK_DUAL_DIALOGUE wraps paired character/dialogue
 * groups. All other blocks are leaf-level.
 */
typedef enum FN_BLOCKTYPE {
    /* Wraps the entire document.
     * enter_block(FN_BLOCK_DOC) is the first callback; leave_block is the
     * last. No detail. */
    FN_BLOCK_DOC = 0,

    /* Wraps all title page entries, if a title page is present.
     * Emitted before any body blocks. No detail. */
    FN_BLOCK_TITLE_PAGE,

    /* A single title page key-value pair (e.g., "Title: My Script").
     * Always nested inside FN_BLOCK_TITLE_PAGE. Text callbacks deliver the
     * value(s); for multi-line values, each line is a separate text callback.
     * Detail: FN_BLOCK_TITLE_ENTRY_DETAIL. */
    FN_BLOCK_TITLE_ENTRY,

    /* Scene heading (INT./EXT./EST., or forced with leading '.').
     * Detail: FN_BLOCK_SCENE_HEADING_DETAIL. */
    FN_BLOCK_SCENE_HEADING,

    /* Action block. Fallback type for unclassified lines.
     * Detail: FN_BLOCK_ACTION_DETAIL. */
    FN_BLOCK_ACTION,

    /* Character cue (all-caps name preceding dialogue).
     * Detail: FN_BLOCK_CHARACTER_DETAIL. */
    FN_BLOCK_CHARACTER,

    /* Dialogue text following a character cue. No detail. */
    FN_BLOCK_DIALOGUE,

    /* Parenthetical direction within dialogue. No detail. */
    FN_BLOCK_PARENTHETICAL,

    /* Transition (CUT TO:, FADE OUT., etc., or forced with leading '>').
     * No detail. */
    FN_BLOCK_TRANSITION,

    /* Lyrics line (leading '~'). No detail. */
    FN_BLOCK_LYRICS,

    /* Empty line between lyric stanzas. No text content. No detail. */
    FN_BLOCK_LYRICS_SPACER,

    /* Explicit page break (three or more consecutive '='). No detail. */
    FN_BLOCK_PAGE_BREAK,

    /* Boneyard comment (/ * ... * / blocks). Only emitted when
     * FN_FLAG_BONEYARD is set. No detail. */
    FN_BLOCK_BONEYARD,

    /* Inline comment ([[...]] on its own line). Only emitted when
     * FN_FLAG_COMMENTS is set. No detail. */
    FN_BLOCK_COMMENT,

    /* Section heading (leading '#' characters, organizational metadata).
     * Only emitted when FN_FLAG_SECTIONS is set.
     * Detail: FN_BLOCK_SECTION_HEADING_DETAIL. */
    FN_BLOCK_SECTION_HEADING,

    /* Synopsis (leading '=', organizational metadata). Only emitted when
     * FN_FLAG_SYNOPSES is set. No detail. */
    FN_BLOCK_SYNOPSIS,

    /* Container wrapping two character/dialogue groups joined by the dual
     * dialogue marker ('^' on the second character cue). The parser buffers
     * one dialogue group internally to detect the '^' lookahead before
     * emitting. No detail. */
    FN_BLOCK_DUAL_DIALOGUE
} FN_BLOCKTYPE;


/* Block detail structures */

typedef struct FN_BLOCK_TITLE_ENTRY_DETAIL {
    /* Key portion of the title entry (e.g., "Title", "Author").
     * Offsets point into the original source text. The key is lowercased
     * by the parser; "Author" is normalized to "authors". */
    FN_OFFSET key_offset;
    FN_SIZE key_size;
} FN_BLOCK_TITLE_ENTRY_DETAIL;

typedef struct FN_BLOCK_SCENE_HEADING_DETAIL {
    /* Scene number extracted from trailing #number# syntax.
     * Both fields are zero when no scene number is present. */
    FN_OFFSET scene_number_offset;
    FN_SIZE scene_number_size;
} FN_BLOCK_SCENE_HEADING_DETAIL;

typedef struct FN_BLOCK_ACTION_DETAIL {
    /* Non-zero if the action was produced by >centered< syntax. */
    int is_centered;
} FN_BLOCK_ACTION_DETAIL;

typedef struct FN_BLOCK_CHARACTER_DETAIL {
    /* Non-zero if this character cue is part of a dual dialogue pair.
     * The '^' marker is on the second cue, but the parser sets this flag
     * on both characters in the pair. */
    int is_dual_dialogue;
} FN_BLOCK_CHARACTER_DETAIL;

typedef struct FN_BLOCK_SECTION_HEADING_DETAIL {
    /* Depth of the section heading (1-6), determined by the number of
     * leading '#' characters. */
    int depth;
} FN_BLOCK_SECTION_HEADING_DETAIL;


/* ********************
 * Inline span types  *
 * ********************/

/* Spans represent inline formatting within block content. The parser
 * processes inline markup after block classification, emitting
 * enter_span/leave_span pairs around the formatted content.
 *
 * Fountain supports nesting (e.g., bold-italic with ***), so spans may
 * appear inside other spans. The parser emits the most specific compound
 * markers first, then processes remaining markers in decreasing specificity:
 *     bold-underline-italic (_***), bold-italic (***), bold-underline (_**),
 *     underline-italic (_*), bold (**), italic (*), underline (_).
 */
typedef enum FN_SPANTYPE {
    FN_SPAN_EMPHASIS,       /* *italic* */
    FN_SPAN_STRONG,         /* **bold** */
    FN_SPAN_UNDERLINE,      /* _underline_ */
    FN_SPAN_NOTE            /* [[note]] -- only emitted when FN_FLAG_NOTES is set */
} FN_SPANTYPE;


/* ***********************
 * Text content types    *
 * ***********************/

typedef enum FN_TEXTTYPE {
    /* Normal text content. Fountain control characters have been stripped;
     * the text is otherwise unmodified (no HTML escaping, no case changes).
     * Inline [[notes]] are silently removed unless FN_FLAG_NOTES is set,
     * in which case note content is delivered as FN_TEXT_NORMAL between
     * enter_span(FN_SPAN_NOTE) and leave_span(FN_SPAN_NOTE). */
    FN_TEXT_NORMAL = 0,

    /* A line break within a block (e.g., multi-line dialogue or action).
     * The consumer may render this as <br>, a literal newline, or ignore it
     * depending on the output format. No text content accompanies this. */
    FN_TEXT_SOFTBREAK
} FN_TEXTTYPE;


/* ***************
 * Parser flags  *
 * ***************/

/* By default, the parser skips organizational and hidden elements (boneyard,
 * comments, section headings, synopses, notes). These flags enable their
 * emission. */

#define FN_FLAG_BONEYARD        0x0001  /* Emit FN_BLOCK_BONEYARD blocks */
#define FN_FLAG_COMMENTS        0x0002  /* Emit FN_BLOCK_COMMENT blocks */
#define FN_FLAG_SECTIONS        0x0004  /* Emit FN_BLOCK_SECTION_HEADING blocks */
#define FN_FLAG_SYNOPSES        0x0008  /* Emit FN_BLOCK_SYNOPSIS blocks */
#define FN_FLAG_NOTES           0x0010  /* Emit inline [[note]] spans */

/* Convenience: emit everything. */
#define FN_FLAG_ALL_META        (FN_FLAG_BONEYARD | FN_FLAG_COMMENTS \
                                | FN_FLAG_SECTIONS | FN_FLAG_SYNOPSES \
                                | FN_FLAG_NOTES)


/* **********************
 * Parser interface     *
 * **********************/

/* Structure holding the callback functions. The parser calls these during
 * fn_parse() to report document structure and content. Any callback may be
 * NULL, in which case the corresponding events are silently ignored.
 *
 * All callbacks return 0 on success. Returning non-zero aborts parsing;
 * fn_parse() propagates the non-zero value to its caller. */
typedef struct FN_PARSER {
    /* Called when a block-level element opens. `detail` points to the
     * corresponding FN_BLOCK_*_DETAIL struct for block types that have one,
     * or NULL otherwise. The detail pointer is only valid for the duration
     * of the callback. */
    int (*enter_block)(FN_BLOCKTYPE type, void* detail, void* userdata);

    /* Called when a block-level element closes. `detail` matches the
     * corresponding enter_block call. */
    int (*leave_block)(FN_BLOCKTYPE type, void* detail, void* userdata);

    /* Called when an inline span opens. No detail structs for spans. */
    int (*enter_span)(FN_SPANTYPE type, void* userdata);

    /* Called when an inline span closes. */
    int (*leave_span)(FN_SPANTYPE type, void* userdata);

    /* Called to deliver text content. `text` points into an internal buffer
     * (not necessarily into the original source, since inline formatting
     * requires processing). `size` is the byte count. The pointer is only
     * valid for the duration of the callback. */
    int (*text)(FN_TEXTTYPE type, const FN_CHAR* text, FN_SIZE size,
                void* userdata);
} FN_PARSER;


/* ***********************
 * Entry point           *
 * ***********************/

/* Parse a Fountain document.
 *
 * `text` and `size` specify the input. The input is not required to be
 * null-terminated. Line endings (\r, \r\n, \n) are normalized internally.
 *
 * `parser` provides the callback functions. `userdata` is passed through
 * to every callback.
 *
 * `flags` is a bitmask of FN_FLAG_* values controlling which optional
 * element types are emitted.
 *
 * Returns 0 on success.
 * Returns -1 if a runtime error occurs (e.g., memory allocation failure).
 * If any callback returns non-zero, parsing is aborted and that value is
 * returned. */
int fn_parse(const FN_CHAR* text, FN_SIZE size, const FN_PARSER* parser,
             void* userdata, unsigned flags);


#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif  /* FOUNTAIN_H */

// clang-format on