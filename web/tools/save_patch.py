#!/usr/bin/env python3
"""Writes the difference between two trees as a -p1 patch (web/build.sh --save-patch).

    save_patch.py <base dir> <changed dir> <out.patch>

The browser build's runtime changes are made in .cache/outputs/web/x86recomp (the copy build.sh
patches) and kept as web/patches/x86recomp-web.patch, against x86recomp-base (the same copy
before it)."""
import difflib
import os
import sys


def files(root):
    out = set()
    for d, dirs, names in os.walk(root):
        dirs[:] = [x for x in dirs if x not in ('.git', 'out')]
        for n in names:
            out.add(os.path.relpath(os.path.join(d, n), root))
    return out


def read(path):
    try:
        with open(path, encoding='utf-8') as f:
            return f.read().splitlines(keepends=True)
    except (FileNotFoundError, UnicodeDecodeError):
        return None


def main():
    base, changed, out = sys.argv[1:4]
    chunks = []
    for rel in sorted(files(base) | files(changed)):
        a, b = read(os.path.join(base, rel)), read(os.path.join(changed, rel))
        if a == b or (a is None and b is None):
            continue
        diff = list(difflib.unified_diff(a or [], b or [],
                                         '/dev/null' if a is None else 'a/' + rel,
                                         '/dev/null' if b is None else 'b/' + rel, n=3))
        for i, line in enumerate(diff):
            if not line.endswith('\n'):
                diff[i] = line + '\n\\ No newline at end of file\n'
        chunks.append(''.join(diff))
    with open(out, 'w', encoding='utf-8') as f:
        f.write(''.join(chunks))
    print(f'{out}: {len(chunks)} files')


if __name__ == '__main__':
    main()
