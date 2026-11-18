"""The languages the apps offer: the latest release of the translations repository.

    languages.py <out dir> <res dir>      prints the address of the release list (or nothing)

The translations are a separate repository, ReHoMM4-translations, managed and released on their
own (its tools/release.sh); this repository does not include it. The apps read
<repository>/releases/latest/download/languages.json at every start and take a new release's
packs then: no new app is needed, and nothing changes for players until a release is made. Here,
at build time:

  - the address: H4_TRANSLATIONS_URL (a languages.json anywhere, e.g. a test server), else the
    repository H4_TRANSLATIONS_REPO (owner/name), else evilHazzarD/ReHoMM4-translations:
    https://github.com/evilHazzarD/ReHoMM4-translations/releases/latest/download/languages.json;
  - the list the app starts with (so a first start offline has it): that languages.json as it is
    now, in <out dir>/languages.inc (h4/src/language.cpp); languages.tsv (code, name, restart note:
    the Mac DMG launcher) and
    translations.tsv (code, name, pack url, sha256: the Mac DMG installer downloads the packs).

H4_TRANSLATIONS_DIR=<a checkout of ReHoMM4-translations> (testing translations not released yet):
every language there is built from your game/ (its tools/pack.py --from-repo) and goes in the app
as <res dir>/<code>.h4p.
"""
import glob
import json
import os
import shutil
import subprocess
import sys
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENGLISH_NOTE = 'Restart the game to apply the new language.'
REPO = 'evilHazzarD/ReHoMM4-translations'


def warn(msg):
    print('warning: ' + msg, file=sys.stderr)


def c_str(s):
    return json.dumps(s, ensure_ascii=False)


def release_url():
    if os.environ.get('H4_TRANSLATIONS_URL'):
        return os.environ['H4_TRANSLATIONS_URL']
    repo = os.environ.get('H4_TRANSLATIONS_REPO') or REPO
    return f'https://github.com/{repo}/releases/latest/download/languages.json'


def fetch(url):
    try:
        with urllib.request.urlopen(url, timeout=20) as r:
            j = json.loads(r.read().decode('utf-8'))
        return [l for l in j.get('languages', []) if l.get('code') and l.get('url') and l.get('sha256')]
    except Exception as e:  # noqa: BLE001 (offline, no release yet: the app looks at its start)
        warn(f'no translations release list at {url} ({e}): the app gets it at its first start online')
        return []


def bundled(res, loc):
    """Every language of a ReHoMM4-translations checkout, built from game/ into the app."""
    if not os.path.exists(os.path.join(loc, 'tools', 'lang.py')):
        warn(f'H4_TRANSLATIONS_DIR={loc}: not a ReHoMM4-translations checkout: nothing to bundle')
        return []
    os.environ.setdefault('H4_LOC_WORK', os.path.join(ROOT, '.cache', 'shared', 'loc'))
    sys.path.insert(0, os.path.join(loc, 'tools'))
    import lang as L
    out = []
    for code in L.codes():
        lg = L.Language(code)
        r = subprocess.run([sys.executable, '-I', os.path.join(loc, 'tools', 'pack.py'), code,
                            os.path.join(ROOT, 'game'), '--from-repo'], capture_output=True, text=True)
        if r.returncode:
            sys.stderr.write(r.stderr[-2000:])
            warn(f'{code}: its pack could not be built: left out')
            continue
        pack = r.stdout.strip().splitlines()[-1]
        shutil.copyfile(pack, os.path.join(res, code + '.h4p'))
        out.append({'code': code, 'name': lg.name, 'english': lg.english_name,
                    'note': lg.meta.get('restart_note') or ENGLISH_NOTE, 'url': '', 'sha256': '', 'size': 0})
        print(f'{lg.english_name}: in the app ({os.path.getsize(pack) >> 20} MB, not released)', file=sys.stderr)
    return out


def main(out, res):
    os.makedirs(out, exist_ok=True)
    os.makedirs(res, exist_ok=True)
    for old in glob.glob(os.path.join(res, '*.h4p')) + glob.glob(os.path.join(res, '.uk.stamp')):
        os.remove(old)
    url = release_url()
    loc = os.environ.get('H4_TRANSLATIONS_DIR', '')
    langs = bundled(res, os.path.abspath(loc)) if loc else []
    if url:
        have = {l['code'] for l in langs}
        for l in fetch(url):
            if l['code'] not in have:
                langs.append(l)
                print(f'{l.get("english", l["code"])}: from the release ({l.get("version", "?")})', file=sys.stderr)
    with open(os.path.join(out, 'languages.inc.new'), 'w', encoding='utf-8') as f:
        f.write('// Made by platforms/languages.py: the translations the app starts with.\n')
        for l in langs:
            f.write('{' + ', '.join(c_str(l.get(k, '')) for k in ('code', 'name', 'english', 'note', 'url', 'sha256'))
                    + f', {int(l.get("size", 0))}ULL}},\n')
    inc = os.path.join(out, 'languages.inc')
    new = open(inc + '.new', encoding='utf-8').read()
    if not os.path.exists(inc) or open(inc, encoding='utf-8').read() != new:  # a compile input
        os.replace(inc + '.new', inc)
    else:
        os.remove(inc + '.new')
    with open(os.path.join(out, 'languages.tsv'), 'w', encoding='utf-8') as f:
        f.write(f'en\tEnglish\t{ENGLISH_NOTE}\n')
        for l in langs:
            f.write(f'{l["code"]}\t{l["name"]}\t{l.get("note") or ENGLISH_NOTE}\n')
    with open(os.path.join(out, 'translations.tsv'), 'w', encoding='utf-8') as f:
        for l in langs:
            f.write(f'{l["code"]}\t{l["name"]}\t{l.get("url", "")}\t{l.get("sha256", "")}\n')
    print(url)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
