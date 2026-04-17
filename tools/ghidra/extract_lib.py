"""Extract .obj members from a COFF .lib (ar archive) into a directory.

Usage:
    python extract_lib.py <input.lib> <output_dir>

Each member is written as <output_dir>/<name>.obj. The first/second linker
members and the long-name string table are skipped.
"""

import os
import sys


def extract_lib(lib_path, out_dir):
    os.makedirs(out_dir, exist_ok=True)

    with open(lib_path, "rb") as f:
        magic = f.read(8)
        if magic != b"!<arch>\n":
            print(f"ERROR: Not an ar archive: {lib_path}", file=sys.stderr)
            sys.exit(1)

        # Read the long-name string table if present
        long_names = b""
        members = []

        while True:
            pos = f.tell()
            header = f.read(60)
            if len(header) < 60:
                break

            raw_name = header[0:16]
            size_str = header[48:58].decode("ascii").strip()
            end_marker = header[58:60]

            if end_marker != b"\x60\x0a":
                print(f"WARNING: Bad end marker at offset {pos}, stopping.", file=sys.stderr)
                break

            size = int(size_str)
            data = f.read(size)

            # Pad to even boundary
            if size % 2 != 0:
                f.read(1)

            name = raw_name.decode("ascii", errors="replace").rstrip()

            # Skip first and second linker members (both named "/")
            if name == "/":
                continue

            # Long-name string table
            if name == "//":
                long_names = data
                continue

            # Resolve long name references like "/26"
            if name.startswith("/") and name[1:].isdigit():
                offset = int(name[1:])
                end = long_names.find(b"\x00", offset)
                if end == -1:
                    # Try newline-terminated (common in MSVC libs)
                    end = long_names.find(b"\n", offset)
                if end == -1:
                    end = len(long_names)
                resolved = long_names[offset:end].decode("ascii", errors="replace").rstrip("/")
                name = resolved

            # Clean the name for filesystem use
            safe_name = name.replace("/", "_").replace("\\", "_").replace("..", "_")
            if not safe_name.endswith(".obj"):
                safe_name += ".obj"

            members.append((safe_name, data))

        # Write members
        written = 0
        for safe_name, data in members:
            out_path = os.path.join(out_dir, safe_name)

            # Handle duplicate names by appending a counter
            if os.path.exists(out_path):
                base, ext = os.path.splitext(safe_name)
                counter = 2
                while os.path.exists(os.path.join(out_dir, f"{base}_{counter}{ext}")):
                    counter += 1
                out_path = os.path.join(out_dir, f"{base}_{counter}{ext}")

            with open(out_path, "wb") as out_f:
                out_f.write(data)
            written += 1

        print(f"Extracted {written} object files from {os.path.basename(lib_path)} -> {out_dir}")
        return written


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.lib> <output_dir>", file=sys.stderr)
        sys.exit(1)

    extract_lib(sys.argv[1], sys.argv[2])
