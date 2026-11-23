#!/usr/bin/env python3
# Writes h4/src/random_map/templates_shipped.cpp from h4/src/random_map/templates/*.txt: our
# template files, which the generator writes into the player's Templates folder. A raw string a
# template (MSVC: 16 KB at most a literal). Run it after changing a file there.
import glob, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', 'h4', 'src', 'random_map')
files = sorted(glob.glob(os.path.join(SRC, 'templates', '*.txt')))
out = ['// Made by tools/random_map/embed_templates.py from templates/*.txt: do not edit.',
       '#include "template.h"', '', 'namespace h4::random_map {', '', 'const ShippedTemplate kShippedTemplates[] = {']
for f in files:
    text = open(f).read()
    assert ')tpl"' not in text
    pieces = re.split(r'(?=\nname )', text)
    out.append(f'    {{"{os.path.basename(f)}",')
    out += [f'     R"tpl({p})tpl"' for p in pieces if p]
    out[-1] += '},'
out += ['};', f'const int kShippedTemplateCount = {len(files)};', '', '}  // namespace h4::random_map', '']
open(os.path.join(SRC, 'templates_shipped.cpp'), 'w').write('\n'.join(out))
print(len(files), 'templates')
