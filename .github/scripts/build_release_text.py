#!/usr/bin/env python3
"""
Assemble a GitHub Release body and title from CHANGELOG.md.

The CHANGELOG is expected to contain three marker-delimited blocks, reused by
every release: the preamble, the known issues, and the footer. Then a
releases anchor, followed by per-version entries whose headings end in
" - tag v<VERSION>". The body is the preamble, the entry, the known issues,
and the footer, in that order. Entries carry no known issues of their own:
the one list is kept in its block, and each release gets it as it stands at
the tagged commit.

Entries are only searched for after the releases anchor, so anything above
it (e.g. a template entry) can never match. An entry's body runs to the next
entry heading, or to the end of the file.

Outputs:
  - RELEASE_BODY.md (or path given by --output)
  - release_title=<title> appended to $GITHUB_OUTPUT (if set)
  - release_title also printed to stdout for visibility
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import NoReturn


PREAMBLE_START = "<!-- release-preamble-start -->"
PREAMBLE_END = "<!-- release-preamble-end -->"
KNOWN_ISSUES_START = "<!-- release-known-issues-start -->"
KNOWN_ISSUES_END = "<!-- release-known-issues-end -->"
FOOTER_START = "<!-- release-footer-start -->"
FOOTER_END = "<!-- release-footer-end -->"
RELEASES_ANCHOR = '<a id="releases"></a>'

# Any release entry heading (used as the end boundary of an entry's body)
ENTRY_HEADING = re.compile(r"^# .+? - tag v\S+\s*$", re.MULTILINE)

# Separator lines between entries, stripped from the end of a body
TRAILING_RULES = re.compile(r"(?:\n\s*---\s*)+\Z")


def die(msg: str) -> NoReturn:
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def extract_between(text: str, start: str, end: str, label: str) -> str:
    s = text.find(start)
    if s == -1:
        die(f"missing marker {start!r} (for {label})")
    e = text.find(end, s + len(start))
    if e == -1:
        die(f"missing marker {end!r} (for {label})")
    return text[s + len(start):e].strip("\n")


def find_entry(text: str, version: str) -> tuple[str, str]:
    """
    Locate the release entry for `version` below the releases anchor and
    return (title, body).
    """
    a = text.find(RELEASES_ANCHOR)
    if a == -1:
        die(f"missing marker {RELEASES_ANCHOR!r} (for releases)")
    releases = text[a + len(RELEASES_ANCHOR):]

    pattern = re.compile(
        rf"^# (?P<title>.+?) - tag v{re.escape(version)}\s*$",
        re.MULTILINE,
    )
    matches = list(pattern.finditer(releases))

    if not matches:
        die(f"no entry found whose heading ends with ' - tag v{version}'")
    if len(matches) > 1:
        die(f"{len(matches)} entries found whose heading ends with ' - tag v{version}'")

    m = matches[0]
    title = m.group("title").strip()

    next_heading = ENTRY_HEADING.search(releases, m.end())
    body_end = next_heading.start() if next_heading else len(releases)

    body = releases[m.end():body_end].strip("\n")
    body = TRAILING_RULES.sub("", body).strip("\n")
    return title, body


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--changelog", required=True, type=Path)
    ap.add_argument("--version", required=True,
                    help="version string without leading 'v' (e.g. 1.2.0-beta.3)")
    ap.add_argument("--output", required=True, type=Path,
                    help="path to write the assembled release body")
    args = ap.parse_args()

    if not args.changelog.is_file():
        die(f"changelog not found: {args.changelog}")

    text = args.changelog.read_text(encoding="utf-8")

    preamble = extract_between(text, PREAMBLE_START, PREAMBLE_END, "preamble")
    known_issues = extract_between(
        text, KNOWN_ISSUES_START, KNOWN_ISSUES_END, "known issues")
    footer = extract_between(text, FOOTER_START, FOOTER_END, "footer")
    title, body = find_entry(text, args.version)

    assembled = f"{preamble}\n\n{body}\n\n{known_issues}\n\n{footer}\n"
    args.output.write_text(assembled, encoding="utf-8")

    print(f"title: {title}")
    print(f"wrote: {args.output} ({len(assembled)} bytes)")

    gh_output = os.environ.get("GITHUB_OUTPUT")
    if gh_output:
        with open(gh_output, "a", encoding="utf-8") as f:
            f.write(f"release_title={title}\n")


if __name__ == "__main__":
    main()
