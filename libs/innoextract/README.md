# innoextract as a library (in-app installer)

innoextract 1.9 built into the iOS and Android apps, so they can install Heroes IV from the
player's GOG offline installer (`setup_*.exe` + `*-1.bin`) without a helper program.
`platforms/deps.sh` fetches innoextract, Boost 1.89 (Filesystem and Iostreams sources),
XZ Utils 5.8.1 (liblzma) and, for Android, bzip2 1.0.8, each pinned by SHA-256, applies
`patches/` and `lib-patches/`, and builds `lib/` into the prefix.

`lib/`, `patches/` and `lib-patches/` come from HommaRe
(github.com/livmaster-dev/HommaRe-public, `kit/third_party/innoextract`), MIT,
see `LICENSE-HommaRe.txt`; `NOTICE.md` lists the licenses of the code compiled in.
Changes here: Android (configure.hpp, bundled bzip2), install rules.
