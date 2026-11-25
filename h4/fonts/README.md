# The app's own fonts

The settings of the Linux, Windows, Switch and browser apps (h4/src/app_ui) write with the game's
own font where the interface's language fits the game's code page, and with these elsewhere
(another script than the game's, and the symbols the game's fonts lack):

- `NotoSans-Regular.ttf`, `NotoSans-Bold.ttf`: Noto Sans (notofonts/latin-greek-cyrillic),
  Latin, Greek and Cyrillic only;
- `NotoSansSymbols-Subset.ttf`: ← ↑ → ↓ ⚙ of Noto Sans Symbols;
- `NotoSansSymbols2-Subset.ttf`: □ △ ▾ ○ ☁ ☐ ☑ ☕ ✕ of Noto Sans Symbols 2.

All under the SIL Open Font License 1.1 (`OFL.txt`). Made from the hinted TTFs of
notofonts.github.io with fontTools' pyftsubset, without hinting and layout tables (the app draws
them with stb_truetype, h4/third_party/stb, which uses neither):

    pyftsubset NotoSans-Regular.ttf --no-hinting --layout-features='' --drop-tables+=GSUB,GPOS,GDEF,DSIG \
      --unicodes=U+0020-007E,U+00A0-024F,U+0300-036F,U+0370-03FF,U+0400-052F,U+1E00-1EFF,U+2000-206F,U+20AC,U+2116,U+2122,U+2212
    (the same for NotoSans-Bold.ttf)
    pyftsubset NotoSansSymbols-Regular.ttf --no-hinting --layout-features='' --text='←↑→↓⚙'
    pyftsubset NotoSansSymbols2-Regular.ttf --no-hinting --layout-features='' --text='□△▾○☁☐☑☕✕'

A new language in another script: its letters added to the first two (their --unicodes).
