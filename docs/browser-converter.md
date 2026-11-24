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
- The first supported executable is GOG Complete 3.0 English, SHA-256
  `b7c2f6caeeba640b1c5f1d812b172938bb4dd4ec62e34ec93ad827bd1ffaec87`.
  Polish is deliberately not an input or output language.

## Components

| Component | Responsibility | May contain game code? |
|---|---|---|
| `converter-web` | File picker, progress UI, download/play controls | No |
| `converter-core` | Parse PE, validate SHA-256, apply the ReHoMM4 manifest and emit a build plan | No |
| `converter-runtime` | Browser-compatible dynamic-lift runtime adapter | No |
| platform template | ReHoMM4 runtime, UI/assets and per-platform packaging metadata | No |
| output | Player-owned build containing their own game data/code | Yes, local only |

`converter-core` and the web worker will be adapted from the MIT converter architecture used by
GameRecomp. ReHoMM4-specific entry points, replacements and compatibility rules come from
`h4/lifter.manifest`; no GameRecomp game runtime is linked into ReHoMM4.

## Delivery order

1. Implement `converter-core` validation and its deterministic build-plan format.
2. Make the browser worker unpack and validate the English installer locally.
3. Add a WebAssembly dynamic-lift adapter and a web template, so **Play in browser** works.
4. Add Windows, then Android and Linux templates.
5. Add the GitHub Actions workflow: build converter/templates, upload non-game artifacts, and
   deploy the site with `actions/deploy-pages`.

## Acceptance checks

- A clean checkout passes the normal repository publication check without a game copy.
- The Pages artifact has no `heroes4.exe`, `.h4r`, generated lift output, or game archive.
- A valid English installer produces the same manifest identity and ReHoMM4 replacement list on
  every supported browser.
- A Polish installer is rejected before conversion with an explicit unsupported-language message.
