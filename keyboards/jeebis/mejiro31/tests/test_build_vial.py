#!/usr/bin/env python3
"""Check engine refresh and the metadata shipped with named UF2 builds."""
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('build_vial', Path(__file__).resolve().parents[1] / 'build_vial.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def git(path, *args):
    return subprocess.check_output(['git', '-C', str(path), *args], text=True).strip()


class BuildTests(unittest.TestCase):
    def test_engine_follows_remote_default_and_fails_without_remote(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            remote, cache = tmp / 'remote', tmp / 'cache'
            remote.mkdir()
            git(remote, 'init', '-b', 'first')
            git(remote, 'config', 'user.name', 'Build test')
            git(remote, 'config', 'user.email', 'build@example.invalid')
            source = remote / 'engine.rs'
            source.write_text('first')
            git(remote, 'add', '.')
            git(remote, 'commit', '-m', 'first')
            first = builder.checkout(cache, str(remote), 'HEAD')
            self.assertEqual(first, git(remote, 'rev-parse', 'HEAD'))
            source.write_text('second')
            git(remote, 'commit', '-am', 'second')
            second = builder.checkout(cache, str(remote), 'HEAD')
            self.assertNotEqual(first, second)
            self.assertEqual((cache / 'engine.rs').read_text(), 'second')
            git(remote, 'checkout', '-b', 'new-default')
            source.write_text('new default')
            git(remote, 'commit', '-am', 'new default')
            latest = builder.checkout(cache, str(remote), 'HEAD')
            self.assertEqual(latest, git(remote, 'rev-parse', 'HEAD'))
            with self.assertRaises(subprocess.CalledProcessError):
                builder.checkout(cache, str(tmp / 'missing'), 'HEAD')

    def test_named_uf2_metadata_matches_both_output_copies(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            kb = root / 'keyboard'
            (kb / 'keymaps/jp_default').mkdir(parents=True)
            (kb / 'keymaps/jp_default/keymap.c').touch()
            vial = root / 'work/vial'
            (vial / '.build').mkdir(parents=True)
            sources = {'engine': 'a' * 40, 'engine_ref': 'HEAD', 'vial': 'b' * 40}
            (vial / '.mejiro31-vial-build.json').write_text(json.dumps(sources))
            block = bytearray(512)
            struct.pack_into('<8I', block, 0, 0x0a324655, 0x9e5d5157, 0x2000,
                             0x10000000, 256, 0, 1, 0xe48bff56)
            block[32:40] = b'M31WEB01'
            struct.pack_into('<I', block, 508, 0x0ab16f30)
            (vial / '.build/jeebis_mejiro31_jp_default.uf2').write_bytes(block)
            # A rejected ELF must never be copied or advertised as verified.
            with patch.object(builder, 'ROOT', root), patch.object(builder, 'KB', kb), \
                 patch.object(builder, 'prepare', return_value=vial), \
                 patch.object(builder, 'run', side_effect=[None, subprocess.CalledProcessError(1, 'USB ELF check')]) as commands, \
                 patch('sys.argv', ['build_vial.py', '--work-dir', str(root / 'work'), '--keymap', 'jp_default']):
                with self.assertRaises(subprocess.CalledProcessError):
                    builder.main()
                self.assertEqual(commands.call_args.args[1:],
                                 (str(kb / 'tests/test_plover_usb.py'),
                                  str(vial / '.build/jeebis_mejiro31_jp_default.elf')))
            self.assertFalse((root / '.build/mejiro31_jp.uf2').exists())
            self.assertFalse((root / 'mejiro31_jp.uf2').exists())
            with patch.object(builder, 'ROOT', root), patch.object(builder, 'KB', kb), \
                 patch.object(builder, 'prepare', return_value=vial), patch.object(builder, 'run'), \
                 patch('sys.argv', ['build_vial.py', '--work-dir', str(root / 'work'), '--keymap', 'jp_default']):
                builder.main()
            for uf2 in [root / 'mejiro31_jp.uf2',
                        root / '.build/mejiro31_jp.uf2']:
                metadata = json.loads(uf2.with_suffix('.uf2.json').read_text())
                self.assertEqual(metadata['sources'], sources)
                self.assertEqual(metadata['filename'], uf2.name)
                self.assertEqual(metadata['keymap'], 'jp_default')
                self.assertEqual(metadata['sha256'], builder.verify_uf2(uf2))
            self.assertFalse((root / 'work/artifacts').exists())
            checksum = root / '.build/mejiro31_jp.uf2.sha256'
            self.assertEqual(checksum.read_text().split()[0], metadata['sha256'])


    def test_local_engine_preparation_uses_working_tree_and_records_changes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            engine, kb, work = root / 'engine local', root / 'keyboard', root / 'work'
            vial = work / 'vial'
            for relative in ['Cargo.toml', 'Cargo.lock', 'native/Cargo.toml',
                             'native/mejiro.h', 'src/lib.rs', 'native/src/lib.rs']:
                path = engine / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('initial')
            for relative, text in {
                'keymaps/vial/plover-hid.patch': '',
                'keymaps/vial/usb-queue-alignment.patch': '',
                'steno/rust/Cargo.toml': '../../../../../../Mejiro_Engine',
                'steno/engine.c': '../../../../../Mejiro_Engine/native/mejiro.h',
                'steno/engine.h': '../../../../../Mejiro_Engine/native/mejiro.h',
                'rules.mk': '$(abspath $(KEYBOARD_PATH_1)/../../../../Mejiro_Engine)',
            }.items():
                path = kb / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            for relative in ['quantum/via.c', 'quantum/vial.c', 'builddefs/build_vial.mk']:
                path = vial / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.touch()
            def output(args, **kwargs):
                return ' M src/lib.rs\n' if 'status' in args else 'a' * 40 + '\n'
            with patch.object(builder, 'ROOT', root), patch.object(builder, 'KB', kb), \
                 patch.object(builder, 'checkout') as checkout, \
                 patch.object(builder.subprocess, 'run') as command, \
                 patch.object(builder.subprocess, 'check_output', side_effect=output):
                command.return_value.returncode = 0
                builder.prepare(work, False, engine)
                checkout.assert_called_once_with(vial, builder.VIAL_URL, builder.VIAL_REF)
                marker = vial / '.mejiro31-vial-build.json'
                first = json.loads(marker.read_text())
                self.assertEqual(first['engine_ref'], 'local')
                self.assertTrue(first['engine_dirty'])
                self.assertEqual(first['engine_source'], str(engine))
                dest = vial / 'keyboards/jeebis/mejiro31'
                self.assertEqual((dest / 'steno/rust/Cargo.toml').read_text(), str(engine))
                self.assertEqual((dest / 'steno/engine.h').read_text(), str(engine / 'native/mejiro.h'))
                (engine / 'src/lib.rs').write_text('uncommitted edit')
                builder.prepare(work, False, engine)
                second = json.loads(marker.read_text())
                self.assertNotEqual(first['engine_source_sha256'], second['engine_source_sha256'])

    def test_invalid_local_engine_fails_before_fetching(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(builder, 'checkout') as checkout:
            with self.assertRaises(ValueError):
                builder.prepare(Path(tmp) / 'work', False, Path(tmp) / 'missing')
            checkout.assert_not_called()


if __name__ == '__main__':
    unittest.main()
