#!/usr/bin/env python3
# Contains code from HoMM4Deadware (https://github.com/LeoDeadware/HoMM4Deadware): Copyright (c) 2026 LeoDeadware, MIT License.
"""THIRD_PARTY_NOTICES.txt for an app made by platforms/build.sh: the project's license and every
third-party part linked into it, with the license each is used under.

    python3 -I platforms/notices.py <target> <build dir> <out.txt>     (target: ios android macos linux windows switch)

The parts come from the build's own link line (the Ninja build's h4 command; the Xcode project for
iOS), so a linked library always has its notice and nothing unlinked is listed (as platforms/macos-dmg/notices.py
for the DMG). Exit 1 on a linked library this table does not know: an app with an unexplained
library is not released.
"""
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
SRC = os.path.join(ROOT, '.cache', 'shared', 'src')
HOST_DEPS = os.path.join(ROOT, 'third_party', 'x86recomp', 'out', 'build', 'release', '_deps')
LIC = os.path.join(ROOT, 'licenses')
DKP = os.environ.get('DEVKITPRO', '/opt/devkitpro')
REPO = 'https://github.com/evilHazzarD/ReHoMM4'


def ffmpeg_offer(shared):
    how = ('dynamically linked' if shared else
           'statically linked into the app. To relink the app with a modified ffmpeg, build it from its '
           'complete source (our code is MIT, in the repository below) with platforms/build.sh against your '
           'own ffmpeg build in platforms/deps.sh')
    return (f'ffmpeg 9.0.2 (libavcodec, libavformat, libavutil, libswscale) is used under the GNU Lesser '
            f'General Public License version 2.1 or later (full text below). It is {how}. Built WITHOUT '
            f'--enable-gpl and --enable-nonfree; only the MP3 and Bink decoders (configure line: '
            f'platforms/deps.sh).\n\nSource code: the unmodified release ffmpeg-9.0.2.tar.xz from '
            f'https://ffmpeg.org/releases/ ; the build scripts are in {REPO} . On request (an issue in that '
            f'repository) we send a copy of the corresponding source code for at least three years after '
            f'the last release of this app.')


def sdl_license(target):
    if target in ('switch', 'switch2'):
        return glob.glob(os.path.join(SRC, 'SDL-*', 'LICENSE.txt'))[:1]
    return sorted(glob.glob(os.path.join(SRC, 'SDL3-*', 'LICENSE.txt')))[-1:]


def table(target):
    """(name, matches(lib), [license files], extra text)"""
    shared_ff = False
    t = [
        ('x86recomp (the runtime: librecomp_*) - MIT', lambda l: l.startswith('librecomp_'),
         [os.path.join(ROOT, 'third_party', 'x86recomp', 'LICENSE')], ''),
        ('Berkeley SoftFloat 3e (x87 arithmetic) - BSD-3-Clause', lambda l: l == 'libsoftfloat.a',
         [os.path.join(HOST_DEPS, 'softfloat-src', 'COPYING.txt')], ''),
        ('CORE-MATH (x87 transcendental functions) - MIT', lambda l: l in ('libcoremath.a', 'librecomp_x87.a'),
         [os.path.join(HOST_DEPS, 'coremath-src', 'LICENSE')], ''),
        ('SDL3 (with its stb_image) - zlib', lambda l: l.startswith('libSDL3'), sdl_license(target), ''),
        ('glslang (shader compiler) - BSD-3-Clause and others', lambda l: l.startswith(('libglslang', 'libSPIRV.')),
         [os.path.join(SRC, 'glslang', 'LICENSE.txt')], ''),
        ('SPIRV-Cross (Metal shaders) - Apache-2.0', lambda l: l.startswith('libspirv-cross'),
         [os.path.join(SRC, 'SPIRV-Cross', 'LICENSE')], ''),
        ('innoextract (unpacks your GOG installer; in librecomp_innoextract) - zlib',
         lambda l: l == 'librecomp_innoextract.a', glob.glob(os.path.join(SRC, 'innoextract-*', 'LICENSE'))[:1], ''),
        ('Boost (Filesystem, Iostreams; used by innoextract) - BSL-1.0', lambda l: l == 'librecomp_innoextract.a',
         glob.glob(os.path.join(SRC, 'boost_*', 'LICENSE_1_0.txt'))[:1], ''),
        ('liblzma from XZ Utils (used by innoextract) - 0BSD', lambda l: l == 'liblzma.a',
         glob.glob(os.path.join(SRC, 'xz-*', 'COPYING.0BSD'))[:1], ''),
        ('zlib - zlib license', lambda l: l == 'libzlibstatic.a',
         glob.glob(os.path.join(SRC, 'zlib-*', 'LICENSE'))[:1], ''),
        ('ffmpeg (libav*, libsw*) - LGPL-2.1-or-later', lambda l: l.startswith(('libav', 'libsw')),
         [os.path.join(SRC, 'ffmpeg-9.0.2', 'COPYING.LGPLv2.1')], ffmpeg_offer(shared_ff)),
    ]
    if target in ('android', 'linux', 'windows'):  # built into librecomp_innoextract (platforms/deps.sh)
        t.append(('bzip2 (used by innoextract) - bzip2 license', lambda l: l == 'librecomp_innoextract.a',
                  glob.glob(os.path.join(SRC, 'bzip2-*', 'LICENSE'))[:1], ''))
    if target == 'android':
        t.append(('LLVM libc++ (libc++_shared.so) - Apache-2.0 WITH LLVM-exception', lambda l: l == 'libc++_shared.so',
                  [os.path.join(LIC, 'LLVM-Apache-2.0-with-LLVM-exception.txt')], ''))
    if target in ('switch', 'switch2'):
        pl = os.path.join(DKP, 'portlibs', 'switch', 'licenses')
        t += [
            ('zlib - zlib', lambda l: l == '-lz', [os.path.join(pl, 'switch-zlib', 'LICENSE')], ''),
            ('bzip2 - bzip2 license', lambda l: l == '-lbz2', [os.path.join(pl, 'switch-bzip2', 'LICENSE')], ''),
            ('curl 7.69 (the GOG download) - curl license', lambda l: l == '-lcurl', [os.path.join(LIC, 'curl.txt')], ''),
            ('Mbed TLS - Apache-2.0', lambda l: l.startswith('-lmbed'), [os.path.join(pl, 'switch-mbedtls', 'LICENSE')], ''),
            ('libnx - ISC', lambda l: l == '-lnx', [os.path.join(LIC, 'libnx-ISC.txt')], ''),
            ('Mesa (libEGL, libglapi) - MIT', lambda l: l in ('-lEGL', '-lglapi'), [os.path.join(LIC, 'mesa-MIT.txt')], ''),
            ('libdrm_nouveau - MIT', lambda l: l == '-ldrm_nouveau', [os.path.join(LIC, 'libdrm-MIT.txt')], ''),
            ('newlib and the GCC runtime (devkitA64)', lambda l: l in ('-lstdc++', 'libm.a'),
             [os.path.join(LIC, 'newlib.txt')], ''),
        ]
    if target == 'switch2':
        sdk = os.path.join(DKP, 'libounce', 'licenses')
        vk = os.path.join(DKP, 'portlibs', 'switch2', 'share', 'licenses', 'ouncevk')
        t += [
            ('libounce and bundled runtime', lambda l: l in ('-lnx', '-lounce'),
             [os.path.join(sdk, f) for f in ('libounce-ISC.txt', 'libnx-ISC.txt', 'mimalloc-MIT.txt')], ''),
            ('OunceVK', lambda l: l in ('-lEGL', '-lglapi', '-ldrm_nouveau', '-lGLESv2',
                                        '-lvulkan', '-louncevk', '-louncevk-gl'),
             [os.path.join(vk, f) for f in ('README.md', 'mesa-license.rst', 'zlib-license.txt')], ''),
        ]
    return t


# The system's own libraries (not shipped with the app) and the app's own parts.
SYSTEM = {'-lm', '-lpthread', '-ldl', '-latomic', '-l:libatomic.a', '-liconv', '-lobjc', '-leaf', '-llog',
          '-landroid', 'libandroid.so', 'libmediandk.so', 'libm.so', 'libmain.so', 'libh4_lifted.a',
          '-lkernel32', '-luser32', '-lgdi32', '-lwinmm', '-limm32', '-lole32', '-loleaut32',
          '-lshell32', '-lversion', '-lsetupapi', '-lcfgmgr32', '-lwinspool', '-luuid', '-lws2_32',
          '-ladvapi32', '-lshcore', '-lbcrypt', '-ldinput8', '-ldxguid', '-lwinhttp', '-lcomdlg32'}


def linked(target, build):
    if target == 'ios':
        text = open(os.path.join(build, 'h4_recomp.xcodeproj', 'project.pbxproj'), encoding='utf-8').read()
        libs = set(re.findall(r'[A-Za-z0-9_+.-]+\.a\b', text)) | set(re.findall(r'(?<![\w-])-l[A-Za-z0-9_]+', text))
        libs.discard('.a')
        return libs - {'-lz', '-lbz2'}  # Apple's own zlib and bzip2
    cmd = subprocess.run(['ninja', '-C', build, '-t', 'commands', 'h4'], capture_output=True, text=True,
                         check=True).stdout.strip().splitlines()[-1]
    libs = set()
    for w in cmd.split():
        if w.startswith('-Wl,'):
            continue
        b = os.path.basename(w)
        if re.match(r'^-l[:A-Za-z0-9_+.]+$', w) or re.search(r'\.(a|so)(\.[0-9.]+)?$', b):
            libs.add(w if w.startswith('-l') else b)
    if target in ('macos', 'linux', 'android', 'windows'):
        libs -= {'-lz', '-lbz2'}  # the system's
    if target == 'android':
        libs.add('libc++_shared.so')  # shipped beside libmain.so (platforms/android/package.sh)
    return libs


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ('ios', 'android', 'macos', 'linux', 'windows', 'switch', 'switch2'):
        print(__doc__)
        return 2
    target, build, out = sys.argv[1:]
    libs = sorted(l for l in linked(target, build) if l not in SYSTEM)
    parts, covered = [], set()
    for name, hit, files, extra in table(target):
        used = [l for l in libs if hit(l)]
        if not used:
            continue
        missing = [f for f in files if not os.path.isfile(f)]
        if missing or not files:
            print(f'notices: {name}: license file missing {missing or files}', file=sys.stderr)
            return 1
        covered.update(used)
        body = '\n\n'.join(([extra] if extra else []) +
                           [open(f, encoding='utf-8', errors='replace').read().strip() for f in files])
        parts.append(f'== {name}\n\n{body}')
    # Inline notices of our patches (AMD FidelityFX FSR1 in the x86recomp patch -> librecomp_gfx).
    for p in sorted(glob.glob(os.path.join(ROOT, 'patches', '*.patch'))):
        cur = []
        for line in open(p, encoding='utf-8', errors='replace').read().splitlines() + ['']:
            m = re.match(r'^\+\s*(//|#|\*|/\*)\s?(.*)$', line)
            if m:
                cur.append(m.group(2))
                continue
            if any('Copyright' in c for c in cur):
                parts.append(f'== {os.path.basename(p)} (inline notice, in librecomp_gfx)\n\n' + '\n'.join(cur).strip())
            cur = []
    unknown = [l for l in libs if l not in covered]
    if unknown:
        print(f'notices: no notice for linked {unknown} (platforms/notices.py)', file=sys.stderr)
        return 1
    own = open(os.path.join(ROOT, 'LICENSE'), encoding='utf-8').read().strip()
    head = (f'Heroes of Might and Magic IV (unofficial port): the licenses of the software in this app.\n\n'
            f'== This project (ReHoMM4, {REPO}) - MIT\n\n{own}\n\n'
            f'The app icon is the project\'s own art (assets/icon), under the same license.\n'
            f'Heroes of Might and Magic IV and its data belong to their owners; the app installs them from '
            f'your own GOG copy.\n\n')
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, 'w', encoding='utf-8') as fh:
        fh.write(head + '\n\n'.join(parts) + '\n')
    print(f'notices: {len(parts)} notices -> {out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
