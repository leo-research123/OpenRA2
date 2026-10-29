"""Small authored MIX, CSF, INI and preview inputs; no installation data."""
import base64
import struct
import zlib

def mix(files):
    rows = []
    for name, data in files.items():
        name = name.upper().encode("ascii")
        remainder = len(name) % 4
        if remainder:
            name += bytes([remainder]) + name[len(name) - remainder:][:1] * (3 - remainder)
        identifier = zlib.crc32(name)
        rows.append((identifier, data))
    rows.sort(key=lambda row: struct.unpack("<i", struct.pack("<I", row[0]))[0])
    entries, body = b"", b""
    for identifier, data in rows:
        entries += struct.pack("<III", identifier, len(body), len(data))
        body += data
    return struct.pack("<IHI", 0, len(rows), len(body)) + entries + body


def csf(labels):
    data = struct.pack("<6I", 0x43534620, 3, len(labels), len(labels), 0, 9)
    for name, text in labels.items():
        name = name.encode("ascii")
        encoded = text.encode("utf-16le")
        data += struct.pack("<3I", 0x4C424C20, 1, len(name)) + name
        data += struct.pack("<2I", 0x53545220, len(encoded) // 2) + bytes(value ^ 0xFF for value in encoded)
    return data


def text_ini_fixtures():
    result = {}
    for codec, phrase in (
        ("gbk", "我们需要完成这个任务，保护基地，指挥部队击败敌人。"),
        ("big5", "我們需要完成這個任務，保護基地，指揮部隊擊敗敵人。"),
        ("shift_jis", "この任務を完了して基地を守り、部隊を指揮して敵を倒してください。"),
        ("cp1251", "Необходимо выполнить задание, защитить базу и уничтожить вражеские войска."),
    ):
        # Real legacy bytes plus UTF-8 edits in the same parsed document.
        result[f"text-{codec}.ini"] = (
            ("[Text]\nName=" + phrase + "\nSample=" + phrase * 16 + "\n").encode(codec)
            + "Edited=UTF-8 编辑\n".encode("utf-8")
        )
    result["text-short.ini"] = "[Text]\nName=中文\n".encode("gbk")
    result["text-invalid.ini"] = b"[Text]\nName=\xff\n"
    return result


def skirmish_map():
    # One literal-only LZO1X block contains four authored RGB pixels.
    pixels = bytes([255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255])
    literal = bytes([17 + len(pixels)]) + pixels + bytes([0x11, 0, 0])
    packed = base64.b64encode(struct.pack("<HH", len(literal), len(pixels)) + literal)
    return (b"[Map]\nSize=0,0,8,12\nLocalSize=0,0,8,12\nTheater=TEMPERATE\n"
            b"[Basic]\nName=Authored lobby map\nMultiplayerOnly=yes\n"
            b"[Waypoints]\n0=10009\n1=12011\n[Preview]\nSize=0,0,2,2\n"
            b"[PreviewPack]\n1=" + packed + b"\n")
