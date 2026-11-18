"""Applies a translation pack (<code>.h4p, made by the ReHoMM4-translations release) to a game's
Data/ archives, the way the apps do (h4/src/language.cpp):

    apply_pack.py <pack> <game dir>

Pack: "H4P1", u32 archive count, then per archive: u16 name length, name (in Data/), u64 original
size, u64 appended size, u32 entry count, per entry u32 file position + 12 bytes English + 12 bytes
translated header, then the appended bytes. Each archive must be the original (its size and the
English headers are checked); the data is appended and the translated headers written. Runs on
the player's Mac with /usr/bin/python3 (the DMG installer, install_game.sh step 6).
"""
import os
import struct
import sys


def read_pack(path):
    with open(path, 'rb') as f:
        d = f.read()
    if d[:4] != b'H4P1':
        raise ValueError(f'{path}: not a language pack')
    n, = struct.unpack_from('<I', d, 4)
    p, out = 8, []
    for _ in range(n):
        ln, = struct.unpack_from('<H', d, p); p += 2
        name = d[p:p + ln].decode(); p += ln
        size, appended, count = struct.unpack_from('<QQI', d, p); p += 20
        entries = []
        for _ in range(count):
            pos, = struct.unpack_from('<I', d, p)
            entries.append((pos, d[p + 4:p + 16], d[p + 16:p + 28])); p += 28
        out.append((name, size, entries, d[p:p + appended])); p += appended
    if p != len(d):
        raise ValueError(f'{path}: {len(d) - p} bytes left over')
    return out


def main(pack, game):
    archives = read_pack(pack)
    for name, size, entries, data in archives:  # all checked before anything is written
        path = os.path.join(game, 'Data', name)
        if os.path.getsize(path) != size:
            raise ValueError(f'{name}: not the original archive ({os.path.getsize(path)} bytes, expected {size})')
        with open(path, 'rb') as f:
            for pos, en, _ in entries:
                f.seek(pos)
                if f.read(12) != en:
                    raise ValueError(f'{name}: not the original archive (entry at {pos})')
    for name, size, entries, data in archives:
        with open(os.path.join(game, 'Data', name), 'r+b') as f:
            f.seek(size)
            f.write(data)
            for pos, _, tr in entries:
                f.seek(pos)
                f.write(tr)
        print(f'{name}: {len(entries)} entries, +{len(data)} bytes')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
