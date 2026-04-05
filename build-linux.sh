#!/usr/bin/env bash
set -euo pipefail
VERSION="0.0.0" # man we're using nightly :sob:
SOURCE_DIR="${1:-.}"
BUILD_CI="${BUILD_CI:-0}"
BUILD_TYPE="${2:-Release}"
XWIN_CACHE="${XWIN_CACHE:-$PWD/.xwin}"
INSTALL_DIR="${INSTALL_PREFIX:-$HOME/.local/share/evolved-lce}"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'
info()    { echo -e "${CYAN}[info]${NC} $*"; }
success() { echo -e "${GREEN}[ok]${NC} $*"; }
warn()    { echo -e "${YELLOW}[warn]${NC} $*"; }
die()     { echo -e "${RED}[error]${NC} $*" >&2; exit 1; }
check_deps() {
    local missing=()
    local deps=(clang-cl lld-link llvm-rc llvm-ml llvm-lib llvm-mt cmake ninja xwin rsync)
    for dep in "${deps[@]}"; do
        command -v "$dep" &>/dev/null || missing+=("$dep")
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        die "Missing dependencies: ${missing[*]}\n\nInstall guide:\n  clang-cl/lld/llvm  -> clang + lld (your distro's llvm package)\n  cmake/ninja        -> cmake + ninja-build\n  xwin               -> cargo install xwin\n  rsync              -> rsync"
    fi

    success "All dependencies found"
}

fetch_winsdk() {
    local splat_dir="$XWIN_CACHE/splat"
    if [[ -d "$splat_dir" ]]; then
        info "Using cached Windows SDK at $splat_dir"
        return
    fi

    info "Downloading Windows SDK and CRT via xwin..."
    mkdir -p "$XWIN_CACHE"
    xwin --accept-license splat --output "$splat_dir"
    success "Windows SDK downloaded"
}

patch_winsdk_symlinks() {
    local splat_dir="$XWIN_CACHE/splat"
    info "Patching case-sensitivity symlinks..."
    ln -sf "$splat_dir/sdk/include/shared/sdkddkver.h" \
           "$splat_dir/sdk/include/shared/SDKDDKVer.h"  2>/dev/null || true
    ln -sf "$splat_dir/sdk/lib/um/x86_64/xinput9_1_0.lib" \
           "$splat_dir/sdk/lib/um/x86_64/XInput9_1_0.lib" 2>/dev/null || true
    ln -sf "$splat_dir/sdk/lib/um/x86_64/ws2_32.lib" \
           "$splat_dir/sdk/lib/um/x86_64/Ws2_32.lib" 2>/dev/null || true
}

write_toolchain() {
    local toolchain_file="$BUILD_DIR/clang-cl-toolchain.cmake"
    cat > "$toolchain_file" <<'CMAKE'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_ASM_MASM_COMPILER llvm-ml)
set(CMAKE_AR llvm-lib)
set(CMAKE_LINKER lld-link)
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -out:<TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -out:<TARGET> <LINK_LIBRARIES>")
add_compile_options(-fms-compatibility -fms-extensions)
add_compile_definitions(_WIN64 _AMD64_ WIN32_LEAN_AND_MEAN)
CMAKE

    echo "$toolchain_file"
}

do_cmake_configure() {
    local winsdk="$XWIN_CACHE/splat"
    local toolchain
    toolchain="$(write_toolchain)"
    local c_flags="/MT -Wno-non-pod-varargs -fms-compatibility -fms-extensions --target=x86_64-pc-windows-msvc \
-imsvc $winsdk/crt/include \
-imsvc $winsdk/sdk/include/ucrt \
-imsvc $winsdk/sdk/include/um \
-imsvc $winsdk/sdk/include/shared"

    local linker_flags="\
-libpath:$winsdk/crt/lib/x86_64 \
-libpath:$winsdk/sdk/lib/um/x86_64 \
-libpath:$winsdk/sdk/lib/ucrt/x86_64"

    info "Configuring with CMake ($BUILD_TYPE)..."
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DCMAKE_C_COMPILER=clang-cl \
        -DCMAKE_CXX_COMPILER=clang-cl \
        -DCMAKE_LINKER=lld-link \
        -DCMAKE_RC_COMPILER=llvm-rc \
        -DCMAKE_MT=llvm-mt \
        -DPLATFORM_DEFINES="_WINDOWS64" \
        -DPLATFORM_NAME="Windows64" \
        -DIGGY_LIBS="iggy_w64.lib;iggyperfmon_w64.lib;iggyexpruntime_w64.lib" \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded \
        -DCMAKE_C_FLAGS="$c_flags" \
        -DCMAKE_CXX_FLAGS="$c_flags" \
        -DCMAKE_ASM_MASM_FLAGS="-m64" \
        -DCMAKE_EXE_LINKER_FLAGS="$linker_flags" \
        -DCMAKE_RC_FLAGS="/I $winsdk/sdk/include/shared /I $winsdk/sdk/include/um /I $winsdk/sdk/include/ucrt"

    success "CMake configuration done"
}

do_build() {
    cores="$(nproc)"
    if [[ "${BUILD_CI:-0}" == "1" ]]; then
        cores=3
    fi
    info "Building with ${cores} cores..."
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j "${cores}"
    success "Build complete"
}

do_install() {
    info "Installing to $INSTALL_DIR..."
    mkdir -p "$INSTALL_DIR"/{client,server}
    cp "$BUILD_DIR/Minecraft.Client/Minecraft.Client.exe" "$INSTALL_DIR/client/"
    cp "$BUILD_DIR/Minecraft.Server/Minecraft.Server.exe" "$INSTALL_DIR/server/"
    for asset in iggy_w64.dll Common music Windows64 Windows64Media; do
        [[ -e "$BUILD_DIR/Minecraft.Client/$asset" ]] && \
            cp -r "$BUILD_DIR/Minecraft.Client/$asset" "$INSTALL_DIR/client/" || true
    done
    for asset in iggy_w64.dll Common Windows64; do
        [[ -e "$BUILD_DIR/Minecraft.Server/$asset" ]] && \
            cp -r "$BUILD_DIR/Minecraft.Server/$asset" "$INSTALL_DIR/server/" || true
    done
    write_client_launcher
    write_server_launcher
    success "Installed to $INSTALL_DIR"
    info "Run the client: $INSTALL_DIR/minecraft-lce-client"
    info "Run the server: $INSTALL_DIR/minecraft-lce-server"
}

write_client_launcher() {
    cat > "$INSTALL_DIR/minecraft-lce-client" <<LAUNCHER
#!/usr/bin/env bash
set -euo pipefail
GAME_DIR="$INSTALL_DIR/client"
PERSIST_DIR="\${MC_DATA_DIR:-\$HOME/.local/share/minecraft-lce-client}"
export WINEARCH=win64
export WINEPREFIX="\${WINEPREFIX:-\$HOME/.local/share/minecraft-lce-client-prefix}"
export WINEDLLOVERRIDES="winemenubuilder.exe=d"
export WINEESYNC=1
export WINEFSYNC=1
export DXVK_LOG_LEVEL=none
mkdir -p "\$PERSIST_DIR" "\$WINEPREFIX"
WORK_DIR="\$(mktemp -d)"
trap 'rm -rf "\$WORK_DIR"' EXIT
cp -rs "\$GAME_DIR"/* "\$WORK_DIR/"
chmod -R u+w "\$WORK_DIR"
mkdir -p "\$PERSIST_DIR/GameHDD"
rm -rf "\$WORK_DIR/Windows64/GameHDD" 2>/dev/null || true
ln -sf "\$PERSIST_DIR/GameHDD" "\$WORK_DIR/Windows64/GameHDD"
cd "\$WORK_DIR"
exec wine "\$WORK_DIR/Minecraft.Client.exe" "\$@"
LAUNCHER
    chmod +x "$INSTALL_DIR/minecraft-lce-client"
}

write_server_launcher() {
    cat > "$INSTALL_DIR/minecraft-lce-server" <<LAUNCHER
#!/usr/bin/env bash
set -euo pipefail
GAME_DIR="$INSTALL_DIR/server"
SERVER_PORT="\${MC_PORT:-25565}"
SERVER_BIND_IP="\${MC_BIND:-0.0.0.0}"
PERSIST_DIR="\${MC_DATA_DIR:-\$HOME/.local/share/minecraft-lce-server}"
export WINEARCH=win64
export WINEPREFIX="\${WINEPREFIX:-\$HOME/.local/share/minecraft-lce-server-prefix}"
export WINEDLLOVERRIDES="winemenubuilder.exe=d"
export WINEESYNC=1
export WINEFSYNC=1
mkdir -p "\$PERSIST_DIR" "\$WINEPREFIX"
WORK_DIR="\$(mktemp -d)"
trap 'rm -rf "\$WORK_DIR"' EXIT
cp -rs "\$GAME_DIR"/* "\$WORK_DIR/"
chmod -R u+w "\$WORK_DIR"
mkdir -p "\$PERSIST_DIR/GameHDD"
for file in server.properties banned-players.json banned-ips.json; do
    if [[ ! -f "\$PERSIST_DIR/\$file" ]]; then
        [[ -f "\$WORK_DIR/\$file" ]] && cp "\$WORK_DIR/\$file" "\$PERSIST_DIR/\$file" || echo "[]" > "\$PERSIST_DIR/\$file"
    fi
    ln -sf "\$PERSIST_DIR/\$file" "\$WORK_DIR/\$file"
done
rm -rf "\$WORK_DIR/Windows64/GameHDD" 2>/dev/null || true
ln -sf "\$PERSIST_DIR/GameHDD" "\$WORK_DIR/Windows64/GameHDD"
cd "\$WORK_DIR"
if [[ -z "\${DISPLAY:-}" ]]; then
    export DISPLAY=":99"
    Xvfb "\$DISPLAY" -nolisten tcp -screen 0 64x64x16 &
    XVFB_PID=\$!
    trap 'kill \$XVFB_PID 2>/dev/null || true; rm -rf "\$WORK_DIR"' EXIT
    sleep 1
fi
exec wine "\$WORK_DIR/Minecraft.Server.exe" -port "\$SERVER_PORT" -bind "\$SERVER_BIND_IP" "\$@"
LAUNCHER
    chmod +x "$INSTALL_DIR/minecraft-lce-server"
}

BUILD_DIR="$SOURCE_DIR/build/windows64-clang"
mkdir -p "$BUILD_DIR"
info "LegacyEvolved LCE v$VERSION build script"
info "Source: $SOURCE_DIR | Type: $BUILD_TYPE"
echo ""
check_deps
fetch_winsdk
patch_winsdk_symlinks
do_cmake_configure
do_build
do_install
