<p align="center">
  <img src="sce_sys/icon0.png" width="160" height="160" alt="ApricotGS project icon">
</p>

<h1 align="center">ApricotGS</h1>

<p align="center">
  <strong>HeartGold &amp; SoulSilver on PlayStation 5</strong><br>
  A PS5 homebrew app maintained by <a href="https://github.com/BaZZtian">BaZZtian</a>.
</p>

<p align="center">
  <a href="#status-and-availability"><img src="https://img.shields.io/badge/status-experimental%20release-orange" alt="Status: experimental release"></a>
  <img src="https://img.shields.io/badge/platform-PS5%20homebrew-003791" alt="Platform: PS5 homebrew">
  <a href="#requirements"><img src="https://img.shields.io/badge/game%20data-Own%20NDS%20dump-555555" alt="Game data: Own NDS dump"></a>
</p>

<p align="center">
  <a href="#status-and-availability">Status</a> &middot;
  <a href="#installation">Installation</a> &middot;
  <a href="#compatibility">Compatibility</a> &middot;
  <a href="#support">Support</a> &middot;
  <a href="#credits-and-licenses">Credits &amp; licenses</a>
</p>

## About

Play HeartGold and SoulSilver on your PS5 with ApricotGS. Install the app, add
your own supported `.nds` game file, and let the PS5 prepare it. Then choose
your edition and start playing.

**No game is included.** You need your own legally obtained game dump.

## Status and availability

Download **[ApricotGS v00.040.005](https://github.com/BaZZtian/PS5-ApricotGS/releases/tag/v00.040.005)**.
Choose **ApricotGS-v00.040.005-PS5.zip** for the application. The release page also
contains the version history and known issues.

This is an experimental release. A complete playthrough has not been verified.

## Requirements

- A homebrew-enabled PS5 that can run native PS5 homebrew applications.
- Your own supported HeartGold or SoulSilver `.nds` file; see [Compatibility](#compatibility).
- A PS5 file manager or file-transfer service to copy your game file onto the console.

## Installation

### 1. Install the app

Download **ApricotGS-v00.040.005-PS5.zip** from the release page and extract its
`PPSA99429` folder into `/data/homebrew/` on the PS5. The executable should be at:

```text
/data/homebrew/PPSA99429/eboot.bin
```

### 2. Select your own game

Start ApricotGS and open the library. Choose **Lokal** to browse a readable
folder you select, including a connected USB drive, or **RomM** to connect to
your server by URL or hostname. For the inbox, use `/data/apricotgs-roms/`.
Only supported individual `.nds` files can be imported; archives are not imported.

For RomM, enter your own server address. Sign in with username/password or use
**Geraet koppeln** and the eight-character code from your RomM device-pairing
screen. The app stores the connection privately so you can add more games later.
Use **Abmelden und Verbindung loeschen** to remove the saved connection.
An expired or revoked login may require signing in again. Passwords are not saved.

### 3. Import and play

Select a supported file and confirm the import. ApricotGS verifies and prepares
it on the PS5; this can take several minutes. Keep your original files as a backup.
Choose an edition with the **D-pad or left stick**, then press **Cross** to start.
The selection screen also appears when you have added only one edition.
Press **Triangle** there to reopen the library and add another game.


## Features

- Local game-file browser and RomM library with download progress/cancellation.
- Remembered RomM connection, device pairing and explicit logout.
- Import management with confirmation; saves are kept when an import is removed.
- Consistent glass surfaces, readable focus/case indicators and original controller prompts.
- Animated cartridge selection with edition-specific title images and music.
- Continuous title music when switching cartridges, plus Home-screen tile music.
- DualSense controls and an on-screen pointer for the touchscreen.
- A two-screen layout with a larger main view.
- Separate saves for HeartGold and SoulSilver.

## Saving and updates

**Importierte Spiele verwalten** removes an imported edition after confirmation.
Original files and saves are preserved. The active edition cannot be removed.
Deleting the saved RomM connection does not delete server games or local saves.


Save through the game's own save menu and wait for it to finish before closing
the app. To exit during play, press **L3 + R3** together. On the cartridge
selection screen, press **Options** to exit.

For an update, replace the application folder with the new release, or use
ProsperoStore when that update is available there. Keep these folders:

- `/data/apricotgs-roms/` â€” your input game files.
- `/data/orqenva/` â€” prepared game data and saves; saves are under `saves/`.

Back up your saves before updating. Normally, prepared games are reused on later
starts. If repair is needed, keep the matching original `.nds` file in the input
folder so the app can prepare a replacement.

Home-screen music is created during setup. After preparation, close the app
and allow the PS5 Home screen time to refresh the title's music.

## Compatibility

Currently supported: **unmodified German revision-0 releases** of:

| Edition | Game code |
| --- | --- |
| HeartGold | `IPKD` |
| SoulSilver | `IPGD` |

Other languages, revisions and modified dumps are not currently supported.
Wireless trading and online play are not available.

## Controls

| DualSense input | Game / application action |
| --- | --- |
| Cross | A / confirm |
| Circle | B / back |
| Triangle | X |
| Square | Select |
| Touchpad click | Y |
| Options | Start; exit from cartridge selection |
| L1 / R1 | L / R |
| D-pad / left stick | Movement; cartridge selection |
| Right stick | Touchscreen pointer |
| R2 | Touchscreen press |
| L3 + R3 | Exit during play |

## Support

If a game does not appear, check the supported edition, the `.nds` file's location
and whether preparation finished. If an error occurs, note its exact text.

Open a [GitHub issue](https://github.com/BaZZtian/PS5-ApricotGS/issues) with the
app version, edition, steps that caused the problem and whether it happens again
after restarting. You may voluntarily attach `/data/orqenva/debug-report.json`.
Do not upload game files, saves or credentials.

## Credits and licenses

ApricotGS is maintained by **BaZZtian** as an independent PS5 port project. This does not confer ownership of the original game, its engine or third-party contributions. Original authors retain their copyright and license terms. Independently authored contributions remain controlled by their actual rightsholders; no additional license is granted by assumption.

Platform contributions include attributed work by **Mihawk-99 / Mihawk** and **BlackBearReloaded**. The native graphics path uses the PS5 Vulkan/Mesa work and the homebrew payload SDK. Controller prompts derive from **AHarmlessPotato's** CC BY 4.0 DualSense model renders, with the required attribution and modification notices. Component notices, exact dependency source revisions and source build entry points are included in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt). Component licenses are retained under licenses/.

The existing [MIT license](LICENSE) applies to the original work covered by that notice. It does not relicense GPL/LGPL components, controller artwork, proprietary game content or the combined runtime as a whole. Missing rights or incompatible license obligations block publication; third-party notices are retained.

Game names are used only to identify compatibility. No game content is supplied, no affiliation with Nintendo, Game Freak or Sony is claimed, and users must have the rights required to prepare their own data under applicable law. Providing your own dump is not a blanket legal exemption.

## Corresponding source and building

The release provides automatic **Source code (zip)** and **Source code (tar.gz)**
downloads at [v00.040.005](https://github.com/BaZZtian/PS5-ApricotGS/releases/tag/v00.040.005).
Pinned dependency revisions and license/relinking requirements are in
[THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
Use `platform/ps5/tools/build-game-ps5.sh` with `ORQENVA_VULKAN_ROOT` pointing to
the pinned PS5 Vulkan/dependency build, then `python3 assets/prepare-ui.py <app-directory>`
for the project-owned display assets. No game data or saved connection is included.
