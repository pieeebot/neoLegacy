#!/usr/bin/env bash
# compare-4jlibs.sh - Compare 4JLibs between two git refs using Ghidra headless analysis.
#
# Extracts .lib files from both refs, runs Ghidra headless to export symbols/functions,
# then generates a structured diff report.
#
# Usage:
#   ./tools/ghidra/compare-4jlibs.sh [OLD_REF] [NEW_REF] [LIB_FILTER]
#
# Arguments:
#   OLD_REF    - Git ref for the old version (default: HEAD)
#   NEW_REF    - Git ref for the new version (default: upstream/main)
#   LIB_FILTER - Optional: only compare libs matching this pattern (e.g. "4J_Input")
#
# Environment:
#   GHIDRA_HOME - Path to Ghidra installation
#                 (default: C:/Users/revela/Documents/Minecraft/Libraries/ghidra_12.0.4_PUBLIC)
#
# Output:
#   tools/ghidra/output/report-<timestamp>/
#     old/         - Extracted old .lib files
#     new/         - Extracted new .lib files
#     analysis/    - Ghidra JSON exports (old_*.json, new_*.json)
#     diff/        - Per-library diff reports
#     summary.txt  - Overall summary of changes

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
GHIDRA_HOME="${GHIDRA_HOME:-C:/Users/revela/Documents/Minecraft/Libraries/ghidra_12.0.4_PUBLIC}"
HEADLESS="$GHIDRA_HOME/support/analyzeHeadless"

OLD_REF="${1:-HEAD}"
NEW_REF="${2:-upstream/main}"
LIB_FILTER="${3:-}"

TIMESTAMP=$(date +%Y%m%d-%H%M%S)
OUTPUT_DIR="$SCRIPT_DIR/output/report-$TIMESTAMP"
OLD_DIR="$OUTPUT_DIR/old"
NEW_DIR="$OUTPUT_DIR/new"
ANALYSIS_DIR="$OUTPUT_DIR/analysis"
DIFF_DIR="$OUTPUT_DIR/diff"
PROJECT_DIR="$OUTPUT_DIR/ghidra-projects"

mkdir -p "$OLD_DIR" "$NEW_DIR" "$ANALYSIS_DIR" "$DIFF_DIR" "$PROJECT_DIR"

LIB_PATH="Minecraft.Client/Windows64/4JLibs/libs"

echo "============================================"
echo "  4JLibs Comparison Tool (Ghidra Headless)"
echo "============================================"
echo ""
echo "  Old ref:    $OLD_REF"
echo "  New ref:    $NEW_REF"
echo "  Filter:     ${LIB_FILTER:-<all>}"
echo "  Output:     $OUTPUT_DIR"
echo "  Ghidra:     $GHIDRA_HOME"
echo ""

# -------------------------------------------------------
# Step 1: Extract .lib files from both git refs
# -------------------------------------------------------
echo "[1/4] Extracting .lib files from git..."

cd "$REPO_ROOT"

# Get list of .lib files that changed between the two refs
CHANGED_LIBS=$(git diff --name-only "$OLD_REF" "$NEW_REF" -- "$LIB_PATH/*.lib" 2>/dev/null || true)

if [ -z "$CHANGED_LIBS" ]; then
    echo "  No .lib file changes found between $OLD_REF and $NEW_REF"
    echo "  Falling back to listing all libs at $NEW_REF..."
    CHANGED_LIBS=$(git ls-tree --name-only -r "$NEW_REF" -- "$LIB_PATH/" | grep '\.lib$' || true)
fi

if [ -z "$CHANGED_LIBS" ]; then
    echo "ERROR: No .lib files found."
    exit 1
fi

echo "  Changed libraries:"
for lib in $CHANGED_LIBS; do
    basename "$lib"
    LIBNAME=$(basename "$lib" .lib)

    # Apply filter if specified
    if [ -n "$LIB_FILTER" ] && [[ "$LIBNAME" != *"$LIB_FILTER"* ]]; then
        continue
    fi

    # Extract old version (may not exist if newly added)
    if git cat-file -e "$OLD_REF:$lib" 2>/dev/null; then
        git show "$OLD_REF:$lib" > "$OLD_DIR/$LIBNAME.lib"
        echo "    old: extracted $LIBNAME.lib ($(wc -c < "$OLD_DIR/$LIBNAME.lib") bytes)"
    else
        echo "    old: $LIBNAME.lib does not exist at $OLD_REF"
    fi

    # Extract new version (may not exist if deleted)
    if git cat-file -e "$NEW_REF:$lib" 2>/dev/null; then
        git show "$NEW_REF:$lib" > "$NEW_DIR/$LIBNAME.lib"
        echo "    new: extracted $LIBNAME.lib ($(wc -c < "$NEW_DIR/$LIBNAME.lib") bytes)"
    else
        echo "    new: $LIBNAME.lib does not exist at $NEW_REF (deleted)"
    fi
done
echo ""

# -------------------------------------------------------
# Step 2: Run Ghidra headless analysis on each .lib
# -------------------------------------------------------
echo "[2/4] Running Ghidra headless analysis..."

analyze_lib() {
    local lib_file="$1"
    local label="$2"  # "old" or "new"
    local libname
    libname=$(basename "$lib_file" .lib)
    local out_json="$ANALYSIS_DIR/${label}_${libname}.json"
    local proj_dir="$PROJECT_DIR/${label}_${libname}"

    mkdir -p "$proj_dir"

    echo "  Analyzing ${label}/${libname}.lib ..."

    # Run Ghidra headless: import the .lib, analyze, run our export script, then delete the project
    "$HEADLESS" "$proj_dir" "proj" \
        -import "$lib_file" \
        -postScript ExportLibInfo.java "$out_json" \
        -scriptPath "$SCRIPT_DIR" \
        -deleteProject \
        -analysisTimeoutPerFile 300 \
        -max-cpu 4 \
        > "$ANALYSIS_DIR/${label}_${libname}_ghidra.log" 2>&1 || {
            echo "    WARNING: Ghidra analysis had issues for ${label}/${libname}. Check log."
        }

    if [ -f "$out_json" ]; then
        local func_count
        func_count=$(grep -c '"name"' "$out_json" 2>/dev/null || echo "0")
        echo "    Done: $out_json ($func_count entries)"
    else
        echo "    WARNING: No output generated for ${label}/${libname}"
    fi
}

# Analyze old libs
for lib_file in "$OLD_DIR"/*.lib; do
    [ -f "$lib_file" ] || continue
    analyze_lib "$lib_file" "old"
done

# Analyze new libs
for lib_file in "$NEW_DIR"/*.lib; do
    [ -f "$lib_file" ] || continue
    analyze_lib "$lib_file" "new"
done
echo ""

# -------------------------------------------------------
# Step 3: Generate diff reports
# -------------------------------------------------------
echo "[3/4] Generating diff reports..."

generate_diff() {
    local libname="$1"
    local old_json="$ANALYSIS_DIR/old_${libname}.json"
    local new_json="$ANALYSIS_DIR/new_${libname}.json"
    local diff_file="$DIFF_DIR/${libname}.diff.txt"

    echo "  Diffing $libname..."
    echo "=== $libname ===" > "$diff_file"
    echo "" >> "$diff_file"

    # Handle deleted libs
    if [ ! -f "$new_json" ]; then
        echo "STATUS: DELETED (library removed in new version)" >> "$diff_file"
        echo "" >> "$diff_file"
        if [ -f "$old_json" ]; then
            echo "--- Functions that were in old version ---" >> "$diff_file"
            grep '"name"' "$old_json" | head -200 >> "$diff_file"
        fi
        return
    fi

    # Handle newly added libs
    if [ ! -f "$old_json" ]; then
        echo "STATUS: ADDED (library is new in new version)" >> "$diff_file"
        echo "" >> "$diff_file"
        echo "--- Functions in new version ---" >> "$diff_file"
        grep '"name"' "$new_json" | head -200 >> "$diff_file"
        return
    fi

    # Both exist - compare
    echo "STATUS: MODIFIED" >> "$diff_file"
    echo "" >> "$diff_file"

    # Extract function names from each
    local old_funcs new_funcs
    old_funcs=$(mktemp)
    new_funcs=$(mktemp)

    grep -oP '"name"\s*:\s*"[^"]*"' "$old_json" | sort -u > "$old_funcs"
    grep -oP '"name"\s*:\s*"[^"]*"' "$new_json" | sort -u > "$new_funcs"

    local old_count new_count
    old_count=$(wc -l < "$old_funcs")
    new_count=$(wc -l < "$new_funcs")

    echo "Old function/symbol count: $old_count" >> "$diff_file"
    echo "New function/symbol count: $new_count" >> "$diff_file"
    echo "" >> "$diff_file"

    # Functions only in old (removed)
    local removed
    removed=$(comm -23 "$old_funcs" "$new_funcs")
    if [ -n "$removed" ]; then
        echo "--- REMOVED (in old, not in new) ---" >> "$diff_file"
        echo "$removed" >> "$diff_file"
        echo "" >> "$diff_file"
    fi

    # Functions only in new (added)
    local added
    added=$(comm -13 "$old_funcs" "$new_funcs")
    if [ -n "$added" ]; then
        echo "+++ ADDED (in new, not in old) +++" >> "$diff_file"
        echo "$added" >> "$diff_file"
        echo "" >> "$diff_file"
    fi

    # Functions in both (check for signature changes)
    local common
    common=$(comm -12 "$old_funcs" "$new_funcs")
    if [ -n "$common" ]; then
        local common_count
        common_count=$(echo "$common" | wc -l)
        echo "=== UNCHANGED names: $common_count ===" >> "$diff_file"
        echo "(Signature changes require detailed Ghidra comparison)" >> "$diff_file"
        echo "" >> "$diff_file"
    fi

    # Extract signatures for comparison
    local old_sigs new_sigs
    old_sigs=$(mktemp)
    new_sigs=$(mktemp)

    grep -oP '"signature"\s*:\s*"[^"]*"' "$old_json" | sort -u > "$old_sigs" 2>/dev/null || true
    grep -oP '"signature"\s*:\s*"[^"]*"' "$new_json" | sort -u > "$new_sigs" 2>/dev/null || true

    local sig_removed sig_added
    sig_removed=$(comm -23 "$old_sigs" "$new_sigs" 2>/dev/null || true)
    sig_added=$(comm -13 "$old_sigs" "$new_sigs" 2>/dev/null || true)

    if [ -n "$sig_removed" ] || [ -n "$sig_added" ]; then
        echo "--- SIGNATURE CHANGES ---" >> "$diff_file"
        if [ -n "$sig_removed" ]; then
            echo "  Old signatures no longer present:" >> "$diff_file"
            echo "$sig_removed" >> "$diff_file"
        fi
        if [ -n "$sig_added" ]; then
            echo "  New signatures:" >> "$diff_file"
            echo "$sig_added" >> "$diff_file"
        fi
        echo "" >> "$diff_file"
    fi

    # Size comparison
    if [ -f "$OLD_DIR/${libname}.lib" ] && [ -f "$NEW_DIR/${libname}.lib" ]; then
        local old_size new_size
        old_size=$(wc -c < "$OLD_DIR/${libname}.lib")
        new_size=$(wc -c < "$NEW_DIR/${libname}.lib")
        local delta=$((new_size - old_size))
        local pct=0
        if [ "$old_size" -gt 0 ]; then
            pct=$(( (delta * 100) / old_size ))
        fi
        echo "SIZE: $old_size -> $new_size bytes (${delta:+$delta} bytes, ${pct}%)" >> "$diff_file"
    fi

    rm -f "$old_funcs" "$new_funcs" "$old_sigs" "$new_sigs"
}

# Get unique lib names across old and new
ALL_LIBS=$(cd "$OUTPUT_DIR" && (ls old/*.lib new/*.lib 2>/dev/null || true) | xargs -I{} basename {} .lib | sort -u)

for libname in $ALL_LIBS; do
    generate_diff "$libname"
done
echo ""

# -------------------------------------------------------
# Step 4: Generate summary
# -------------------------------------------------------
echo "[4/4] Generating summary..."

SUMMARY="$OUTPUT_DIR/summary.txt"
{
    echo "============================================"
    echo "  4JLibs Comparison Report"
    echo "============================================"
    echo ""
    echo "  Old ref:    $OLD_REF"
    echo "  New ref:    $NEW_REF"
    echo "  Generated:  $(date)"
    echo ""
    echo "--------------------------------------------"
    echo "  Library Status"
    echo "--------------------------------------------"

    for libname in $ALL_LIBS; do
        local_diff="$DIFF_DIR/${libname}.diff.txt"
        if [ -f "$local_diff" ]; then
            status=$(grep "^STATUS:" "$local_diff" | head -1 | cut -d: -f2 | xargs)
            size_line=$(grep "^SIZE:" "$local_diff" | head -1 || echo "")
            echo ""
            echo "  $libname: $status"
            if [ -n "$size_line" ]; then
                echo "    $size_line"
            fi

            # Count added/removed
            added_count=$(grep -c '^\+\+\+' "$local_diff" 2>/dev/null || echo "0")
            removed_count=$(grep -c '^---' "$local_diff" 2>/dev/null || echo "0")
            if [ "$added_count" -gt 0 ] || [ "$removed_count" -gt 0 ]; then
                echo "    Sections: +$added_count added, -$removed_count removed"
            fi
        fi
    done

    echo ""
    echo "--------------------------------------------"
    echo "  Detailed reports in: $DIFF_DIR/"
    echo "  Raw Ghidra JSON in:  $ANALYSIS_DIR/"
    echo "  Ghidra logs in:      $ANALYSIS_DIR/*_ghidra.log"
    echo "--------------------------------------------"
} > "$SUMMARY"

cat "$SUMMARY"

echo ""
echo "Done. Full report: $OUTPUT_DIR"
