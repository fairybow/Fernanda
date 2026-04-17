#!/usr/bin/env python3
"""
Assemble a GitHub Release body and title from CHANGELOG.md.

The CHANGELOG is expected to contain two marker-delimited blocks for the
reusable preamble and footer, followed by per-version entries whose
headings end in " - tag v<VERSION>".

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


PREAMBLE_START = "<!-- release-preamble-start -->"
PREAMBLE_END = "<!-- release-preamble-end -->"
FOOTER_START = "<!-- release-footer-start -->"
FOOTER_END = "<!-- release-footer-end -->"


def die(msg: str) -> None:
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
    Locate the release entry for `version` and return (title, body).
    """
    pattern = re.compile(
        rf"^# (?P<title>.+?) - tag v{re.escape(version)}\s*$",
        re.MULTILINE,
    )
    m = pattern.search(text)
    if not m:
        die(f"no entry found whose heading ends with ' - tag v{version}'")

    title = m.group("title").strip()
    body_start = m.end()

    tail = text[body_start:]
    boundaries = []
    for b_pat in (r"^---\s*$", r"^# "):
        bm = re.search(b_pat, tail, re.MULTILINE)
        if bm:
            boundaries.append(bm.start())
    body_end = min(boundaries) if boundaries else len(tail)

    body = tail[:body_end].strip("\n")
    return title, body


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--changelog", required=True, type=Path)
    ap.add_argument("--version", required=True,
                    help="version string without leading 'v' (e.g. 0.99.0-beta.22)")
    ap.add_argument("--output", required=True, type=Path,
                    help="path to write the assembled release body")
    args = ap.parse_args()

    if not args.changelog.is_file():
        die(f"changelog not found: {args.changelog}")

    text = args.changelog.read_text(encoding="utf-8")

    preamble = extract_between(text, PREAMBLE_START, PREAMBLE_END, "preamble")
    footer = extract_between(text, FOOTER_START, FOOTER_END, "footer")
    title, body = find_entry(text, args.version)

    assembled = f"{preamble}\n\n{body}\n\n{footer}\n"
    args.output.write_text(assembled, encoding="utf-8")

    print(f"title: {title}")
    print(f"wrote: {args.output} ({len(assembled)} bytes)")

    gh_output = os.environ.get("GITHUB_OUTPUT")
    if gh_output:
        with open(gh_output, "a", encoding="utf-8") as f:
            f.write(f"release_title={title}\n")


if __name__ == "__main__":
    main()