#!/usr/bin/env python3
"""Cross-build check (docs/PORTING.md X0): run the same input on two or more bmxnative builds (or settings) and
compare their per-tick BMX_SIM_HASH files.

    py -I tools/xcheck.py --game-dir <game folder> --recording <file.bmxr> <run> [<run> ...] [--ini <ini>]
    py -I tools/xcheck.py --game-dir <game folder> --keys tools/scripts/ride.keys <run> [<run> ...]

A <run> is an exe, optionally preceded by KEY=VALUE environment settings for that run only, e.g.
`build/native64/bmxnative.exe BMX_GPU=null build/native64/bmxnative.exe` compares the windowed and the headless
null-GPU mode of one build (docs/PORTING.md "X5 part 3").

--recording: each run gets the recording's starting SAVE.DAT in <game folder>/save and BMX_REPLAY.
--keys: each run gets the BMX_KEYS script in the file (whitespace ignored), BMX_QUIT_FRAMES (--quit-frames,
default 4000) and the save given by --save (default `none`: no SAVE.DAT, a fresh game).
The folder's own SAVE.DAT is put back afterwards (or removed again if there was none). Every run also gets
BMX_SIM_HASH, BMX_MUTE and BMX_NO_MOVIES; runs are sequential, in the background (the window never takes the
focus). --args adds command-line arguments after Scene.Txt (e.g. "-r640.480.16"). --ref-hash <file> adds a
precomputed hash file (another machine's, e.g. data/simhash/ride.hash) as the reference before the
runs. The reference (or else the first run) is compared with the others; the report names the first tick where
another differs. Exit status 0 when all hash files are identical.

--shots 300,900,1500 also saves those shown frames of every run (BMX_SHOT, <label>-f<frame>.bmp) and compares
them byte for byte, so a renderer change that should not show is checked too. Use an ini with `vanilla = 1`:
the frame interpolation blends by wall-clock time, so with it frames differ from run to run (even then a frame
occasionally differs in a few pixels between two runs of one exe; compare such a frame against a second run).
A null-GPU run saves no screenshots.
"""
import argparse
import os
import shlex
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


def read_hash(path):
    with open(path, encoding='ascii') as f:
        return [line.split() for line in f if line.strip()]


def parse_runs(items):
    """[KEY=VALUE ...] exe groups -> [(exe, {KEY: VALUE})]."""
    runs, env = [], {}
    for item in items:
        key, eq, value = item.partition('=')
        if eq and key.replace('_', '').isalnum() and key.upper() == key and not os.path.exists(item):
            env[key] = value
            continue
        runs.append((item, env))
        env = {}
    if env:
        sys.exit(f'environment settings {env} are not followed by an exe')
    return runs


def run(exe, extra_env, game_dir, inputs, save, ini, out_dir, label, timeout, shots, exe_args):
    hash_path = os.path.join(os.path.abspath(out_dir), f'{label}.hash')
    save_path = os.path.join(game_dir, 'save', 'SAVE.DAT')
    env = dict(os.environ)
    for k in ('BMX_KEYS', 'BMX_REPLAY', 'BMX_RECORD', 'BMX_SHOT', 'BMX_SHOT_FRAME', 'BMX_QUIT_FRAMES', 'BMX_GPU'):
        env.pop(k, None)
    env.update(inputs)
    env.update(BMX_SIM_HASH=hash_path, BMX_MUTE='1', BMX_NO_MOVIES='1')
    if ini:
        env['BMX_CONFIG'] = os.path.abspath(ini)
    if shots:
        env.update(BMX_SHOT=os.path.join(os.path.abspath(out_dir), f'{label}-f%d.bmp'), BMX_SHOT_FRAME=shots)
    env.update(extra_env)
    if save is None:
        if os.path.exists(save_path):
            os.remove(save_path)
    else:
        with open(save_path, 'wb') as f:
            f.write(save)
    subprocess.run([os.path.abspath(exe), 'Scene.Txt'] + exe_args, cwd=game_dir, env=env, timeout=timeout,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    log = os.path.join(os.path.dirname(os.path.abspath(exe)), 'bmxnative.log')
    if os.path.exists(log):
        shutil.copy(log, os.path.join(out_dir, f'{label}.log'))
    return read_hash(hash_path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game-dir', required=True)
    source = ap.add_mutually_exclusive_group(required=True)
    source.add_argument('--recording', help='a BMX_RECORD file to replay')
    source.add_argument('--keys', help='a file with a BMX_KEYS script (e.g. tools/scripts/ride.keys)')
    ap.add_argument('--save', help='with --keys: the SAVE.DAT to start from, or "none" (default): no save')
    ap.add_argument('--quit-frames', type=int, default=4000, help='with --keys: BMX_QUIT_FRAMES (default 4000)')
    ap.add_argument('--args', default='', help='command-line arguments after Scene.Txt, e.g. "-r640.480.16"')
    ap.add_argument('--ref-hash', help='a precomputed BMX_SIM_HASH file as the reference (compared first)')
    ap.add_argument('--ini', help='bmxnative.ini for every run (default: each exe\'s own)')
    ap.add_argument('--out', help='folder for the hash files and logs (default: a temporary one)')
    ap.add_argument('--timeout', type=int, default=3600)
    ap.add_argument('--shots', help='shown frames to save and compare, e.g. 300,900,1500')
    ap.add_argument('runs', nargs='+', metavar='[KEY=VALUE ...] exe')
    args = ap.parse_args()

    runs = parse_runs(args.runs)
    if args.recording:
        save, pad, _ids = recording_save(args.recording)
        inputs = {'BMX_REPLAY': os.path.abspath(args.recording)}
        what = f'recording {args.recording}: pad {pad}, save {len(save)} bytes'
    else:
        with open(args.keys, encoding='ascii') as f:
            script = ''.join(f.read().split())
        save = None
        if args.save not in (None, 'none'):
            with open(args.save, 'rb') as f:
                save = f.read()
        inputs = {'BMX_KEYS': script, 'BMX_QUIT_FRAMES': str(args.quit_frames)}
        what = (f'keys {args.keys}: {args.quit_frames} frames, '
                f'save {"none" if save is None else f"{len(save)} bytes"}')
    out_dir = args.out or tempfile.mkdtemp(prefix='xcheck-')
    os.makedirs(out_dir, exist_ok=True)
    save_path = os.path.join(args.game_dir, 'save', 'SAVE.DAT')
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    own_save = None
    if os.path.exists(save_path):
        with open(save_path, 'rb') as f:
            own_save = f.read()
    print(f'{what}; results in {out_dir}')
    results, labels = [], []
    if args.ref_hash:
        results.append(read_hash(args.ref_hash))
        labels.append('ref')
        print(f'  ref: {args.ref_hash}: {len(results[0])} ticks')
    try:
        for i, (exe, extra_env) in enumerate(runs):
            label = f'run{i}'
            hashes = run(exe, extra_env, args.game_dir, inputs, save, args.ini, out_dir, label, args.timeout,
                         args.shots, shlex.split(args.args))
            settings = ''.join(f'{k}={v} ' for k, v in extra_env.items())
            print(f'  {label}: {settings}{exe}: {len(hashes)} ticks')
            results.append(hashes)
            labels.append(label)
    finally:
        if own_save is not None:
            with open(save_path, 'wb') as f:
                f.write(own_save)
        elif os.path.exists(save_path):
            os.remove(save_path)
    ok = len(results[0]) > 0
    for i, hashes in enumerate(results[1:], 1):
        first = next((a[0] for a, b in zip(results[0], hashes) if a != b), None)
        if first is not None:
            print(f'  {labels[i]} differs from {labels[0]} from tick {first}')
            ok = False
        elif len(hashes) != len(results[0]):
            print(f'  {labels[i]} has {len(hashes)} ticks, {labels[0]} {len(results[0])}')
            ok = False
        else:
            print(f'  {labels[i]} identical to {labels[0]}')
    for frame in (args.shots.split(',') if args.shots else []):
        ref = os.path.join(out_dir, f'run0-f{frame}.bmp')
        if not os.path.exists(ref):
            print(f'  frame {frame}: run0 saved no screenshot')
            ok = False
            continue
        with open(ref, 'rb') as f:
            want = f.read()
        for i in range(1, len(runs)):
            path = os.path.join(out_dir, f'run{i}-f{frame}.bmp')
            same = os.path.exists(path) and open(path, 'rb').read() == want
            if not same:
                print(f'  frame {frame}: run{i} differs from run0')
                ok = False
    print('identical' if ok else 'DIFFERENT')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
