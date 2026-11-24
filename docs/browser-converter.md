# Browser converter

This document specifies the browser converter that will be published through GitHub Pages. It is
separate from the existing `web/` player: the player runs an already lifted build, while the
converter must turn a GOG installer chosen by the player into a ReHoMM4 build without uploading
any game file.

## Security and publishing boundary

- GitHub Pages and GitHub Actions publish only ReHoMM4 source, converter code, and platform shell
  templates. They never contain `heroes4.exe`, generated lifted code, or game data.
- The browser reads the two GOG installer files through the File System Access API / file inputs.
  The files remain in the browser's origin-private storage and may be deleted by the player.
- Generated game builds are downloaded to the player or retained in that browser. They are never
  uploaded as Actions artifacts.
- The supported GOG Complete 3.0 build is 76223 in German, English, French, Italian and Spanish.
  The allowlist in `converter/h4/game.toml` excludes the Polish executable.

## Components

| Component | Responsibility | May contain game code? |
|---|---|---|
| `converter-web` | File picker, progress UI, download/play controls | No |
| `converter-core` | Parse PE, validate SHA-256, apply the ReHoMM4 manifest and emit a build plan | No |
| `converter-runtime` | Browser-compatible dynamic-lift runtime adapter | No |
| platform template | ReHoMM4 runtime, UI/assets and per-platform packaging metadata | No |
| output | Player-owned build containing their own game data/code | Yes, local only |

`third_party/game-recomp` is a pinned MIT-licensed submodule used only for its dynamic conversion
crates (`rc-convert`, `rc-emit-wasm`, PE parsing and browser worker plumbing). ReHoMM4-specific
entry points, replacements and compatibility rules come from `h4/lifter.manifest`; no GameRecomp
game runtime, templates, UI, game data or Polish target is linked into ReHoMM4.

The converter build stages `game.toml` with discovery seeds from the manifest's `entry`,
`probe` and `replace` directives. The preparation worker also returns the complete rules in
its lift plan, including `resume` addresses and replacement names.

## Delivery order

1. Implement `converter-core` validation and its deterministic build-plan format.
2. Make the browser worker unpack and validate the supported installers locally.
3. Add a WebAssembly dynamic-lift adapter and a web template, so **Play in browser** works.
4. Add Windows, then Android and Linux templates.
5. Add the GitHub Actions workflow: build converter/templates, upload non-game artifacts, and
   deploy the site with `actions/deploy-pages`.

## Acceptance checks

- A clean checkout passes the normal repository publication check without a game copy.
- The Pages artifact has no `heroes4.exe`, `.h4r`, generated lift output, or game archive.
- A supported installer produces the same manifest identity and ReHoMM4 replacement list on
  every supported browser.
- A Polish installer is rejected before conversion with an explicit unsupported-language message.

## ReHoMM4 web ABI

Lifted PE32 functions receive an i32 pointer to the active x86recomp thread's context. In ABI
version 1, EAX through EDI are 32-bit values at offsets 0, 8, 16, 24, 32, 40, 48 and 56 in x86
encoding order. EFLAGS is at offset 128 and the guest FS base at 132. The shell checks these
offsets at compile time. The lifted module imports the shell's shared memory as `h4.memory`.
It adds the host pointer returned by `h4.guest_base()` to guest addresses for memory accesses.

`h4.dispatch(ctx, target, from)` enters the shell's existing dispatcher.
`h4.resume(target, ebp, from)` throws the runtime's `GuestResume` at the staged manifest's resume
sites. Dispatch from a resumable frame checks the caller's EBP and propagates exceptions that belong
to another activation. The emitter still needs player integration for entry registration,
continuation dispatch and the replacement hooks.

`converter/emitter` lowers a subset of GameRecomp IR to this ABI. It currently supports
32-bit and partial register operations, 64-bit integer intermediates, guest memory access,
branches in both directions, jump tables, direct and indirect calls, returns, and x87 memory
operations for binary32, binary64 and signed integer operands. Integer division reports arithmetic faults through
the existing runtime. The emitter reports the guest address of unsupported instructions. The
`check_install` example accepts `--coverage` to group the first failure per function
on a selected local executable. Its `--output PATH` mode writes a local diagnostic WASM module
for validation.

`web/src/dynamic_loader.mjs` checks the shell ABI version and links a lifted module to each
worker's shared memory and exports. The dynamic shell registers the C++ replacement functions
generated from the manifest. Their `_lifted` counterparts call the original bodies in the
worker's WASM registry.

## Local web package and CI

`bash web/build.sh --dynamic` builds the shell and browser converter into
`.cache/outputs/web/dist-dynamic`. The shell starts from the installed EXE's SHA-256 and loads
the corresponding code from OPFS. When there is no cached code for that EXE, the page runs a
converter worker and restarts after it has written the file.

The **ReHoMM4 web** workflow builds the same directory on GitHub and uploads `rehomm4-web`.
Its manual `publish` input enables the Pages deployment job when the repository uses GitHub
Actions as the Pages source.
