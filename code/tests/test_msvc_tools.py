"""Unit tests for migration tooling, independent of SDK and original game data."""
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def load(name, file):
    spec = importlib.util.spec_from_file_location(name, ROOT.parent/'scripts'/file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

abi = load('abi_check', 'check-msvc-abi.py')

def fixture_ir(bytes_mode=False):
    text = 'target triple = "i686-pc-windows-msvc19.33.0"\n'
    for name, slot in {**abi.EXPECTED, **abi.STDCALL_SLOTS}.items():
        text += f'define void @RA2ABI_{name}(ptr %p) {{\n'
        adjustment = {'TeleportDestination': 0x04, 'FlyDestination': 0x04,
                      'HouseConnectionEnum': 0x2C, 'HouseConnectionFind': 0x2C}.get(name)
        if adjustment is not None:
            text += f'  %base = getelementptr inbounds i8, ptr %p, i32 {adjustment}\n'
        if slot:
            kind, index = ('i8', slot*4) if bytes_mode else ('ptr', slot)
            text += f'  %f = getelementptr inbounds nuw {kind}, ptr %p, i32 {index}\n'
        convention = 'x86_stdcallcc' if name in abi.STDCALL_SLOTS else 'x86_thiscallcc'
        text += f'  call {convention} void %fn(ptr %p)\n  ret void\n}}\n'
    for name, convention in abi.DIRECT_CONVENTIONS.items():
        # LLVM normally omits the default C calling convention.
        prefix = '' if convention == 'ccc' else convention + ' '
        text += f'define void @RA2ABI_{name}(ptr %p) {{\n'
        text += f'  call {prefix}void @{name}(ptr %p)\n  ret void\n}}\n'
    return text

class ABIToolTests(unittest.TestCase):
    def test_pointer_slot_ir(self):
        self.assertEqual(abi.check_ir(fixture_ir()), {**abi.EXPECTED, **abi.STDCALL_SLOTS})

    def test_byte_offset_ir(self):
        self.assertEqual(abi.check_ir(fixture_ir(True)), {**abi.EXPECTED, **abi.STDCALL_SLOTS})

    def test_reject_wrong_target(self):
        with self.assertRaisesRegex(ValueError, 'IR does not target 32-bit Windows MSVC'):
            abi.check_ir(fixture_ir().replace('i686-pc-windows-msvc', 'x86_64-pc-windows-msvc'))

    def test_reject_shifted_slot(self):
        with self.assertRaisesRegex(ValueError, 'RA2ABI_Read: expected vtable slot 9'):
            abi.check_ir(fixture_ir().replace('ptr %p, i32 9\n', 'ptr %p, i32 10\n'))

    def test_reject_wrong_calling_convention(self):
        text = fixture_ir()
        start = text.index('define void @RA2ABI_Name(')
        text = text[:start] + text[start:].replace('x86_thiscallcc', 'ccc', 1)
        with self.assertRaisesRegex(ValueError, 'RA2ABI_Name did not emit x86_thiscallcc'):
            abi.check_ir(text)

    def test_reject_missing_probe(self):
        with self.assertRaisesRegex(ValueError, 'Missing RA2ABI_Read in IR'):
            abi.check_ir(fixture_ir().replace('RA2ABI_Read(', 'NotAProbe('))

    def test_reject_wrong_stdcall_convention(self):
        with self.assertRaisesRegex(ValueError, 'RA2ABI_QueryInterface did not emit x86_stdcallcc'):
            abi.check_ir(fixture_ir().replace('call x86_stdcallcc', 'call x86_thiscallcc', 1))

    def test_reject_missing_stdcall_probe(self):
        with self.assertRaisesRegex(ValueError, 'Missing RA2ABI_QueryInterface in IR'):
            abi.check_ir(fixture_ir().replace('RA2ABI_QueryInterface(', 'NotAProbe('))

    def test_reject_wrong_direct_convention(self):
        for name, convention in abi.DIRECT_CONVENTIONS.items():
            with self.subTest(probe=name):
                prefix = '' if convention == 'ccc' else convention + ' '
                wrong = 'x86_stdcallcc' if convention == 'ccc' else 'ccc'
                text = fixture_ir().replace(
                    f'call {prefix}void @{name}(', f'call {wrong} void @{name}(')
                with self.assertRaisesRegex(ValueError, f'RA2ABI_{name} did not emit {convention}'):
                    abi.check_ir(text)

    def test_reject_missing_direct_probe(self):
        for name in abi.DIRECT_CONVENTIONS:
            with self.subTest(probe=name):
                with self.assertRaisesRegex(ValueError, f'Missing RA2ABI_{name} in IR'):
                    abi.check_ir(fixture_ir().replace(f'RA2ABI_{name}(', 'NotAProbe('))

if __name__ == '__main__':
    unittest.main()
