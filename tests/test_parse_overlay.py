#!/usr/bin/env python3
"""Smoke-test the r_screenoverlay command parser (mirrors Overlay_ParseScreenOverlay)."""

import re
import sys


def parse_screen_overlay(cmd: str):
    if cmd is None:
        return None
    cmd = cmd.lstrip()
    m = re.match(r"(?i)^r_screenoverlay(?:\s+(.*))?$", cmd)
    if not m:
        return None
    rest = m.group(1) or ""
    rest = rest.strip()
    if rest.startswith('"'):
        rest = rest[1:]
        end = rest.find('"')
        if end >= 0:
            rest = rest[:end]
        return rest
    return rest.rstrip()


def main() -> int:
    cases = [
        ('r_screenoverlay effects/foo', 'effects/foo'),
        ('r_screenoverlay "effects/foo"', 'effects/foo'),
        ('R_ScreenOverlay \"\"', ''),
        ('  r_screenoverlay   "a/b"  ', 'a/b'),
        ('say hello', None),
        ('r_screenoverlay', ''),
        ('r_screenoverlay0', None),
        ('r_screenoverlay off', 'off'),
    ]
    failed = 0
    for cmd, expected in cases:
        got = parse_screen_overlay(cmd)
        ok = got == expected
        print(f"{'OK' if ok else 'FAIL'}: {cmd!r} -> {got!r} (expected {expected!r})")
        if not ok:
            failed += 1
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
