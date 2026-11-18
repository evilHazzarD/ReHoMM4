"""A Linux x86-64 sysroot from Debian 12 (bookworm) packages, for cross-building the Linux (Steam
Deck) version on a Mac (platforms/env.sh linux). glibc 2.36: the binary runs on SteamOS 3 and on
distributions from 2022 on. Headers and libraries only: SDL3 loads X11, PulseAudio, PipeWire,
ALSA and udev at run time from the system, so only their headers matter here.

    python3 -I platforms/linux/sysroot.py <out dir>
"""
import io
import lzma
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

MIRROR = 'https://deb.debian.org/debian'
DIST = 'bookworm'
PACKAGES = [
    # C/C++ toolchain parts clang links against
    'libc6', 'libc6-dev', 'linux-libc-dev', 'libgcc-12-dev', 'libgcc-s1', 'libstdc++-12-dev',
    'libstdc++6', 'zlib1g', 'zlib1g-dev', 'libasan8', 'libitm1', 'libquadmath0', 'libtsan2',
    'libubsan1', 'liblsan0', 'libatomic1', 'libgomp1',
    # SDL3's video (X11: Steam Deck's Game Mode runs games on gamescope's Xwayland)
    'libx11-dev', 'libx11-6', 'x11proto-dev', 'libxext-dev', 'libxext6', 'libxrandr-dev',
    'libxrender-dev', 'libxi-dev', 'libxfixes-dev', 'libxcursor-dev', 'libxss-dev', 'libxcb1-dev',
    'libxau-dev', 'libxdmcp-dev', 'libxtst-dev', 'libxkbcommon-dev',
    # audio and input
    'libpulse-dev', 'libasound2-dev', 'libpipewire-0.3-dev', 'libspa-0.2-dev', 'libudev-dev',
    'libdbus-1-dev',
    # their run-time libraries: SDL finds their names (sonames) to load them, not link them
    'libpulse0', 'libpipewire-0.3-0', 'libasound2', 'libudev1', 'libdbus-1-3', 'libxcursor1',
    'libxi6', 'libxrandr2', 'libxfixes3', 'libxss1', 'libxrender1', 'libxcb1', 'libxau6',
    'libxdmcp6', 'libxkbcommon0', 'libxtst6',
]


def packages_index():
    url = f'{MIRROR}/dists/{DIST}/main/binary-amd64/Packages.xz'
    data = lzma.decompress(urllib.request.urlopen(url, timeout=120).read()).decode('utf-8', 'replace')
    index = {}
    for block in data.split('\n\n'):
        fields = {}
        for line in block.splitlines():
            if line and not line[0].isspace() and ':' in line:
                k, v = line.split(':', 1)
                fields[k] = v.strip()
        if 'Package' in fields and 'Filename' in fields:
            index[fields['Package']] = fields['Filename']
    return index


def extract_deb(deb, out):
    with tempfile.TemporaryDirectory() as t:
        subprocess.run(['ar', 'x', deb], cwd=t, check=True)
        data = [f for f in os.listdir(t) if f.startswith('data.tar')][0]
        mode = 'r:xz' if data.endswith('.xz') else 'r:gz' if data.endswith('.gz') else None
        if mode is None:  # .zst: through the zstd tool
            raw = subprocess.run(['zstd', '-dc', os.path.join(t, data)], check=True, capture_output=True).stdout
            tf = tarfile.open(fileobj=io.BytesIO(raw))
        else:
            tf = tarfile.open(os.path.join(t, data), mode)
        with tf:
            for m in tf.getmembers():
                if m.name.startswith(('./usr/share/doc', './usr/share/man', './usr/share/locale')):
                    continue
                if m.isdev():
                    continue
                tf.extract(m, out, filter='tar')


def fix_links(out):
    # Absolute symbolic links (libfoo.so -> /lib/x86_64-linux-gnu/libfoo.so.1) made relative to
    # the sysroot; /lib is /usr/lib (merged /usr).
    for d, dirs, files in os.walk(out):
        for n in dirs + files:
            p = os.path.join(d, n)
            if os.path.islink(p):
                t = os.readlink(p)
                if t.startswith('/'):
                    target = os.path.join(out, t.lstrip('/'))
                    os.remove(p)
                    os.symlink(os.path.relpath(target, d), p)
    # Linker scripts naming absolute paths (libc.so: /lib/x86_64-linux-gnu/libc.so.6): lld finds
    # them in the sysroot when it is given (--sysroot), as clang does.


def main(out):
    os.makedirs(out, exist_ok=True)
    for d in ('lib', 'lib64', 'bin', 'sbin'):  # merged /usr
        p = os.path.join(out, d)
        if not os.path.lexists(p):
            os.makedirs(os.path.join(out, 'usr', d), exist_ok=True)
            os.symlink(os.path.join('usr', d), p)
    index = packages_index()
    with tempfile.TemporaryDirectory() as t:
        for name in PACKAGES:
            if name not in index:
                sys.exit(f'{name}: not in {DIST}')
            deb = os.path.join(t, os.path.basename(index[name]))
            with urllib.request.urlopen(f'{MIRROR}/{index[name]}', timeout=300) as r, open(deb, 'wb') as f:
                shutil.copyfileobj(r, f)
            extract_deb(deb, out)
            os.remove(deb)
            print(name)
    fix_links(out)
    open(os.path.join(out, '.complete'), 'w').write(DIST + '\n')


if __name__ == '__main__':
    main(sys.argv[1])
