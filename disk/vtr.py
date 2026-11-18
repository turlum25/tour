#!/usr/bin/env python3
"""
ttfs.py - Read/Write Linux manager for TOUR's TTFS v1 disk images.

Commands:
  python3 ttfs.py info disk.img
  python3 ttfs.py ls disk.img [directory]
  python3 ttfs.py cat disk.img filename
  python3 ttfs.py get disk.img filename [-o output]
  python3 ttfs.py put disk.img local_file [filename] [-f]
  python3 ttfs.py mkdir disk.img directory
  python3 ttfs.py rm disk.img filename
  python3 ttfs.py format disk.img

This follows the on-disk layout in TOUR's supplied ttfs.h.
"""

import argparse
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

SECTOR_SIZE = 512
INDEX_FIRST_SECTOR = 11
INDEX_LAST_SECTOR = 2048
DATA_FIRST_SECTOR = 2049
MAX_LBA = 0x0FFFFFFF
FILENAME_SIZE = 32
ENTRY_SIZE = 40
ENTRIES_PER_SECTOR = 12
INDEX_SECTOR_COUNT = INDEX_LAST_SECTOR - INDEX_FIRST_SECTOR + 1
MAX_FILES = INDEX_SECTOR_COUNT * ENTRIES_PER_SECTOR
UINT32_MAX = 0xFFFFFFFF


class TTFSException(Exception):
    pass


@dataclass
class Entry:
    name: str
    start_lba: int
    size: int
    slot: int

    @property
    def sector_count(self):
        return (self.size + SECTOR_SIZE - 1) // SECTOR_SIZE


class TTFS:
    def __init__(self, image_path, mode="rb"):
        self.path = Path(image_path)
        self.mode = mode
        if "w" in mode or "+" in mode or mode.startswith("r"):
            pass
        else:
            mode = "rb"
        
        try:
            self.file = self.path.open(mode)
        except OSError as exc:
            raise TTFSException(f"cannot open image: {exc}") from exc

        try:
            self.image_size = self.path.stat().st_size
            if mode != "wb" and self.image_size >= (INDEX_FIRST_SECTOR + 1) * SECTOR_SIZE:
                self._check_header()
                self.entries = self._read_index()
            else:
                self.entries = []
        except Exception:
            self.file.close()
            raise

    def close(self):
        self.file.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def _read_at(self, offset, length):
        self.file.seek(offset)
        data = self.file.read(length)
        if len(data) != length:
            raise TTFSException(
                f"image is truncated: wanted {length} bytes at offset {offset}, got {len(data)}"
            )
        return data

    def _write_at(self, offset, data):
        self.file.seek(offset)
        self.file.write(data)

    def _read_sector(self, lba):
        if lba < 0 or lba > MAX_LBA:
            raise TTFSException(f"LBA {lba} is outside TTFS's 28-bit LBA range")
        return self._read_at(lba * SECTOR_SIZE, SECTOR_SIZE)

    def _write_sector(self, lba, data):
        if lba < 0 or lba > MAX_LBA:
            raise TTFSException(f"LBA {lba} is outside TTFS's 28-bit LBA range")
        if len(data) != SECTOR_SIZE:
            raise TTFSException(f"invalid sector payload size: {len(data)}")
        self._write_at(lba * SECTOR_SIZE, data)

    def _check_header(self):
        sector = self._read_sector(INDEX_FIRST_SECTOR)
        if sector[:4] != b"TTFS":
            raise TTFSException(f"no TTFS signature at sector {INDEX_FIRST_SECTOR}")
        if sector[4] != 1:
            raise TTFSException(f"unsupported TTFS version {sector[4]} (this reader supports v1)")

    def _get_slot_offset(self, slot):
        sector_offset = slot // ENTRIES_PER_SECTOR
        local_slot = slot % ENTRIES_PER_SECTOR
        offset = local_slot * ENTRY_SIZE
        return (offset + 8) if sector_offset == 0 else offset

    def _decode_entry(self, raw, slot):
        name_bytes = raw[:FILENAME_SIZE]
        nul = name_bytes.find(b"\0")
        if nul < 0:
            raise TTFSException(f"index entry {slot} has no filename terminator")
        if nul == 0:
            return None

        try:
            name = name_bytes[:nul].decode("utf-8")
        except UnicodeDecodeError:
            name = name_bytes[:nul].decode("latin-1")

        start_lba, size = struct.unpack_from("<II", raw, FILENAME_SIZE)
        sectors = (size + SECTOR_SIZE - 1) // SECTOR_SIZE

        if sectors == 0:
            if start_lba != 0:
                raise TTFSException(f"entry {slot} ({name!r}) is empty but has start LBA {start_lba}")
        else:
            if start_lba < DATA_FIRST_SECTOR:
                raise TTFSException(f"entry {slot} ({name!r}) points into reserved/index sectors")
            if sectors - 1 > MAX_LBA or start_lba > MAX_LBA - (sectors - 1):
                raise TTFSException(f"entry {slot} ({name!r}) exceeds TTFS's maximum LBA")

        return Entry(name, start_lba, size, slot)

    def _read_index(self):
        entries = []
        seen = set()

        for sector_offset in range(INDEX_SECTOR_COUNT):
            lba = INDEX_FIRST_SECTOR + sector_offset
            sector = self._read_sector(lba)

            first_slot = sector_offset * ENTRIES_PER_SECTOR
            base_offset = 8 if sector_offset == 0 else 0

            for local_slot in range(ENTRIES_PER_SECTOR):
                slot = first_slot + local_slot
                offset = base_offset + local_slot * ENTRY_SIZE
                raw = sector[offset:offset + ENTRY_SIZE]
                if len(raw) != ENTRY_SIZE:
                    raise TTFSException(f"index entry {slot} crosses sector boundary")

                entry = self._decode_entry(raw, slot)
                if entry is None:
                    continue
                if entry.name in seen:
                    raise TTFSException(f"duplicate filename in index: {entry.name!r}")
                seen.add(entry.name)
                entries.append(entry)

        return entries

    def format_disk(self):
        """Initializes a new TTFS layout inside the image file."""
        min_size = DATA_FIRST_SECTOR * SECTOR_SIZE
        if self.image_size < min_size:
            self.file.truncate(min_size)
            self.image_size = min_size

        # Write clean empty index blocks
        zero_sector = bytearray(SECTOR_SIZE)
        for offset in range(INDEX_SECTOR_COUNT):
            self._write_sector(INDEX_FIRST_SECTOR + offset, zero_sector)

        # Write master signature configuration header
        hdr = bytearray(SECTOR_SIZE)
        hdr[:5] = b"TTFS\x01"
        self._write_sector(INDEX_FIRST_SECTOR, hdr)
        self.entries = []

    def _find_free_slot(self):
        occupied_slots = {e.slot for e in self.entries}
        for slot in range(MAX_FILES):
            if slot not in occupied_slots:
                return slot
        return None

    def _find_data_run(self, needed_sectors, ignored_slot=-1):
        candidate = DATA_FIRST_SECTOR
        max_allocatable = MAX_LBA - (needed_sectors - 1)

        while candidate <= max_allocatable:
            moved = False
            for entry in self.entries:
                if entry.slot == ignored_slot or entry.sector_count == 0:
                    continue
                
                # Check overlapping block intervals
                if candidate < entry.start_lba + entry.sector_count and candidate + needed_sectors > entry.start_lba:
                    candidate = entry.start_lba + entry.sector_count
                    moved = True
                    break
            if not moved:
                return candidate
        return None

    def _write_entry(self, slot, name, start_lba, size):
        sector_offset = slot // ENTRIES_PER_SECTOR
        lba = INDEX_FIRST_SECTOR + sector_offset
        sector_data = bytearray(self._read_sector(lba))
        offset = self._get_slot_offset(slot)

        entry_bytes = bytearray(ENTRY_SIZE)
        name_encoded = name.encode("utf-8")[:FILENAME_SIZE - 1]
        entry_bytes[:len(name_encoded)] = name_encoded
        struct.pack_into("<II", entry_bytes, FILENAME_SIZE, start_lba, size)

        sector_data[offset:offset + ENTRY_SIZE] = entry_bytes
        self._write_sector(lba, sector_data)

    def _clear_entry(self, slot):
        sector_offset = slot // ENTRIES_PER_SECTOR
        lba = INDEX_FIRST_SECTOR + sector_offset
        sector_data = bytearray(self._read_sector(lba))
        offset = self._get_slot_offset(slot)

        sector_data[offset:offset + ENTRY_SIZE] = b"\x00" * ENTRY_SIZE
        self._write_sector(lba, sector_data)

    def write_file(self, name, data, overwrite=False):
        if len(name.encode("utf-8")) >= FILENAME_SIZE:
            raise TTFSException(f"filename too long: max {FILENAME_SIZE - 1} bytes")

        existing = [e for e in self.entries if e.name == name]
        existing_entry = existing[0] if existing else None

        if existing_entry and not overwrite:
            raise TTFSException(f"file already exists: {name}")

        needed_sectors = (len(data) + SECTOR_SIZE - 1) // SECTOR_SIZE
        start_lba = 0

        if needed_sectors > 0:
            ignored_slot = existing_entry.slot if existing_entry else -1
            if existing_entry and existing_entry.sector_count >= needed_sectors:
                start_lba = existing_entry.start_lba
            else:
                start_lba = self._find_data_run(needed_sectors, ignored_slot)
                if start_lba is None:
                    raise TTFSException("no contiguous free space available on image disk")

            # Expand backing host storage if allocations demand it
            required_size = (start_lba + needed_sectors) * SECTOR_SIZE
            if self.image_size < required_size:
                self.file.truncate(required_size)
                self.image_size = required_size

            # Write raw data block contents out to storage
            self._write_at(start_lba * SECTOR_SIZE, data)
            
            # Zero out any tail slack remainder spaces within trailing sector boundaries
            slack = len(data) % SECTOR_SIZE
            if slack > 0:
            # Zero out any tail slack remainder spaces within trailing sector boundaries
                slack = len(data) % SECTOR_SIZE
            if slack > 0:
                self._write_at(start_lba * SECTOR_SIZE + len(data), b"\x00" * (SECTOR_SIZE - slack))

        slot = existing_entry.slot if existing_entry else self._find_free_slot()
        if slot is None:
            raise TTFSException("TTFS volume capacity reached: index directory table full")
        self._write_entry(slot, name, start_lba, len(data))
        self.entries = self._read_index()

    def remove_file(self, name):
        matches = [e for e in self.entries if e.name == name]
        if not matches:
            raise TTFSException(f"file not found: {name}")
        entry = matches[0]
        self._clear_entry(entry.slot)
        self.entries = self._read_index()

    def read_file(self, name):
        matches = [entry for entry in self.entries if entry.name == name]
        if not matches:
            raise TTFSException(f"file not found: {name}")
        entry = matches[0]
        if entry.size == 0:
            return b""
        offset = entry.start_lba * SECTOR_SIZE
        return self._read_at(offset, entry.size)

    def list_directory(self, directory=""):
        directory = directory.strip("/")
        prefix = directory + "/" if directory else ""
        found = {}
        for entry in self.entries:
            if not entry.name.startswith(prefix):
                continue
            remainder = entry.name[len(prefix):]
            if not remainder:
                continue
            component, separator, _tail = remainder.partition("/")
            is_directory = bool(separator)
            if component not in found:
                found[component] = {
                    "directory": is_directory,
                    "size": entry.size if not is_directory else 0,
                }
            elif is_directory:
                found[component]["directory"] = True
                found[component]["size"] = 0
        return sorted(found.items(), key=lambda item: (not item[1]["directory"], item[0].lower()))

    def print_info(self):
        print(f"Image:              {self.path}")
        print(f"Image size:         {self.image_size:,} bytes")
        print(f"TTFS version:       1")
        print(f"Index sectors:      {INDEX_FIRST_SECTOR}-{INDEX_LAST_SECTOR}")
        print(f"Data starts at:     sector {DATA_FIRST_SECTOR}")
        print(f"Index capacity:     {MAX_FILES:,} entries")
        print(f"Files indexed:      {len(self.entries):,}")
        print(f"Sector size:        {SECTOR_SIZE} bytes")


# Command Processors

def command_info(args):
    with TTFS(args.image, "rb") as fs:
        fs.print_info()


def command_ls(args):
    with TTFS(args.image, "rb") as fs:
        directory = args.directory or ""
        if directory.strip("/") and not any(
            e.name.startswith(directory.strip("/") + "/") for e in fs.entries
        ):
            raise TTFSException(f"directory not found: {directory}")
        rows = fs.list_directory(directory)
        if not rows:
            print("(empty)")
            return
        for name, details in rows:
            if details["directory"]:
                print(f"{name + '/':<34} ")
            else:
                print(f"{name:<34} {details['size']:>12,} bytes")


def command_cat(args):
    with TTFS(args.image, "rb") as fs:
        data = fs.read_file(args.filename)
        try:
            sys.stdout.buffer.write(data)
        except BrokenPipeError:
            return


def command_get(args):
    with TTFS(args.image, "rb") as fs:
        data = fs.read_file(args.filename)
    output = Path(args.output) if args.output else Path(args.filename).name
    if output.exists() and not args.force:
        raise TTFSException(f"output already exists: {output} (use --force to overwrite)")
    try:
        output.write_bytes(data)
    except OSError as exc:
        raise TTFSException(f"cannot write {output}: {exc}") from exc
    print(f"extracted {args.filename!r} -> {output} ({len(data):,} bytes)")


def command_put(args):
    local_path = Path(args.local_file)
    if not local_path.is_file():
        raise TTFSException(f"local file does not exist: {local_path}")
    data = local_path.read_bytes()
    dest_name = args.filename if args.filename else local_path.name
    dest_name = dest_name.replace("\\", "/").strip("/")
    with TTFS(args.image, "r+b") as fs:
        fs.write_file(dest_name, data, overwrite=args.force)
    print(f"added local file {local_path} -> internal file {dest_name!r} ({len(data):,} bytes)")


def command_mkdir(args):
    dirname = args.directory.strip("/") + "/.placeholder"
    with TTFS(args.image, "r+b") as fs:
        fs.write_file(dirname, b"", overwrite=True)
    print(f"created directory layout for {args.directory!r}")


def command_rm(args):
    with TTFS(args.image, "r+b") as fs:
        fs.remove_file(args.filename)
    print(f"removed file {args.filename!r} from filesystem index")


def command_format(args):
    with TTFS(args.image, "r+b") as fs:
        fs.format_disk()
    print(f"successfully formatted a clean TTFS volume structure inside {args.image}")


def main():
    parser = argparse.ArgumentParser(description="Read/Write TOUR TTFS v1 disk images on Linux.")
    sub = parser.add_subparsers(dest="command", required=True)
    p_info = sub.add_parser("info", help="show TTFS image information")
    p_info.add_argument("image")
    p_info.set_defaults(func=command_info)
    p_ls = sub.add_parser("ls", help="list files in a TTFS directory")
    p_ls.add_argument("image")
    p_ls.add_argument("directory", nargs="?", default="")
    p_ls.set_defaults(func=command_ls)
    p_cat = sub.add_parser("cat", help="write a file's raw contents to stdout")
    p_cat.add_argument("image")
    p_cat.add_argument("filename")
    p_cat.set_defaults(func=command_cat)
    p_get = sub.add_parser("get", help="extract a file from the image")
    p_get.add_argument("image")
    p_get.add_argument("filename")
    p_get.add_argument("-o", "--output")
    p_get.add_argument("-f", "--force", action="store_true")
    p_get.set_defaults(func=command_get)
    p_put = sub.add_parser("put", help="insert a host file into the image")
    p_put.add_argument("image")
    p_put.add_argument("local_file")
    p_put.add_argument("filename", nargs="?", default="")
    p_put.add_argument("-f", "--force", action="store_true", help="overwrite if file exists")
    p_put.set_defaults(func=command_put)
    p_mkdir = sub.add_parser("mkdir", help="create a simulated directory path entry")
    p_mkdir.add_argument("image")
    p_mkdir.add_argument("directory")
    p_mkdir.set_defaults(func=command_mkdir)
    p_rm = sub.add_parser("rm", help="delete a file entry from the layout map")
    p_rm.add_argument("image")
    p_rm.add_argument("filename")
    p_rm.set_defaults(func=command_rm)
    p_format = sub.add_parser("format", help="format a fresh, blank TTFS storage image")
    p_format.add_argument("image")
    p_format.set_defaults(func=command_format)
    args = parser.parse_args()
    try:
        args.func(args)
    except TTFSException as exc:
        print(f"ttfs: error: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"ttfs: error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
