/*
 * fountain-html.h -- Fountain to HTML renderer
 *
 * Companion to fountain.h. Renders a Fountain document as flow HTML (no
 * pagination). Only body-level content is generated; the caller provides
 * HTML header/footer and CSS.
 *
 * Modeled after md4c-html by Martin Mitas (https://github.com/mity/md4c).
 *
 * MIT License
 *
 * Copyright (c) 2026 fairybow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

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


/* Returns a pointer to a static null-terminated string containing the
 * default CSS for styling fn_html() output. The caller can embed this
 * in a <style> tag or use their own stylesheet. */
const char* fn_html_css(void);


#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif  /* FOUNTAIN_HTML_H */
