# Build and developer documentation

[Back to the user guide](README.md)

## Corresponding source and building

The release provides automatic **Source code (zip)** and **Source code (tar.gz)**
downloads at [v00.040.007](https://github.com/BaZZtian/PS5-ApricotGS/releases/tag/v00.040.007).
Pinned dependency revisions and license/relinking requirements are in
[THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
Use `platform/ps5/tools/build-game-ps5.sh` with `ORQENVA_VULKAN_ROOT` pointing to
the pinned PS5 Vulkan/dependency build, then `python3 assets/prepare-ui.py <app-directory>`
for the project-owned display assets. No game data or saved connection is included.
