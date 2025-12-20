# GammaHotkey

**Apply display gamma changes using hotkeys, for Windows**

GammaHotkey is a lightweight portable application that allows you to adjust your display's brightness, contrast, and gamma using hotkeys.<br>
Built as a clean modern solution to this problem, with functionality not found in alternative applications.

![Version](https://img.shields.io/badge/version-0.1-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

## 🚀 Getting Started

1. Download the latest release from the [Releases](../../releases) page.
3. Run `GammaHotkey.exe` - **No installation!**
4. Adjust sliders and options, bind an on/off hotkey, done!
5. Switch to Advanced mode for additional functionality.
6. Exit the app by right clicking the icon in the system tray and select "Exit" - this will revert gamma back to default.

## 🏆 Pro-Tip

Use 2 hotkeys to cycle through as many adjustments as you like!

1. Switch to Advanced mode.
2. Bind the **Previous Profile** and **Next Profile** hotkeys.
3. Create several profiles with incremental adjustments for each scenario you may need - order them as desired.
4. Cycle through the profiles with the hotkeys, and try the on/off hotkey too!

## ✨ Features

### 🪶 Lightweight Portable Application

- **No installation** - just a small portable executable.
- Config file is stored next to the executable, with matching name. e.g. GammaHotkey.ini
- Make sure the config stays alongside the executable, if you rename the .exe, rename the .ini to match!

### 🎯 Simple and Advanced Modes

- **Simple mode** - Quick access to a single set of controls and options, with a toggle on/off hotkey.
- **Advanced mode** - Manage gamma using profiles, bind keys to profiles, cycle through profiles sequentially, and more!

### ⚡ Hotkeys

- **Toggle On/Off** - Enable/disable the current gamma adjustments.
- **Next/Previous Profile** - Cycle through all profiles using one or two keys.
- **Per Profile Hotkeys** - Bind a key to each profile.
- Hotkey binding supports any key!


### 💾 Profile Management (Advanced Mode)

- Create unlimited profiles, storing unique brightness, contrast, and gamma settings.
- Assign hotkeys to profiles for instant switching.
- Edit, delete, and re-order profiles easily.

### 🖥️ Multi-Monitor Support

- **Targets a specific display** - select the display to apply gamma changes to.
- It is possible to run multiple instances of the app targeting different displays to manage gamma for multiple displays.<br>
To do this, duplicate the executable and give it a unique name, you can then run two instances of the application, each with their own configs.

### ⚠️ Screen Capture Unaffected

- GammaHotkey applies gamma adjustments directly to your display using Windows display APIs.
- **What you see** - the display appears brighter/darker/adjusted.
- **What gets recorded** - screenshots, screen recordings, and streaming software capture the original, unadjusted image.

## 🔧 Technical Details

### How It Works

GammaHotkey uses the Windows `SetDeviceGammaRamp()` API to directly modify your display adapter's gamma lookup table. This provides:

- **Hardware-level adjustments** - applied at the GPU before the signal reaches your monitor.
- **Zero performance impact** - no ongoing processing or system load.
- **Instant application** - changes take effect immediately.
- **Screen capture unaffected** - Recording software captures the pre-adjustment image.

### System Requirements

- **OS:** Windows 10 or later (64-bit).
- **Display:** Any monitor with gamma ramp support (most modern displays).
- **Privileges:** Standard user rights (no administrator required).

### Configuration File

Settings are stored in `{ExecutableName}.ini` in the same folder as the executable.

## 🛠️ Building from Source

### Prerequisites

- Visual Studio 2026
- Windows 10 SDK
- C++17 compiler

### Build Steps

1. Clone and enter the repository:
   ```bash
   git clone https://github.com/maxgodman/GammaHotkey.git
   cd GammaHotkey
   ```

2. Clone the ImGui submodule:
   ```bash
   git submodule update --init --recursive
   ```

3. Open `GammaHotkey.slnx` in Visual Studio 2026

4. Build the solution:
   - Configuration: Release
   - Platform: x64

4. Output: `x64/Release/GammaHotkey.exe`

### Dependencies

- **Dear ImGui** - included in `/external/imgui/`
- **DirectX 11** - included with Windows SDK
- **Windows API** - shell32.lib, ole32.lib

## 🐛 Troubleshooting

### "Gamma adjustments aren't working"

- **Display support:** Some monitors or virtual displays don't support gamma ramps, meaning this app will not work.
- **Try different displays:** If you have multiple displays, try selecting each one to check if the changes apply.
- **Verify toggle is ON:** Ensure gamma is enabled (check status in the title bar), adjusting settings via the UI will forcibly turn it on.

### "Hotkeys not working"

- **Check conflicts:** Another application might be using the same hotkey.
- **Administrator apps:** Some programs block global hotkeys - try running the app as admin.
- **Virtual machines:** Hotkeys may not work inside VMs.

### "Settings not saving"

- **File permissions:** Ensure the folder is writable (not in Program Files).
- **Antivirus:** Some security software may block config file writes.
- **Portable application:** Settings save next to the executable, make sure to keep them together.

### "I've opened it, where is it?"

- **Check system tray:** Look for the GammaHotkey icon (bottom-right of taskbar).
- **Right-click tray icon:** Select "Show" to restore the window, or left click on the icon.
- **Check options:** There are options to "Run in background when closed" and "Run in background when launched".

---

## 🤝 Contributing

Contributions are welcome! Please open an issue or submit a pull request.

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
