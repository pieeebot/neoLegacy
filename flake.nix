{
  description = "LCE-Revelations - Minecraft Legacy Console Edition recreation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };

        # Version info
        version = "1.6.0560.0";

        # Windows SDK downloaded via xwin (fixed-output derivation)
        windowsSdk = pkgs.stdenvNoCC.mkDerivation {
          pname = "windows-sdk-xwin";
          version = "0.6.0";

          outputHashAlgo = "sha256";
          outputHashMode = "recursive";
          outputHash = "sha256-ksSytBUjv/tD3IJzHM9BkAzFjJ+JAGD353Pur0G4rQE=";

          nativeBuildInputs = [ pkgs.xwin pkgs.cacert pkgs.rsync ];

          dontUnpack = true;

          buildPhase = ''
            runHook preBuild

            export HOME=$(mktemp -d)
            TEMP_OUT=$(mktemp -d)

            # Download and splat to temp directory first (same filesystem)
            xwin --accept-license splat --output "$TEMP_OUT"

            # Copy to actual output (handles cross-device)
            mkdir -p $out
            rsync -a "$TEMP_OUT/" "$out/"

            runHook postBuild
          '';

          dontInstall = true;
          dontFixup = true;
        };

        # Helper to create case-insensitive symlinks for SDK headers/libs
        sdkWithSymlinks = pkgs.runCommand "windows-sdk-symlinked" {} ''
          cp -r ${windowsSdk} $out
          chmod -R u+w $out

          # SDK header symlinks (case sensitivity fixes)
          ln -sf $out/sdk/include/shared/sdkddkver.h $out/sdk/include/shared/SDKDDKVer.h 2>/dev/null || true

          # Library symlinks (case sensitivity fixes)
          ln -sf $out/sdk/lib/um/x86_64/xinput9_1_0.lib $out/sdk/lib/um/x86_64/XInput9_1_0.lib 2>/dev/null || true
          ln -sf $out/sdk/lib/um/x86_64/ws2_32.lib $out/sdk/lib/um/x86_64/Ws2_32.lib 2>/dev/null || true
        '';

        # CMake toolchain file for clang-cl cross-compilation
        clangClToolchain = pkgs.writeText "clang-cl-toolchain.cmake" ''
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
        '';

        # The main build derivation
        minecraft-lce-unwrapped = pkgs.stdenv.mkDerivation {
          pname = "minecraft-lce-unwrapped";
          inherit version;

          src = pkgs.lib.cleanSourceWith {
            src = ./.;
            filter = path: type:
              let
                baseName = baseNameOf path;
              in
              # Exclude build directories and other non-source files
              !(baseName == "build" ||
                baseName == "result" ||
                baseName == ".git" ||
                baseName == ".direnv" ||
                pkgs.lib.hasPrefix "result-" baseName);
          };

          nativeBuildInputs = with pkgs; [
            llvmPackages.clang-unwrapped  # provides clang-cl
            llvmPackages.lld               # provides lld-link
            llvmPackages.llvm              # provides llvm-rc, llvm-ml, llvm-lib, llvm-mt
            cmake
            ninja
            rsync
          ];

          # Set up environment for clang-cl
          WINSDK = sdkWithSymlinks;

          configurePhase = ''
            runHook preConfigure

            export INCLUDE="$WINSDK/crt/include;$WINSDK/sdk/include/um;$WINSDK/sdk/include/ucrt;$WINSDK/sdk/include/shared"
            export LIB="$WINSDK/crt/lib/x86_64;$WINSDK/sdk/lib/um/x86_64;$WINSDK/sdk/lib/ucrt/x86_64"

            cmake -S . -B build \
              -G Ninja \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_TOOLCHAIN_FILE=${clangClToolchain} \
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
              -DCMAKE_C_FLAGS="/MT -fms-compatibility -fms-extensions --target=x86_64-pc-windows-msvc -imsvc $WINSDK/crt/include -imsvc $WINSDK/sdk/include/ucrt -imsvc $WINSDK/sdk/include/um -imsvc $WINSDK/sdk/include/shared" \
              -DCMAKE_CXX_FLAGS="/MT -fms-compatibility -fms-extensions --target=x86_64-pc-windows-msvc -imsvc $WINSDK/crt/include -imsvc $WINSDK/sdk/include/ucrt -imsvc $WINSDK/sdk/include/um -imsvc $WINSDK/sdk/include/shared" \
              -DCMAKE_ASM_MASM_FLAGS="-m64" \
              -DCMAKE_EXE_LINKER_FLAGS="-libpath:$WINSDK/crt/lib/x86_64 -libpath:$WINSDK/sdk/lib/um/x86_64 -libpath:$WINSDK/sdk/lib/ucrt/x86_64"

            runHook postConfigure
          '';

          buildPhase = ''
            runHook preBuild
            cmake --build build --config Release -j $NIX_BUILD_CORES
            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall

            mkdir -p $out/{client,server}

            # Install client
            cp build/Minecraft.Client/Minecraft.Client.exe $out/client/
            cp build/Minecraft.Client/iggy_w64.dll $out/client/ 2>/dev/null || true
            cp -r build/Minecraft.Client/Common $out/client/ 2>/dev/null || true
            cp -r build/Minecraft.Client/music $out/client/ 2>/dev/null || true
            cp -r build/Minecraft.Client/Windows64 $out/client/ 2>/dev/null || true
            cp -r build/Minecraft.Client/Windows64Media $out/client/ 2>/dev/null || true
            cp -r build/Minecraft.Client/iggy* $out/client/ 2>/dev/null || true

            # Install server
            cp build/Minecraft.Server/Minecraft.Server.exe $out/server/
            cp build/Minecraft.Server/iggy_w64.dll $out/server/ 2>/dev/null || true
            cp -r build/Minecraft.Server/Common $out/server/ 2>/dev/null || true
            cp -r build/Minecraft.Server/Windows64 $out/server/ 2>/dev/null || true

            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "Minecraft Legacy Console Edition recreation (Windows executables)";
            homepage = "https://github.com/minecraft-lce/MinecraftConsoles";
            license = licenses.unfree;
            platforms = [ "x86_64-linux" ];
          };
        };

        # Wine package using staging for better performance
        winePackage = pkgs.wineWow64Packages.staging;

        # Wine prefix base path
        winePrefixBase = ".local/share/minecraft-lce";

        # Wrapped client package
        minecraft-lce-client = pkgs.stdenv.mkDerivation {
          pname = "minecraft-lce-client";
          inherit version;

          dontUnpack = true;
          dontBuild = true;

          nativeBuildInputs = [ pkgs.makeWrapper ];

          installPhase = ''
            runHook preInstall

            mkdir -p $out/bin
            mkdir -p $out/share/minecraft-lce-client

            # Copy game files
            cp -r ${minecraft-lce-unwrapped}/client/* $out/share/minecraft-lce-client/

            # Create wrapper script
            cat > $out/bin/minecraft-lce-client << 'WRAPPER'
            #!/usr/bin/env bash
            set -euo pipefail
            
            GAME_DIR="@gameDir@"
            PERSIST_DIR="''${MC_DATA_DIR:-$HOME/.local/share/minecraft-lce-client}"
            
            export WINEARCH=win64
            export WINEPREFIX="''${WINEPREFIX:-$HOME/@winePrefixBase@-client}"
            
            # Wine performance settings
            export WINEDLLOVERRIDES="winemenubuilder.exe=d"
            export WINEESYNC=1
            export WINEFSYNC=1
            export DXVK_LOG_LEVEL=none
            
            mkdir -p "$PERSIST_DIR"
            mkdir -p "$WINEPREFIX"
            
            # Create working directory with symlinks to immutable store
            WORK_DIR="$(mktemp -d)"
            trap 'rm -rf "$WORK_DIR"' EXIT
            
            cp -rs "$GAME_DIR"/* "$WORK_DIR/"
            chmod -R u+w "$WORK_DIR"
            
            # Setup persistent data directory
            mkdir -p "$PERSIST_DIR/GameHDD"
            rm -rf "$WORK_DIR/Windows64/GameHDD" 2>/dev/null || true
            ln -sf "$PERSIST_DIR/GameHDD" "$WORK_DIR/Windows64/GameHDD"
            
            cd "$WORK_DIR"
            
            echo "[info] Starting Minecraft LCE client"
            echo "[info] Data directory: $PERSIST_DIR"
            echo "[info] Wine prefix: $WINEPREFIX"
            
            exec wine "$WORK_DIR/Minecraft.Client.exe" "$@"
            WRAPPER

            chmod +x $out/bin/minecraft-lce-client

            substituteInPlace $out/bin/minecraft-lce-client \
              --replace "@gameDir@" "$out/share/minecraft-lce-client" \
              --replace "@winePrefixBase@" "${winePrefixBase}"

            # Use wrapProgram to add Wine to PATH (creates proper runtime closure)
            wrapProgram $out/bin/minecraft-lce-client \
              --prefix PATH : "${winePackage}/bin"

            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "Minecraft Legacy Console Edition - Client";
            homepage = "https://github.com/minecraft-lce/MinecraftConsoles";
            license = licenses.unfree;
            platforms = [ "x86_64-linux" ];
            mainProgram = "minecraft-lce-client";
          };
        };

        # Wrapped server package
        minecraft-lce-server = pkgs.stdenv.mkDerivation {
          pname = "minecraft-lce-server";
          inherit version;

          dontUnpack = true;
          dontBuild = true;

          nativeBuildInputs = [ pkgs.makeWrapper ];

          installPhase = ''
            runHook preInstall

            mkdir -p $out/bin
            mkdir -p $out/share/minecraft-lce-server

            # Copy game files
            cp -r ${minecraft-lce-unwrapped}/server/* $out/share/minecraft-lce-server/

            # Create wrapper script
            cat > $out/bin/minecraft-lce-server << 'WRAPPER'
            #!/usr/bin/env bash
            set -euo pipefail
            
            GAME_DIR="@gameDir@"
            SERVER_PORT="''${MC_PORT:-25565}"
            SERVER_BIND_IP="''${MC_BIND:-0.0.0.0}"
            PERSIST_DIR="''${MC_DATA_DIR:-$HOME/.local/share/minecraft-lce-server}"
            
            export WINEARCH=win64
            export WINEPREFIX="''${WINEPREFIX:-$HOME/@winePrefixBase@-server}"
            
            # Wine settings
            export WINEDLLOVERRIDES="winemenubuilder.exe=d"
            export WINEESYNC=1
            export WINEFSYNC=1
            
            mkdir -p "$PERSIST_DIR"
            mkdir -p "$WINEPREFIX"
            
            # Create working directory with symlinks to immutable store
            WORK_DIR="$(mktemp -d)"
            trap 'rm -rf "$WORK_DIR"' EXIT
            
            cp -rs "$GAME_DIR"/* "$WORK_DIR/"
            chmod -R u+w "$WORK_DIR"
            
            # Setup persistent data
            mkdir -p "$PERSIST_DIR/GameHDD"
            
            for file in server.properties banned-players.json banned-ips.json; do
              if [[ ! -f "$PERSIST_DIR/$file" ]]; then
                if [[ -f "$WORK_DIR/$file" ]]; then
                  cp "$WORK_DIR/$file" "$PERSIST_DIR/$file"
                else
                  echo "[]" > "$PERSIST_DIR/$file"
                fi
              fi
              ln -sf "$PERSIST_DIR/$file" "$WORK_DIR/$file"
            done
            
            rm -rf "$WORK_DIR/Windows64/GameHDD" 2>/dev/null || true
            ln -sf "$PERSIST_DIR/GameHDD" "$WORK_DIR/Windows64/GameHDD"
            
            cd "$WORK_DIR"
            
            # Start Xvfb if no display (server may require a virtual display)
            if [[ -z "''${DISPLAY:-}" ]]; then
              export DISPLAY=":99"
              Xvfb "$DISPLAY" -nolisten tcp -screen 0 64x64x16 &
              XVFB_PID=$!
              trap 'kill $XVFB_PID 2>/dev/null || true; rm -rf "$WORK_DIR"' EXIT
              sleep 1
              echo "[info] Started Xvfb on $DISPLAY"
            fi
            
            echo "[info] Starting Minecraft LCE server on $SERVER_BIND_IP:$SERVER_PORT"
            echo "[info] Data directory: $PERSIST_DIR"
            echo "[info] Wine prefix: $WINEPREFIX"
            
            exec wine "$WORK_DIR/Minecraft.Server.exe" -port "$SERVER_PORT" -bind "$SERVER_BIND_IP" "$@"
            WRAPPER

            chmod +x $out/bin/minecraft-lce-server

            substituteInPlace $out/bin/minecraft-lce-server \
              --replace "@gameDir@" "$out/share/minecraft-lce-server" \
              --replace "@winePrefixBase@" "${winePrefixBase}"

            # Use wrapProgram to add Wine and Xvfb to PATH (creates proper runtime closure)
            wrapProgram $out/bin/minecraft-lce-server \
              --prefix PATH : "${winePackage}/bin:${pkgs.xorg-server}/bin"

            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "Minecraft Legacy Console Edition - Dedicated Server";
            homepage = "https://github.com/minecraft-lce/MinecraftConsoles";
            license = licenses.unfree;
            platforms = [ "x86_64-linux" ];
            mainProgram = "minecraft-lce-server";
          };
        };

        # Build script for development
        buildScript = pkgs.writeShellApplication {
          name = "minecraft-lce-build";
          runtimeInputs = with pkgs; [
            llvmPackages.clang-unwrapped
            llvmPackages.lld
            llvmPackages.llvm
            cmake
            ninja
            xwin
            rsync
            coreutils
            cacert
          ];
          text = ''
            set -euo pipefail

            SOURCE_DIR="''${1:-.}"
            BUILD_TYPE="''${2:-Release}"
            XWIN_CACHE="''${XWIN_CACHE:-$HOME/.cache/xwin}"

            export XWIN_CACHE

            echo "[info] Checking Windows SDK cache at $XWIN_CACHE"

            if [[ ! -d "$XWIN_CACHE/splat" ]]; then
              echo "[info] Downloading Windows SDK and CRT via xwin..."
              mkdir -p "$XWIN_CACHE"
              xwin --accept-license splat --output "$XWIN_CACHE/splat"
            else
              echo "[info] Using cached Windows SDK"
            fi

            WINSDK="$XWIN_CACHE/splat"

            export INCLUDE="$WINSDK/crt/include;$WINSDK/sdk/include/um;$WINSDK/sdk/include/ucrt;$WINSDK/sdk/include/shared"
            export LIB="$WINSDK/crt/lib/x86_64;$WINSDK/sdk/lib/um/x86_64;$WINSDK/sdk/lib/ucrt/x86_64"

            BUILD_DIR="$SOURCE_DIR/build/windows64-clang"
            mkdir -p "$BUILD_DIR"

            echo "[info] Configuring with CMake..."
            cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
              -G Ninja \
              -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
              -DCMAKE_TOOLCHAIN_FILE="${clangClToolchain}" \
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
              -DCMAKE_C_FLAGS="/MT -fms-compatibility -fms-extensions --target=x86_64-pc-windows-msvc -imsvc $WINSDK/crt/include -imsvc $WINSDK/sdk/include/ucrt -imsvc $WINSDK/sdk/include/um -imsvc $WINSDK/sdk/include/shared" \
              -DCMAKE_CXX_FLAGS="/MT -fms-compatibility -fms-extensions --target=x86_64-pc-windows-msvc -imsvc $WINSDK/crt/include -imsvc $WINSDK/sdk/include/ucrt -imsvc $WINSDK/sdk/include/um -imsvc $WINSDK/sdk/include/shared" \
              -DCMAKE_ASM_MASM_FLAGS="-m64" \
              -DCMAKE_EXE_LINKER_FLAGS="-libpath:$WINSDK/crt/lib/x86_64 -libpath:$WINSDK/sdk/lib/um/x86_64 -libpath:$WINSDK/sdk/lib/ucrt/x86_64"

            echo "[info] Building..."
            cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$(nproc)"

            echo "[info] Build complete! Output in $BUILD_DIR"
          '';
        };

      in
      {
        packages = {
          # Main packages - build from source
          client = minecraft-lce-client;
          server = minecraft-lce-server;

          # Unwrapped (just the Windows executables)
          unwrapped = minecraft-lce-unwrapped;

          # Windows SDK (for debugging)
          windows-sdk = sdkWithSymlinks;

          default = minecraft-lce-client;
        };

        apps = {
          client = {
            type = "app";
            program = "${minecraft-lce-client}/bin/minecraft-lce-client";
          };
          server = {
            type = "app";
            program = "${minecraft-lce-server}/bin/minecraft-lce-server";
          };
          build = {
            type = "app";
            program = "${buildScript}/bin/minecraft-lce-build";
          };
          default = {
            type = "app";
            program = "${minecraft-lce-client}/bin/minecraft-lce-client";
          };
        };

        devShells.default = pkgs.mkShell {
          name = "minecraft-lce-dev";
          packages = with pkgs; [
            # Cross-compilation toolchain
            llvmPackages.clang-unwrapped
            llvmPackages.lld
            llvmPackages.llvm
            cmake
            ninja
            xwin
            rsync

            # Wine for testing
            winePackage
            winetricks

            # For running server without display
            xvfb-run

            # Useful tools
            git
            cacert
          ];

          XWIN_CACHE = "$HOME/.cache/xwin";

          shellHook = ''
            echo "LCE-Revelations development shell"
            echo ""
            echo "Quick build (uses cached SDK):"
            echo "  nix build .#client   # Build client package"
            echo "  nix build .#server   # Build server package"
            echo ""
            echo "Development build (in-tree):"
            echo "  minecraft-lce-build [source_dir] [Release|Debug]"
            echo ""
            echo "Run:"
            echo "  nix run .#client"
            echo "  nix run .#server"
            echo ""
            echo "Environment variables:"
            echo "  MC_PORT     - Server port (default: 25565)"
            echo "  MC_BIND     - Server bind address (default: 0.0.0.0)"
            echo "  MC_DATA_DIR - Persistent data directory"
            echo "  WINEPREFIX  - Custom Wine prefix"
            echo ""
          '';
        };
      }
    );
}
