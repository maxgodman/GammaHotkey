# Fonts in GammaHotkey

## What we ship

**Cascadia Mono, Regular (static instance)** from release
[`v2407.24`](https://github.com/microsoft/cascadia-code/releases/tag/v2407.24), subset to
**U+0020-00FF** and embedded in the binary as a compressed byte array. Licensed **SIL OFL 1.1**
(Microsoft). Design size is `style.FontSizeBase = 15.0f`.

Subsetting is what keeps it small: the 576 KB source TTF becomes a 31 KB subset and a 23 KB
compressed array.

Nothing is read from disk and nothing is generated at build time - a clean clone builds as-is.
The tooling below is only needed to *change* the font.

## Changing or regenerating the font

1. **Get the font.** Take `ttf/static/<Face>-Regular.ttf` from the
   [release zip](https://github.com/microsoft/cascadia-code/releases) - the **static** instance,
   not the variable font at `ttf/<Face>.ttf`, and **Mono** rather than **Code**, which has
   programming ligatures. Drop it in `resources/fonts/`.

   The zip holds only `otf/`, `ttf/` and `woff2/`, with no licence file, so also take `LICENSE`
   from the repo at the matching tag and save it as `resources/fonts/LICENSE-<Face>.txt`.

2. **Regenerate the header.**

   ```
   powershell -NoProfile -File scripts/regenerate-font.ps1
   ```

   It subsets, compresses and rewrites `src/ui/Font_CascadiaMono.h`. `-FontPath`, `-Unicodes`,
   `-SymbolName` and `-OutputPath` override the defaults. fontTools is installed into a
   throwaway virtualenv rather than the machine's Python; MSVC comes from vswhere. Output is
   deterministic, so an unchanged font leaves the header untouched.

3. **Rebuild and check** text at 100%, 150% and 200%, and dragging between monitors of different
   scale - the typeface should be identical and crisp everywhere, and must not change
   mid-session.

## How it's wired

- [`src/ui/Font_CascadiaMono.h`](src/ui/Font_CascadiaMono.h) - the generated array. Tool output;
  do not hand-edit.
- [`src/ui/ImGui_Integration.cpp`](src/ui/ImGui_Integration.cpp) - the
  `AddFontFromMemoryCompressedTTF` call in `ImGuiRenderer::Initialize`, passing neither
  `size_pixels` nor a glyph range: since 1.92 the size comes from `style.FontSizeBase` and glyphs
  load on demand. The array stays owned by the caller, so `static const` in a header is right.
- [`src/ui/UI_Shared.cpp`](src/ui/UI_Shared.cpp) - `style.FontSizeBase` in `ApplyImGuiStyle`.
- [`GammaHotkey.vcxproj`](GammaHotkey.vcxproj) - `IMGUI_DISABLE_DEFAULT_FONT`, in **all four**
  configuration blocks. Do not set it in `external/imgui/imconfig.h`; that is a vendored
  submodule file and the edit would be lost on update.

## Gotchas

- **Static instance, never the variable font.** ImGui rasterizes with stb_truetype, which has no
  variable-font support. The script rejects any font carrying an `fvar` table.
- **There is no fallback font.** `IMGUI_DISABLE_DEFAULT_FONT` compiles out ProggyClean and
  ProggyForever (~23 KB) and makes `AddFontDefaultXXX()` assert, so if our font fails to load
  there is no font at all. That is exactly why it is embedded rather than read from disk.
- **`style.FontSizeBase` has to stay inside `ApplyImGuiStyle`.** `OnDpiChanged` does
  `style = ImGuiStyle()` and then calls `ApplyImGuiStyle()`; set the size anywhere else and it
  silently reverts to ImGui's default after the first DPI change.
- **Coverage comes from the TTF, not from code.** U+0020-00FF is exactly
  `GetGlyphRangesDefault()`, what ImGui's own default fonts covered. Display names come from the
  driver and are not guaranteed Latin-1, so a name outside the range renders as tofu - widen
  `-Unicodes` if that ever matters.
- **To confirm the wiring took, search the linked exe** for the first 12+ bytes of
  `CascadiaMono_compressed_data` and of `proggy_clean_ttf_compressed_data` (`imgui_draw.cpp`):
  ours present, Proggy absent. Fewer bytes than that will not do - every stb-compressed array
  opens with the same 10-byte magic.

## Licensing obligations

The OFL requires the copyright notice and licence text to accompany the font even when bundled
with software. MIT does not conflict with that, but the app ships as a single portable exe, so
the repo's licence file never reaches users - the About dialog is the only thing that does:

- `resources/fonts/LICENSE-CascadiaMono.txt` in the repo.
- README, under *Third-party notices*.
- `IDS_ABOUT_THIRDPARTY` in the string table, rendered by `RenderAboutDialog`. Keep its lines
  short: the longest one sets the auto-resizing About dialog's width, and Simple mode gives it
  only 450 logical pixels to fit in.

The OFL also forbids selling the font on its own and reserves the name for modified versions;
neither applies to embedding it unmodified.
