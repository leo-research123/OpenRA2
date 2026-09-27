"""The MSVC image fixture supports EH records without masking null accesses."""
import unittest

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

from compare_image_pal import initialize_exception_chain


class ImageEnvironmentTests(unittest.TestCase):
    def setUp(self):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.uc.mem_map(0x1000, 0x1000)
        initialize_exception_chain(self.uc)

    def execute(self, code):
        self.uc.mem_write(0x1000, code)
        self.uc.emu_start(0x1000, 0x1000 + len(code))

    def test_exception_record_roundtrip(self):
        self.execute(b"\x64\xa1\x00\x00\x00\x00")  # mov eax, fs:[0]
        self.assertEqual(self.uc.reg_read(UC_X86_REG_EAX), 0xFFFFFFFF)
        self.uc.reg_write(UC_X86_REG_EAX, 0x12345678)
        self.execute(b"\x64\xa3\x00\x00\x00\x00")  # mov fs:[0], eax
        self.uc.reg_write(UC_X86_REG_EAX, 0)
        self.execute(b"\x64\xa1\x00\x00\x00\x00")
        self.assertEqual(self.uc.reg_read(UC_X86_REG_EAX), 0x12345678)

    def test_ordinary_null_access_still_faults(self):
        for code in (b"\xa1\x00\x00\x00\x00", b"\xa3\x00\x00\x00\x00"):
            with self.subTest(code=code), self.assertRaises(UcError):
                self.execute(code)

    def test_stack_keeps_32_bit_addressing(self):
        self.uc.mem_map(0x3000000, 0x1000)
        self.uc.reg_write(UC_X86_REG_ESP, 0x3000800)
        self.uc.reg_write(UC_X86_REG_EAX, 0x12345678)
        self.execute(b"\x50\x31\xc0\x58")  # push eax; xor eax,eax; pop eax
        self.assertEqual(self.uc.reg_read(UC_X86_REG_EAX), 0x12345678)
        self.assertEqual(self.uc.reg_read(UC_X86_REG_ESP), 0x3000800)


if __name__ == "__main__":
    unittest.main()
