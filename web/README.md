# Heroes IV in the browser (web)

The same static recompilation of `heroes4.exe` as the apps (`platforms/`), built with
[Emscripten](https://emscripten.org) into one WebAssembly module and a page that runs it. No game
files are in the build or on the server: the page installs them from the player's own GOG copy
into the browser's storage, as the apps' in-app installer does.

Status: builds; see *Known issues*.

## Build

```sh
tools/build.sh          # once, as for the apps: game/, x86recomp + patches/, h4/generated
web/deps.sh          # the Emscripten SDK (.cache/shared/emsdk), SDL3, ffmpeg, innoextract (~10 min)
web/build.sh         # -> .cache/outputs/web/dist (the site)
python3 web/serve.py # http://localhost:8080
```

`build.sh` runs `deps.sh` itself when the libraries are missing. Run the scripts with bash (they
are not meant to be sourced from zsh).

## Play

Open the page, press **Start**. The first time the game's installer screen opens (the apps' own,
drawn in the page):

- **Install from GOG installer files**: pick both files of GOG's offline installer,
  `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(76223).exe` and its `-1.bin`
  (GOG.com > your library > the game > Downloads). They are copied into the browser's storage,
  unpacked by innoextract inside the page, checked against `h4/src/install/manifest_en.inc`, and
  deleted again.
- **Use an installed game (its folder)**: pick the folder of an installed copy (the one with
  `heroes4.exe`). The browser asks once whether the page may read the folder's files.

"Sign in to GOG.com and download" is not offered: GOG's servers do not answer pages of other sites
(no CORS), so a page cannot download from them.

The game files (about 1 GB), saves and settings stay in the browser's *origin private file
system*, for this site only; the page asks the browser to keep them (`navigator.storage.persist`).
"Delete the game files kept in this browser" on the start screen removes them all.

Testing, or a server of your own: `python3 web/serve.py --game game` serves an installed game
folder at `/game-files/`, and `http://localhost:8080/?game=game-files` copies it in once instead of
the installer. Never put the game where others can download it.

### Hosting

Any static server works, over HTTPS (or `localhost`): shared memory, which the game's threads need,
is only given to *cross-origin isolated* pages. Send

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

with every file (`serve.py` does). Where the server cannot (GitHub Pages), the page registers
`coi-sw.js`, a service worker that adds them, and reloads once.

Browsers: current desktop Chrome, Edge, Firefox and Safari, 64-bit (the page needs 2.5-3 GB of
memory of the 4 GB a tab may have). WebAssembly tail calls are required (Chrome 112, Firefox 121,
Safari 18.2).

## How it works

| | the apps | the browser |
|---|---|---|
| CPU | the lifted code, native | the same C++ compiled to wasm32 (`-mtail-call` for the lifted tail calls) |
| guest memory | a 4 GiB reservation, pages committed by mprotect | 2 GiB of the module's linear memory (wasm32: 4 GiB at most), from the heap's break, as on the Switch |
| SEH from access violations | SIGSEGV handler | none: wasm has no faults inside its memory (as on the Switch) |
| locked GPU surfaces (lazy guest memory) | page protection | filled at Lock (the Switch's path) |
| threads | pthreads | pthreads on Web Workers (shared memory) |
| the game's main loop | the process's main thread | a worker (`PROXY_TO_PTHREAD`): it may block like on Windows; SDL proxies the page work (canvas, events, audio) to the page's thread |
| graphics | SDL_GPU (Vulkan/Metal) | the soft device (`gfx/soft_device.cpp`, as on the Switch) on SDL's software renderer, drawn into the canvas |
| files | the app's folder | WasmFS: OPFS at `/persist`; `ui/`, `wings/` preloaded at `/` |
| SEH resumes, C++ exceptions | native unwinding | wasm exception handling (`-fwasm-exceptions`) |

The changes to the runtime are `patches/x86recomp-web.patch`, applied by `build.sh` to a copy of
`third_party/x86recomp` in `.cache/outputs/web/x86recomp` (the original is not touched): mostly the
Switch's "no signals, no mmap" paths taken for `__EMSCRIPTEN__` too, the arena from the heap, no
stack switch, smaller host stacks (16 MiB: in wasm only the shadow stack is in linear memory). To
change it, edit that copy, then `web/build.sh --save-patch`; `build.sh` refuses to overwrite
changes there that are not in the patch.

The game's own sources are `h4/src` as they are, as the Linux app builds them (`H4_DESKTOP_APP`,
`H4_LINUX_APP`; the settings and language buttons drawn into the frame), with `src/`:

- `web_main.cpp`: `main()`: mounts OPFS, sets `XDG_DATA_HOME` to it (where the Linux app keeps
  `ReHoMM4/game` and `ReHoMM4/player`), SDL's hints for the page, then the game's `main()`
  (`h4/src/main.cpp`, compiled as `h4_game_main`); restarting (after a language is picked) reloads
  the page. The log stays the browser's console (F12).
- `web_platform.cpp`: the installer's platform layer (`install/platform.h`): HTTPS by the Fetch API,
  files picked with the page's file input (`web_pre.js`) and copied into OPFS, no GOG sign-in.
- `web_pre.js`: the page side of picking files.
- `shell/`: the page (`index.html`) and `coi-sw.js`.

## Known issues

- Not yet played through in a browser. First things to look at:
  - the browser's own stack: guest calls nest as host calls, and lifted frames are large; a deep
    guest call chain may hit the engine's stack limit ("Maximum call stack size exceeded");
  - SDL's software renderer scales each frame on the CPU and copies it to the canvas;
    with the canvas transferred to the game's worker (OffscreenCanvas) the WebGL renderer could do it;
  - audio: SDL's Web Audio output runs on the page's thread.
- No GOG.com download (CORS, above).
- Guest exceptions from access violations (a game's `__try` around a bad pointer) never happen:
  WebAssembly memory has no unmapped pages inside it. The Switch build has the same limit.
- Save sync (Syncthing/rclone folders, iCloud) is not there: the saves are in the browser only.
