#!/usr/bin/env python3
"""Build site-compatible Mejiro31 UF2s from this checkout using pinned Vial core."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

KB = Path(__file__).resolve().parent
ROOT = KB.parents[2]
VIAL_URL = 'https://github.com/vial-kb/vial-qmk.git'
VIAL_REF = 'dd43959ae5c08d8a28d38a1acf7b04e86b14a344'
ENGINE_URL = 'https://github.com/JEEBIS27/Mejiro_Engine.git'
# Remote HEAD follows the repository's default branch, including branch renames.
ENGINE_REF = 'HEAD'


def run(*args, cwd=None):
    subprocess.run(args, cwd=cwd, check=True)


def checkout(path, url, ref):
    if path.exists() and not (path / '.git').exists():
        raise ValueError(f'Refusing to use a non-Git cache directory: {path}')
    if not path.exists():
        run('git', 'init', str(path))
        run('git', 'remote', 'add', 'origin', url, cwd=path)
    current = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=path, capture_output=True, text=True)
    if ref == 'HEAD' or current.returncode or current.stdout.strip() != ref:
        run('git', 'fetch', '--depth', '1', url, ref, cwd=path)
        run('git', 'checkout', '--detach', 'FETCH_HEAD', cwd=path)
    return subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=path, text=True).strip()


def write_build_metadata(uf2, sources, variant, digest):
    metadata = {'format': 'mejiro31-build-v1', 'filename': uf2.name,
                'sha256': digest, 'sources': sources, **variant}
    uf2.with_suffix('.uf2.json').write_text(json.dumps(metadata, indent=2) + '\n')


def verify_uf2(path):
    raw = path.read_bytes()
    if not raw or len(raw) % 512:
        raise ValueError('Invalid UF2 length')
    addresses = set()
    payload = bytearray()
    for index in range(len(raw) // 512):
        block = raw[index * 512:(index + 1) * 512]
        magic1, magic2, flags, address, size, number, total, family = struct.unpack_from('<8I', block)
        if (magic1, magic2, struct.unpack_from('<I', block, 508)[0]) != (0x0a324655, 0x9e5d5157, 0x0ab16f30):
            raise ValueError('Invalid UF2 magic')
        if flags != 0x2000 or family != 0xe48bff56 or size != 256 or number != index or total != len(raw) // 512:
            raise ValueError('Invalid RP2040 UF2 metadata')
        if address % 256 or address < 0x10000000 or address + size > 0x10200000 - 8192 or address in addresses:
            raise ValueError('Invalid UF2 flash address')
        addresses.add(address)
        payload.extend(block[32:32 + size])
    if b'M31WEB01' not in payload:
        raise ValueError('The site protocol is missing')
    return hashlib.sha256(raw).hexdigest()


def prepare(work, submodules):
    vial, engine = work / 'vial', work / 'Mejiro_Engine'
    checkout(vial, VIAL_URL, VIAL_REF)
    engine_revision = checkout(engine, ENGINE_URL, ENGINE_REF)
    print(f'Mejiro_Engine ({ENGINE_URL}, remote {ENGINE_REF}): {engine_revision}', flush=True)
    dest = vial / 'keyboards/jeebis/mejiro31'
    marker = vial / '.mejiro31-vial-build.json'
    if dest.exists():
        if not marker.exists():
            raise ValueError('Refusing to replace a keyboard in an unmanaged Vial tree')
        shutil.rmtree(dest)
    shutil.copytree(KB, dest, ignore=shutil.ignore_patterns('target', '__pycache__', '*.uf2'))
    patch = (KB / 'keymaps/vial/plover-hid.patch').read_bytes()
    reverse = subprocess.run(['git', 'apply', '--reverse', '--check', '-'], cwd=vial, input=patch, capture_output=True)
    if reverse.returncode:
        subprocess.run(['git', 'apply', '--check', '-'], cwd=vial, input=patch, check=True)
        subprocess.run(['git', 'apply', '-'], cwd=vial, input=patch, check=True)
    # Allow cancellation during unlock and clear the polling barrier on lock.
    core = vial / 'quantum/via.c'
    text = core.read_text().replace('cmd != vial_unlock_start && cmd != vial_unlock_poll)', 'cmd != vial_unlock_start && cmd != vial_unlock_poll && cmd != vial_lock)')
    core.write_text(text)
    core = vial / 'quantum/vial.c'
    text = core.read_text()
    if 'case vial_lock: {\n            vial_unlock_in_progress = 0;' not in text:
        text = text.replace('case vial_lock: {', 'case vial_lock: {\n            vial_unlock_in_progress = 0;')
    core.write_text(text)
    # Named keymaps share the keyboard's Vial definition.
    build_vial = vial / 'builddefs/build_vial.mk'
    text = build_vial.read_text().replace('$(KEYMAP_PATH)/vial.json', '$(VIAL_DEFINITION)')
    build_vial.write_text(text)
    cargo = dest / 'steno/rust/Cargo.toml' 
    cargo.write_text(cargo.read_text().replace('../../../../../../Mejiro_Engine', str(engine)))
    for path in [dest / 'steno/engine.c', dest / 'steno/engine.h']:
        path.write_text(path.read_text().replace('../../../../../Mejiro_Engine/native/mejiro.h', str(engine / 'native/mejiro.h')))
    rules = dest / 'rules.mk'
    rules.write_text(rules.read_text().replace('$(abspath $(KEYBOARD_PATH_1)/../../../../Mejiro_Engine)', str(engine)))
    if submodules:
        run('git', 'submodule', 'update', '--init', '--depth', '1', 'lib/chibios', 'lib/chibios-contrib', 'lib/pico-sdk', 'lib/printf', cwd=vial)
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    marker.write_text(json.dumps({'vial': VIAL_REF, 'engine': engine_revision,
                                 'engine_url': ENGINE_URL, 'engine_ref': ENGINE_REF,
                                 'keyboard_head': revision}, indent=2) + '\n')
    return vial


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--work-dir', type=Path, default=Path(os.environ.get('MJI_VIAL_WORK_DIR', str(ROOT / '.build/mejiro31-vial'))))
    p.add_argument('--keymap', help='Existing Mejiro31 keymap name; uses its original defaults')
    p.add_argument('--action', choices=['build','flash','clean'], default='build')
    p.add_argument('--profile', choices=['jp', 'en', 'all'], default='all')
    p.add_argument('--layout', choices=['qwerty', 'graphite'], default='qwerty')
    p.add_argument('--jobs', type=int, default=4)
    p.add_argument('--prepare-only', action='store_true')
    p.add_argument('--skip-submodules', action='store_true', help='Only for host tests or an already initialized cache')
    args = p.parse_args()
    if args.keymap and (not args.keymap.replace('_','').replace('-','').isalnum() or not (KB / 'keymaps' / args.keymap / 'keymap.c').is_file()):
        p.error('Unknown Mejiro31 keymap')
    work = args.work_dir.resolve()
    if work == ROOT or work.is_relative_to(KB):
        p.error('The build cache must be outside the keyboard sources')
    work.mkdir(parents=True, exist_ok=True)
    vial = prepare(work, not args.skip_submodules)
    print(f'Prepared Vial source: {vial}', flush=True)
    if args.prepare_only:
        return
    output = ROOT / '.build'
    output.mkdir(exist_ok=True)
    sources = json.loads((vial / '.mejiro31-vial-build.json').read_text())
    variants = [(None, args.keymap)] if args.keymap else [(profile, 'vial') for profile in (['jp', 'en'] if args.profile == 'all' else [args.profile])]
    for profile, keymap in variants:
        shutil.rmtree(vial / f'.build/obj_jeebis_mejiro31_{keymap}', ignore_errors=True)
        target = f'jeebis/mejiro31:{keymap}' + (f':{args.action}' if args.action != 'build' else '')
        options = [] if args.keymap else [f'MEJIRO_PROFILE={int(profile == "en")}', f'MEJIRO_LAYOUT={int(args.layout == "graphite")}']
        run('make', target, f'-j{args.jobs}', *options, cwd=vial)
        if args.action == 'clean':
            continue
        filename = f'jeebis_mejiro31_{keymap}.uf2' if args.keymap else f'mejiro31_vial_{profile}_{args.layout}.uf2'
        uf2 = output / filename
        shutil.copyfile(vial / f'.build/jeebis_mejiro31_{keymap}.uf2', uf2)
        digest = verify_uf2(uf2)
        uf2.with_suffix('.uf2.sha256').write_text(f'{digest}  {uf2.name}\n')
        variant = {'keymap': keymap}
        if profile is not None:
            variant.update(profile=profile, layout=args.layout)
        write_build_metadata(uf2, sources, variant, digest)
        print(f'Verified site-compatible UF2: {uf2}', flush=True)
        if args.keymap:
            named_uf2 = ROOT / f'jeebis_mejiro31_{keymap}.uf2'
            shutil.copyfile(uf2, named_uf2)
            write_build_metadata(named_uf2, sources, variant, digest)


if __name__ == '__main__':
    main()
