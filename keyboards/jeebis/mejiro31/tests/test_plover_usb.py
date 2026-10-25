#!/usr/bin/env python3
"""Validate the actual linked USB descriptors, not a copied test descriptor."""
import struct
import sys
from elftools.elf.elffile import ELFFile


def check(path):
    with open(path, 'rb') as f:
        elf = ELFFile(f)
        symtab = elf.get_section_by_name('.symtab')

        def data(name):
            sym = symtab.get_symbol_by_name(name)[0]
            section = elf.get_section(sym['st_shndx'])
            offset = sym['st_value'] - section['sh_addr']
            return section.data()[offset:offset + sym['st_size']]

        config = data('ConfigurationDescriptor')
        assert struct.unpack_from('<H', config, 2)[0] == len(config)
        report = data('PloverReport')
        assert report == bytes.fromhex(
            '06 50 ff 0a 56 4c a1 01 85 50 15 00 25 01 75 01 '
            '95 40 05 0a 19 00 29 3f 81 02 c0')
        interfaces = {}
        current = None
        offset = 0
        while offset < len(config):
            length, kind = config[offset:offset+2]
            assert length >= 2
            descriptor = config[offset:offset+length]
            if kind == 4:
                current = descriptor[2]
                interfaces[current] = {'class': descriptor[5], 'hid': None, 'eps': []}
            elif kind == 0x21:
                interfaces[current]['hid'] = descriptor
            elif kind == 5:
                interfaces[current]['eps'].append(descriptor)
            offset += length
        assert len(interfaces) == config[4]
        assert all(i['class'] == 3 for i in interfaces.values()), 'CDC interface remains'
        plover = [(n, i) for n, i in interfaces.items()
                  if i['hid'] and struct.unpack_from('<H', i['hid'], 7)[0] == len(report)]
        assert len(plover) == 1
        number, interface = plover[0]
        assert number != 1, 'VIA Raw HID interface 1 was displaced'
        assert len(interface['eps']) == 1
        ep = interface['eps'][0]
        assert ep[2] & 0x80 and ep[3] == 3
        assert struct.unpack_from('<H', ep, 4)[0] == 9 and ep[6] == 1
        raw = interfaces[1]
        assert len(raw['eps']) == 2
        assert all(struct.unpack_from('<H', e, 4)[0] == 32 for e in raw['eps'])
        lut = data('usb_endpoint_interface_lut')
        width = len(lut) // len(interfaces)
        assert width in (1, 2, 4) and len(lut) == width * len(interfaces)
        indexes = struct.unpack('<' + {1: 'B', 2: 'H', 4: 'I'}[width] * len(interfaces), lut)
        assert indexes[number] != indexes[1]
        for name in ['process_steno', 'virtser_send', 'send_steno_chord_user']:
            assert not symtab.get_symbol_by_name(name), name + ' still linked'
        assert symtab.get_symbol_by_name('send_plover_hid')
        print(f'{path}: HID report 0x50/64 bits, dedicated 9-byte IN endpoint, VIA interface 1, no serial steno OK')


if __name__ == '__main__':
    for arg in sys.argv[1:]:
        check(arg)
