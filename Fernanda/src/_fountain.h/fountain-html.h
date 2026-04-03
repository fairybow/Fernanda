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

#ifndef FOUNTAIN_HTML_H
#define FOUNTAIN_HTML_H

#include "fountain.h"

#ifdef __cplusplus
    extern "C" {
#endif


/* Render flags */
#define FN_HTML_FLAG_DEBUG          0x0001  /* Log debug output to stderr */
#define FN_HTML_FLAG_SKIP_TITLE     0x0002  /* Omit title page from output */
#define FN_HTML_FLAG_SKIP_BODY      0x0004  /* Omit body from output */


/* Render a Fountain document as HTML.
 *
 * Only content markup is generated (no <html>, <head>, <body>, or <style>
 * tags). The caller wraps the output in a complete HTML document and
 * provides CSS. Title page content is wrapped in a <div id="script-title">
 * container. Body content is wrapped in <section> tags.
 *
 * `input` and `input_size` specify the Fountain source text.
 *
 * `process_output` is called with chunks of HTML output. A typical
 * implementation appends the bytes to a buffer or writes them to a file.
 * `userdata` is passed through to the callback.
 *
 * `parser_flags` is a bitmask of FN_FLAG_* values from fountain.h,
 * propagated to fn_parse().
 *
 * `renderer_flags` is a bitmask of FN_HTML_FLAG_* values.
 *
 * Returns 0 on success.
 * Returns -1 on error (if fn_parse() fails). */
int fn_html(const FN_CHAR* input, FN_SIZE input_size,
            void (*process_output)(const FN_CHAR*, FN_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);


#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif  /* FOUNTAIN_HTML_H */

// clang-format off