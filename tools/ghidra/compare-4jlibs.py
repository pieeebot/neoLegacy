"""Compare 4JLibs between two git refs.

Extracts .lib files from both refs, parses their symbol tables (using dumpbin
or direct ar-archive parsing), demangles MSVC symbols, and generates a
structured diff report.

Usage:
    python compare-4jlibs.py [OLD_REF] [NEW_REF] [--filter PATTERN] [--no-demangle]

Defaults:
    OLD_REF  = HEAD
    NEW_REF  = upstream/main

Output:
    tools/ghidra/output/report-<timestamp>/
"""

import argparse
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from collections import defaultdict
from datetime import datetime
from pathlib import Path


# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
LIB_PATH = "Minecraft.Client/Windows64/4JLibs/libs"
OUTPUT_BASE = Path(__file__).resolve().parent / "output"

# MSVC tool discovery
MSVC_SEARCH_PATHS = [
    Path(r"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC"),
    Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"),
    Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC"),
    Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC"),
]


def find_msvc_tool(name):
    """Find an MSVC tool (dumpbin.exe, undname.exe) in VS installations."""
    for base in MSVC_SEARCH_PATHS:
        if not base.exists():
            continue
        for version_dir in sorted(base.iterdir(), reverse=True):
            tool = version_dir / "bin" / "Hostx64" / "x64" / name
            if tool.exists():
                return str(tool)
    return None


DUMPBIN = find_msvc_tool("dumpbin.exe")
UNDNAME = find_msvc_tool("undname.exe")


# ---------------------------------------------------------------------------
# Symbol table parsing (direct, no external tools needed)
# ---------------------------------------------------------------------------

def parse_lib_symbols_direct(lib_path):
    """Parse the first linker member of a .lib to get all public symbols."""
    symbols = []
    with open(lib_path, "rb") as f:
        magic = f.read(8)
        if magic != b"!<arch>\n":
            return symbols

        # First linker member (big-endian)
        header = f.read(60)
        name = header[0:16].decode("ascii").strip()
        size = int(header[48:58].decode("ascii").strip())

        if name != "/":
            return symbols

        data = f.read(size)
        num_symbols = struct.unpack(">I", data[0:4])[0]
        offsets_end = 4 + num_symbols * 4
        string_data = data[offsets_end:]

        pos = 0
        for _ in range(num_symbols):
            end = string_data.find(b"\x00", pos)
            if end == -1:
                break
            sym = string_data[pos:end].decode("ascii", errors="replace")
            symbols.append(sym)
            pos = end + 1

    return symbols


def parse_lib_symbols_dumpbin(lib_path):
    """Use dumpbin /LINKERMEMBER to get symbols (more reliable for edge cases)."""
    if not DUMPBIN:
        return None

    try:
        result = subprocess.run(
            [DUMPBIN, "/LINKERMEMBER:2", str(lib_path)],
            capture_output=True, text=True, timeout=60
        )
        symbols = []
        in_symbols = False
        for line in result.stdout.splitlines():
            line = line.strip()
            if "public symbols" in line:
                in_symbols = True
                continue
            if in_symbols and line:
                # Format: "   offset symbol_name"
                parts = line.split(None, 1)
                if len(parts) == 2 and all(c in "0123456789ABCDEFabcdef" for c in parts[0]):
                    symbols.append(parts[1])
                elif not line[0].isdigit():
                    in_symbols = False
        return symbols
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return None


def parse_lib_members(lib_path):
    """Extract member (object file) names from a .lib archive."""
    members = []
    with open(lib_path, "rb") as f:
        magic = f.read(8)
        if magic != b"!<arch>\n":
            return members

        long_names = b""

        while True:
            header = f.read(60)
            if len(header) < 60:
                break

            raw_name = header[0:16].decode("ascii", errors="replace").rstrip()
            size_str = header[48:58].decode("ascii").strip()
            end_marker = header[58:60]

            if end_marker != b"\x60\x0a":
                break

            size = int(size_str)

            if raw_name == "/":
                f.seek(size + (size % 2), 1)
                continue
            if raw_name == "//":
                long_names = f.read(size)
                if size % 2:
                    f.read(1)
                continue

            name = raw_name
            if name.startswith("/") and name[1:].isdigit():
                offset = int(name[1:])
                end = long_names.find(b"\x00", offset)
                if end == -1:
                    end = long_names.find(b"\n", offset)
                if end == -1:
                    end = len(long_names)
                name = long_names[offset:end].decode("ascii", errors="replace").rstrip("/")

            # Read first 2 bytes to check machine type
            member_data = f.read(min(size, 2))
            if len(member_data) >= 2:
                machine = struct.unpack("<H", member_data[:2])[0]
            else:
                machine = 0
            remaining = size - len(member_data)
            if remaining > 0:
                f.seek(remaining, 1)
            if size % 2:
                f.read(1)

            members.append({
                "name": name,
                "size": size,
                "machine": f"0x{machine:04x}",
                "is_ltcg": machine == 0x01f2,
            })

    return members


# ---------------------------------------------------------------------------
# Demangling
# ---------------------------------------------------------------------------

def demangle_symbols(mangled_symbols):
    """Demangle MSVC-mangled symbols using undname.exe."""
    if not UNDNAME or not mangled_symbols:
        return {}

    demangled = {}

    # undname takes symbols as command-line arguments (not stdin).
    # Output format:
    #   Undecoration of :- "??0CProfile@@QAA@XZ"
    #   is :- "public: __cdecl CProfile::CProfile(void)"
    # Process in batches to avoid command line length limits.
    batch_size = 100
    for i in range(0, len(mangled_symbols), batch_size):
        batch = mangled_symbols[i:i + batch_size]
        try:
            result = subprocess.run(
                [UNDNAME] + batch,
                capture_output=True, text=True, timeout=60
            )
            current_mangled = None
            for line in result.stdout.splitlines():
                line = line.strip()
                if line.startswith('Undecoration of :- "'):
                    current_mangled = line.split('"')[1]
                elif line.startswith('is :- "') and current_mangled:
                    dem = line.split('"')[1]
                    demangled[current_mangled] = dem
                    current_mangled = None
        except (subprocess.TimeoutExpired, FileNotFoundError):
            break

    return demangled


# ---------------------------------------------------------------------------
# Classification
# ---------------------------------------------------------------------------

def classify_symbol(mangled, demangled=None):
    """Classify a symbol into a category for organized reporting."""
    name = demangled or mangled

    # Filter out std:: library symbols
    if "std::" in name or mangled.startswith("??_C@"):
        return "std/compiler"

    # Constructor/destructor
    if mangled.startswith("??0"):
        return "constructor"
    if mangled.startswith("??1"):
        return "destructor"

    # Operators
    if mangled.startswith("??"):
        return "operator"

    # Virtual function table
    if mangled.startswith("??_7") or "vftable" in name.lower():
        return "vtable"

    # Static data
    if mangled.startswith("?_") and "@" in mangled:
        return "static_data"

    # Check class membership
    if "@C_4J" in mangled or "@C_4j" in mangled:
        return "4j_interface"

    for prefix in ["CAwardManager", "CProfile", "CProfileData", "CRichPresence",
                    "CSys", "CStorage", "CInput", "CRender", "CRenderer"]:
        if f"@{prefix}@@" in mangled or f"@{prefix}@" in mangled:
            return "4j_class"

    return "other"


def extract_class_name(mangled):
    """Try to extract the class name from a mangled symbol."""
    # Pattern: ?Method@ClassName@@...
    m = re.match(r"\?\??\d?(\w+)@(\w+)@@", mangled)
    if m:
        return m.group(2)

    m = re.match(r"\?(\w+)@(\w+)@@", mangled)
    if m:
        return m.group(2)

    return None


# ---------------------------------------------------------------------------
# Git operations
# ---------------------------------------------------------------------------

def git_extract_lib(ref, lib_rel_path, output_path):
    """Extract a file from a git ref to a local path."""
    try:
        result = subprocess.run(
            ["git", "cat-file", "-e", f"{ref}:{lib_rel_path}"],
            capture_output=True, cwd=str(REPO_ROOT)
        )
        if result.returncode != 0:
            return False

        result = subprocess.run(
            ["git", "show", f"{ref}:{lib_rel_path}"],
            capture_output=True, cwd=str(REPO_ROOT)
        )
        if result.returncode != 0:
            return False

        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, "wb") as f:
            f.write(result.stdout)
        return True
    except Exception as e:
        print(f"  WARNING: Failed to extract {lib_rel_path} from {ref}: {e}", file=sys.stderr)
        return False


def git_changed_libs(old_ref, new_ref):
    """Get list of .lib files that changed between two refs."""
    result = subprocess.run(
        ["git", "diff", "--name-only", old_ref, new_ref, "--", f"{LIB_PATH}/*.lib"],
        capture_output=True, text=True, cwd=str(REPO_ROOT)
    )
    if result.returncode != 0 or not result.stdout.strip():
        # Fallback: list all libs at new ref
        result = subprocess.run(
            ["git", "ls-tree", "--name-only", "-r", new_ref, "--", f"{LIB_PATH}/"],
            capture_output=True, text=True, cwd=str(REPO_ROOT)
        )
    return [l for l in result.stdout.strip().splitlines() if l.endswith(".lib")]


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def generate_lib_report(lib_name, old_syms, new_syms, old_demangled, new_demangled,
                        old_members, new_members, old_size, new_size):
    """Generate a detailed comparison report for one library."""
    lines = []
    lines.append(f"{'=' * 70}")
    lines.append(f"  {lib_name}")
    lines.append(f"{'=' * 70}")
    lines.append("")

    # Status
    if old_syms is None and new_syms is not None:
        lines.append("STATUS: ADDED (new library)")
    elif old_syms is not None and new_syms is None:
        lines.append("STATUS: DELETED")
    else:
        lines.append("STATUS: MODIFIED")
    lines.append("")

    # Size
    if old_size and new_size:
        delta = new_size - old_size
        pct = (delta * 100) // old_size if old_size else 0
        sign = "+" if delta > 0 else ""
        lines.append(f"SIZE: {old_size:,} -> {new_size:,} bytes ({sign}{delta:,}, {sign}{pct}%)")
    elif new_size:
        lines.append(f"SIZE: (new) {new_size:,} bytes")
    elif old_size:
        lines.append(f"SIZE: {old_size:,} bytes (deleted)")
    lines.append("")

    # Members
    if old_members or new_members:
        old_member_names = {m["name"] for m in (old_members or [])}
        new_member_names = {m["name"] for m in (new_members or [])}
        lines.append(f"OBJECT FILES: {len(old_member_names)} -> {len(new_member_names)}")
        added_m = new_member_names - old_member_names
        removed_m = old_member_names - new_member_names
        if added_m:
            lines.append(f"  + Added: {', '.join(sorted(added_m))}")
        if removed_m:
            lines.append(f"  - Removed: {', '.join(sorted(removed_m))}")
        lines.append("")

    old_set = set(old_syms or [])
    new_set = set(new_syms or [])

    # Filter out std/compiler symbols for the main diff
    old_user = {s for s in old_set if classify_symbol(s) not in ("std/compiler",)}
    new_user = {s for s in new_set if classify_symbol(s) not in ("std/compiler",)}

    old_std = old_set - old_user
    new_std = new_set - new_user

    lines.append(f"SYMBOLS: {len(old_set)} -> {len(new_set)} total")
    lines.append(f"  User symbols: {len(old_user)} -> {len(new_user)}")
    lines.append(f"  Std/compiler: {len(old_std)} -> {len(new_std)}")
    lines.append("")

    # Added symbols (grouped by class)
    added = sorted(new_user - old_user)
    removed = sorted(old_user - new_user)
    unchanged = old_user & new_user

    if added:
        lines.append(f"+++ ADDED SYMBOLS ({len(added)}) +++")
        by_class = defaultdict(list)
        for s in added:
            cls = extract_class_name(s) or "(global)"
            d = new_demangled.get(s, s)
            by_class[cls].append(d)
        for cls in sorted(by_class.keys()):
            lines.append(f"  [{cls}]")
            for d in sorted(by_class[cls]):
                lines.append(f"    + {d}")
        lines.append("")

    if removed:
        lines.append(f"--- REMOVED SYMBOLS ({len(removed)}) ---")
        by_class = defaultdict(list)
        for s in removed:
            cls = extract_class_name(s) or "(global)"
            d = old_demangled.get(s, s)
            by_class[cls].append(d)
        for cls in sorted(by_class.keys()):
            lines.append(f"  [{cls}]")
            for d in sorted(by_class[cls]):
                lines.append(f"    - {d}")
        lines.append("")

    lines.append(f"UNCHANGED: {len(unchanged)} symbols")
    lines.append("")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Compare 4JLibs between git refs")
    parser.add_argument("old_ref", nargs="?", default="HEAD", help="Old git ref (default: HEAD)")
    parser.add_argument("new_ref", nargs="?", default="upstream/main", help="New git ref (default: upstream/main)")
    parser.add_argument("--filter", "-f", default="", help="Only compare libs matching this pattern")
    parser.add_argument("--no-demangle", action="store_true", help="Skip demangling")
    parser.add_argument("--json", action="store_true", help="Also output JSON data")
    args = parser.parse_args()

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    report_dir = OUTPUT_BASE / f"report-{timestamp}"
    report_dir.mkdir(parents=True, exist_ok=True)

    print("=" * 56)
    print("  4JLibs Comparison Tool")
    print("=" * 56)
    print(f"  Old ref:    {args.old_ref}")
    print(f"  New ref:    {args.new_ref}")
    print(f"  Filter:     {args.filter or '<all>'}")
    print(f"  Demangle:   {not args.no_demangle}")
    print(f"  dumpbin:    {'found' if DUMPBIN else 'not found (using direct parsing)'}")
    print(f"  undname:    {'found' if UNDNAME else 'not found (no demangling)'}")
    print(f"  Output:     {report_dir}")
    print()

    # Step 1: Find changed libs
    print("[1/4] Finding changed libraries...")
    changed_libs = git_changed_libs(args.old_ref, args.new_ref)
    if args.filter:
        changed_libs = [l for l in changed_libs if args.filter in os.path.basename(l)]

    if not changed_libs:
        print("  No matching .lib changes found.")
        return

    for lib in changed_libs:
        print(f"  {os.path.basename(lib)}")
    print()

    # Step 2: Extract libs from git
    print("[2/4] Extracting libraries from git...")
    old_dir = report_dir / "old"
    new_dir = report_dir / "new"

    lib_pairs = {}  # name -> (old_path, new_path)
    for lib_rel in changed_libs:
        name = os.path.basename(lib_rel).replace(".lib", "")
        old_path = old_dir / f"{name}.lib"
        new_path = new_dir / f"{name}.lib"

        old_ok = git_extract_lib(args.old_ref, lib_rel, str(old_path))
        new_ok = git_extract_lib(args.new_ref, lib_rel, str(new_path))

        old_size = old_path.stat().st_size if old_ok else None
        new_size = new_path.stat().st_size if new_ok else None

        print(f"  {name}: old={'found' if old_ok else 'N/A'} new={'found' if new_ok else 'N/A'}")
        lib_pairs[name] = (
            str(old_path) if old_ok else None,
            str(new_path) if new_ok else None,
            old_size, new_size
        )
    print()

    # Step 3: Parse symbols and generate diffs
    print("[3/4] Parsing symbols...")
    all_reports = []
    json_data = {}

    all_mangled_to_demangle = set()

    for name, (old_path, new_path, old_size, new_size) in sorted(lib_pairs.items()):
        print(f"  Parsing {name}...")

        old_syms = None
        new_syms = None
        old_members = None
        new_members = None

        if old_path:
            old_syms = parse_lib_symbols_dumpbin(old_path) or parse_lib_symbols_direct(old_path)
            old_members = parse_lib_members(old_path)
            print(f"    Old: {len(old_syms)} symbols, {len(old_members)} objects")

        if new_path:
            new_syms = parse_lib_symbols_dumpbin(new_path) or parse_lib_symbols_direct(new_path)
            new_members = parse_lib_members(new_path)
            print(f"    New: {len(new_syms)} symbols, {len(new_members)} objects")

        # Collect symbols needing demangling
        if not args.no_demangle:
            if old_syms:
                all_mangled_to_demangle.update(old_syms)
            if new_syms:
                all_mangled_to_demangle.update(new_syms)

        lib_pairs[name] = (old_path, new_path, old_size, new_size,
                           old_syms, new_syms, old_members, new_members)
    print()

    # Step 3b: Batch demangle
    old_demangled = {}
    new_demangled = {}
    if not args.no_demangle and all_mangled_to_demangle:
        print(f"  Demangling {len(all_mangled_to_demangle)} unique symbols...")
        all_demangled = demangle_symbols(sorted(all_mangled_to_demangle))
        print(f"  Demangled {len(all_demangled)} symbols")
        old_demangled = all_demangled
        new_demangled = all_demangled
    print()

    # Step 4: Generate reports
    print("[4/4] Generating reports...")

    for name in sorted(lib_pairs.keys()):
        entry = lib_pairs[name]
        old_path, new_path, old_size, new_size = entry[0], entry[1], entry[2], entry[3]
        old_syms, new_syms, old_members, new_members = entry[4], entry[5], entry[6], entry[7]

        report = generate_lib_report(
            name, old_syms, new_syms, old_demangled, new_demangled,
            old_members, new_members, old_size, new_size
        )
        all_reports.append(report)

        # Write individual report
        diff_dir = report_dir / "diff"
        diff_dir.mkdir(exist_ok=True)
        (diff_dir / f"{name}.txt").write_text(report, encoding="utf-8")

        if args.json:
            json_data[name] = {
                "old_size": old_size,
                "new_size": new_size,
                "old_symbol_count": len(old_syms) if old_syms else 0,
                "new_symbol_count": len(new_syms) if new_syms else 0,
                "added": sorted(set(new_syms or []) - set(old_syms or [])),
                "removed": sorted(set(old_syms or []) - set(new_syms or [])),
                "old_members": old_members,
                "new_members": new_members,
            }

    # Write combined report
    summary = []
    summary.append("=" * 70)
    summary.append("  4JLibs Comparison Report")
    summary.append("=" * 70)
    summary.append(f"  Old ref:    {args.old_ref}")
    summary.append(f"  New ref:    {args.new_ref}")
    summary.append(f"  Generated:  {datetime.now().isoformat()}")
    summary.append("")
    summary.append("-" * 70)
    summary.append("  Quick Summary")
    summary.append("-" * 70)

    for name in sorted(lib_pairs.keys()):
        entry = lib_pairs[name]
        old_syms, new_syms = entry[4], entry[5]
        old_set = set(old_syms or [])
        new_set = set(new_syms or [])
        added = len(new_set - old_set)
        removed = len(old_set - new_set)

        if old_syms is None:
            status = "ADDED"
        elif new_syms is None:
            status = "DELETED"
        else:
            status = "MODIFIED"

        summary.append(f"  {name:30s} {status:10s} +{added} -{removed} symbols")

    summary.append("")
    summary.append("=" * 70)
    summary.append("")

    full_report = "\n".join(summary) + "\n\n" + "\n\n".join(all_reports)
    summary_path = report_dir / "summary.txt"
    summary_path.write_text(full_report, encoding="utf-8")

    if args.json:
        json_path = report_dir / "data.json"
        json_path.write_text(json.dumps(json_data, indent=2), encoding="utf-8")

    print()
    print("\n".join(summary))
    print()
    print(f"Full report:  {summary_path}")
    print(f"Per-lib diffs: {report_dir / 'diff'}")
    if args.json:
        print(f"JSON data:    {report_dir / 'data.json'}")


if __name__ == "__main__":
    main()
