# Changelog

All notable changes to GammaHotkey are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
(`Major.Minor.Hotfix`).

Add entries under `## [Unreleased]` as work lands; at release time that heading is renamed to
`## [X.Y.Z] - YYYY-MM-DD`. Within each version, group changes under `Added`, `Changed`,
`Fixed`, `Removed`, `Deprecated`, and `Security`.

## [Unreleased] - YYYY-MM-DD

_Changes for the next release land here._

## [1.0.0] - Draft pending release

First complete release, overhauling several systems from the beta releases, and numerous minor changes and additions.

### Added

- Real-time **brightness, contrast, and gamma** adjustment for your display, applied at the
  GPU via the Windows `SetDeviceGammaRamp` API. Because it adjusts the display's gamma ramp
  directly, screenshots, recordings, and streams capture the original unadjusted image.
- **Global hotkeys** (via `RegisterHotKey`) to toggle adjustments on/off and to switch
  between profiles. Active system-wide, even when the window is hidden in the tray.
- **Simple mode**, one set of brightness/contrast/gamma controls with a toggle-on/off hotkey.
- **Advanced mode**, profile-based control:
  - Create unlimited profiles, each storing its own brightness, contrast, and gamma.
  - Assign a hotkey to any profile for instant switching.
  - Cycle through all profiles with **Next Profile** / **Previous Profile** hotkeys.
  - Edit, delete, and re-order profiles.
- **Pseudo multi-monitor support**: target a specific display; run multiple renamed copies of the
  executable to control several displays independently, each with its own config.
- **Portable, no installation**: a single executable that stores its settings in a UTF-8
  `.ini` alongside it, using the same basename (e.g. `GammaHotkey.ini`).
- **System tray operation**: idles in the background from the tray, with options to run in
  the background when closed, run in the background when launched, toggle adjustments on at
  launch, and launch on Windows startup. Exiting reverts gamma to default.
- **Per-monitor DPI awareness (V2)**, for a crisp UI across mixed-DPI setups.

[Unreleased]: https://github.com/maxgodman/GammaHotkey/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/maxgodman/GammaHotkey/releases/tag/v1.0.0
