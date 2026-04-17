#!/usr/bin/env bash
# list-lib-symbols.sh - Quick symbol listing for a single .lib file using Ghidra headless.
#
# Usage:
#   ./tools/ghidra/list-lib-symbols.sh <path-to-lib-file> [output.json]
#
# If no output path is given, writes to tools/ghidra/output/<libname>.json

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GHIDRA_HOME="${GHIDRA_HOME:-C:/Users/revela/Documents/Minecraft/Libraries/ghidra_12.0.4_PUBLIC}"
HEADLESS="$GHIDRA_HOME/support/analyzeHeadless"

LIB_FILE="${1:?Usage: list-lib-symbols.sh <path-to-lib-file> [output.json]}"
LIBNAME=$(basename "$LIB_FILE" .lib)

OUTPUT="${2:-$SCRIPT_DIR/output/${LIBNAME}.json}"
mkdir -p "$(dirname "$OUTPUT")"

PROJ_DIR=$(mktemp -d)

echo "Analyzing $LIB_FILE ..."
echo "  Output: $OUTPUT"

"$HEADLESS" "$PROJ_DIR" "proj" \
    -import "$LIB_FILE" \
    -postScript ExportLibInfo.java "$OUTPUT" \
    -scriptPath "$SCRIPT_DIR" \
    -deleteProject \
    -analysisTimeoutPerFile 300 \
    -max-cpu 4 \
    2>&1 | tail -5

rm -rf "$PROJ_DIR"

if [ -f "$OUTPUT" ]; then
    func_count=$(grep -c '"signature"' "$OUTPUT" 2>/dev/null || echo "0")
    echo ""
    echo "Done. $func_count function entries exported to $OUTPUT"
else
    echo "ERROR: No output was generated."
    exit 1
fi
