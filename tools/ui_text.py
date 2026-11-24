#!/usr/bin/env python3
"""The app's words in the game's languages (h4/src/ui_text.h, h4/ui_text/<code>.json): what each
language lacks.

  tools/ui_text.py              each language's missing words (and the ones no code asks for)
  tools/ui_text.py new <code>   h4/ui_text/<code>.json to start a language: every word, English
                                (translate the values; a value left English shows English)

The words are found in the code by the calls that look them up with a literal: T("..."), TN, TF,
TrIn(lang, "..."), tr("...") (Android), Text("...") (the random map), ShowNotice("...") and
ui.T("...") (h4/src/app_ui). Words looked up from tables (the gamepad's
actions, the scaling filters, the random map's templates...) are known from the other languages'
files: a word every file has is not reported as unused.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIR = os.path.join(ROOT, "h4", "ui_text")
SOURCES = [
    ("h4/src", (".cpp", ".mm", ".h")),
    ("platforms/android/app/src/main/java", (".java",)),
]
LIT = r'"(?:[^"\\\n]|\\.)*"'
CALL = re.compile(r'(?:\b(?:T|TN|TF|tr|Text|ShowNotice)\(|\bTrIn\([^,()]*,\s*)((?:' + LIT + r'\s*(?:\+\s*)?)+)')


def unescape(lits):
    out = ""
    for m in re.finditer(LIT, lits):
        s, i = m.group(0)[1:-1], 0
        while i < len(s):
            if s[i] == "\\":
                n = s[i + 1]
                if n == "u":
                    out += chr(int(s[i + 2:i + 6], 16))
                    i += 6
                    continue
                out += {"n": "\n", "t": "\t", '"': '"', "\\": "\\", "'": "'"}.get(n, n)
                i += 2
            else:
                out += s[i]
                i += 1
    return out


def used_words():
    words = {}
    for top, exts in SOURCES:
        for d, _, files in os.walk(os.path.join(ROOT, top)):
            for f in files:
                if not f.endswith(exts):
                    continue
                path = os.path.join(d, f)
                text = open(path, encoding="utf-8").read()
                for m in CALL.finditer(text):
                    w = unescape(m.group(1))
                    if w and not w.isspace():
                        words.setdefault(w, os.path.relpath(path, ROOT) + ":" + str(text.count("\n", 0, m.start()) + 1))
    return words


def languages():
    out = {}
    for f in sorted(os.listdir(DIR)):
        if f.endswith(".json"):
            out[f[:-5]] = json.load(open(os.path.join(DIR, f), encoding="utf-8"))
    return out


def main():
    langs = languages()
    used = used_words()
    known = set(used)
    for l in langs.values():
        known |= set(l["words"])
    if len(sys.argv) == 3 and sys.argv[1] == "new":
        path = os.path.join(DIR, sys.argv[2] + ".json")
        if os.path.exists(path):
            sys.exit(f"{path} is there already")
        words = {w: w.split("|")[0] for w in sorted(known)}
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"name": sys.argv[2], "words": words}, f, ensure_ascii=False, indent=2)
            f.write("\n")
        print(f"{path}: {len(words)} words to translate")
        return
    bad = False
    for code, l in langs.items():
        missing = sorted(known - set(l["words"]))
        print(f"{code} ({l.get('name', '')}): {len(l['words'])} words, {len(missing)} missing")
        for w in missing:
            print(f"  missing: {w!r}" + (f"  ({used[w]})" if w in used else ""))
            bad = True
        for w, t in l["words"].items():
            for k in re.findall(r"\{\d+\}", w):
                if k not in t:
                    print(f"  {w!r}: the translation has no {k}")
                    bad = True
    everywhere = set.intersection(*(set(l["words"]) for l in langs.values())) if langs else set()
    unused = sorted(w for w in known - set(used) if w not in everywhere)
    for w in unused:
        print(f"  in some files, asked for by no code: {w!r}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
