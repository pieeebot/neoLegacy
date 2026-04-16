![banner](.gitea/banner.png)
# neoLegacy (Legacy Console Edition)
This project aims to backport the newer title updates back to Legacy Console Edition (which is based on TU19).

[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/D6hEPNYeyn)
[![Emerald Launcher](https://img.shields.io/badge/Get_it_on-LCE_Emerald_Launcher-3fc724)](https://github.com/LCE-Hub/LCE-Emerald-Launcher)

# Our roadmap:
![Roadmap](.gitea/roadmap.png)

- Port Title Update 25 (100% complete) ( 🎉 )
- Port Title Update 31 (86.36% complete)

See our [Contributor's Guide](./CONTRIBUTING.md) for more information on the goals of this project.

# Download
Users can download our [Nightly Build](https://codeberg.org/piebot/LegacyEvolved/releases)! Simply download the `.zip` file and extract it!

# Acknowledgments

Huge thanks to the following projects:

- [Patoke/LCERenewed](https://github.com/Patoke/LCERenewed) - for some of the patches that required deep decompilation
- [itsRevela/LCE-Revelations](https://github.com/itsRevela/LCE-Revelations) - for providing a stable project for neoLegacy to continue with

# Build & Run

## Windows
1. Install [Visual Studio 2022](https://aka.ms/vs/17/release/vs_community.exe) or [newer](https://visualstudio.microsoft.com/downloads/).
2. Clone the repository.
3. Open the project folder from Visual Studio.
4. Set the build configuration to **Windows64 - Debug** (Release is also ok but missing some debug features), then build and run.

## GNU/Linux

We provide both a generic build script and a Nix flake.

- Nix: `nix run .#client`
- Generic: `./build-linux.sh`
