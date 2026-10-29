"""Embed upstream's data package without changing any bytes or requiring host ICU tools."""
from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:])
block_size = 4096
data = source.read_bytes()
if len(data) < 32 or data[2:4] != bytes((0xDA, 0x27)) or data[8] != 0:
    raise ValueError("Expected the pinned little-endian ICU data package")

# C permits a full-size character array initialized without a trailing NUL.
# Separate rows keep literals below MSVC's limit; only the last row is padded.
# The rows remain contiguous and the package header describes its real size.
temporary = destination.with_suffix(".tmp")
with temporary.open("w", encoding="ascii", newline="\n") as output:
    output.write('/* Generated from unmodified upstream ICU data. */\n')
    output.write('#include "unicode/utypes.h"\n')
    output.write('#if defined(_MSC_VER)\n__declspec(align(16))\n#else\n_Alignas(16)\n#endif\n')
    output.write(f'const unsigned char U_ICUDATA_ENTRY_POINT[][{block_size}] = {{\n')
    for offset in range(0, len(data), block_size):
        block = data[offset:offset + block_size]
        output.write('    "' + ''.join(f'\\x{value:02X}' for value in block) + '",\n')
    output.write('};\n')
temporary.replace(destination)
