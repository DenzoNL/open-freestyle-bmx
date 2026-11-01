#!/usr/bin/env python3
"""Cross-build check (docs/PORTING.md X0): replay a BMX_RECORD recording on two or more bmxnative builds and
compare their per-tick BMX_SIM_HASH files.

    py -I tools/xcheck.py --game-dir <game folder> --recording <file.bmxr> <exe> [<exe> ...] [--ini <ini>]

Each run gets the recording's starting SAVE.DAT in <game folder>/save (the folder's own file is put back
afterwards), BMX_REPLAY, BMX_SIM_HASH, BMX_MUTE and BMX_NO_MOVIES; runs are sequential, in the background (the
window never takes the focus). The first exe is the reference; the report names the first tick where another
differs. Exit status 0 when all hash files are identical.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HEADER = struct.Struct('<4sIBBHH128sI')


def recording_save(path):
    with open(path, 'rb') as f:
        head = f.read(HEADER.size)
        magic, version, present, _family, vendor, product, name, size = HEADER.unpack(head)
        if magic != b'BMXR' or version != 2:
            sys.exit(f'{path}: not a version 2 BMX recording')
        pad = name.split(b'\0')[0].decode('utf-8', 'replace') if present else 'none'
        return f.read(size), pad, (vendor, product)


def run(exe, game_dir, recording, save, ini, out_dir, label, timeout):
    hash_path = os.path.join(out_dir, f'{label}.hash')
    save_path = os.path.join(game_dir, 'save', 'SAVE.DAT')
    env = dict(os.environ)
    env.update(BMX_REPLAY=os.path.abspath(recording), BMX_SIM_HASH=hash_path, BMX_MUTE='1', BMX_NO_MOVIES='1')
    for k in ('BMX_KEYS', 'BMX_RECORD', 'BMX_SHOT', 'BMX_SHOT_FRAME', 'BMX_QUIT_FRAMES'):
        env.pop(k, None)
    if ini:
        env['BMX_CONFIG'] = os.path.abspath(ini)
    with open(save_path, 'wb') as f:
        f.write(save)
    subprocess.run([os.path.abspath(exe), 'Scene.Txt'], cwd=game_dir, env=env, timeout=timeout,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    log = os.path.join(os.path.dirname(os.path.abspath(exe)), 'bmxnative.log')
    if os.path.exists(log):
        shutil.copy(log, os.path.join(out_dir, f'{label}.log'))
    with open(hash_path, encoding='ascii') as f:
        return [line.split() for line in f if line.strip()]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game-dir', required=True)
    ap.add_argument('--recording', required=True)
    ap.add_argument('--ini', help='bmxnative.ini for every run (default: each exe\'s own)')
    ap.add_argument('--out', help='folder for the hash files and logs (default: a temporary one)')
    ap.add_argument('--timeout', type=int, default=3600)
    ap.add_argument('exes', nargs='+')
    args = ap.parse_args()

    save, pad, _ids = recording_save(args.recording)
    out_dir = args.out or tempfile.mkdtemp(prefix='xcheck-')
    os.makedirs(out_dir, exist_ok=True)
    save_path = os.path.join(args.game_dir, 'save', 'SAVE.DAT')
    with open(save_path, 'rb') as f:
        own_save = f.read()
    print(f'recording {args.recording}: pad {pad}, save {len(save)} bytes; results in {out_dir}')
    results = []
    try:
        for i, exe in enumerate(args.exes):
            hashes = run(exe, args.game_dir, args.recording, save, args.ini, out_dir, f'run{i}', args.timeout)
            print(f'  run{i}: {exe}: {len(hashes)} ticks')
            results.append(hashes)
    finally:
        with open(save_path, 'wb') as f:
            f.write(own_save)
    ok = len(results[0]) > 0
    for i, hashes in enumerate(results[1:], 1):
        first = next((a[0] for a, b in zip(results[0], hashes) if a != b), None)
        if first is not None:
            print(f'  run{i} differs from run0 from tick {first}')
            ok = False
        elif len(hashes) != len(results[0]):
            print(f'  run{i} has {len(hashes)} ticks, run0 {len(results[0])}')
            ok = False
        else:
            print(f'  run{i} identical to run0')
    print('identical' if ok else 'DIFFERENT')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
