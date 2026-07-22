# Releasing GammaHotkey

This is the documentation regarding the release process, and acts as a runbook for cutting a new release.

## Version scheme

GammaHotkey uses **`Major.Minor.Hotfix`** (semantic versioning). This maps to Windows mandatory four-part field as
**`Major.Minor.Hotfix.0`**. The fourth slot is always `0` (reserved for a build counter if needed).

There are no hard rules on how we bump versions, but generally speaking:
- Major: Anything that breaks incompatibility with config files, and major feature updates.
- Minor: Pretty much anything feature related that isn't big enough to consider a major update.
- Hotfix: Just bugfixes.
- Build Counter: If you need it.

## Where the version lives

1. [**`resources/Resource.h`**](resources/Resource.h):
   ```c
   #define VER_FILEVERSION        1,0,0,0
   #define VER_FILEVERSION_STR    "1.0.0"
   #define VER_PRODUCTVERSION     1,0,0,0
   #define VER_PRODUCTVERSION_STR "1.0.0"
   ```
   The string versions use the three-part versioning, the rest use the four-part as required by Windows.

2. [**`resources/GammaHotkey.manifest`**](resources/GammaHotkey.manifest):
   ```xml
   <assemblyIdentity type="win32" name="GammaHotkey" version="1.0.0.0" processorArchitecture="*"/>
   ```

## Release checklist

1. **Decide the number**. 

2. **Bump version on all files** to the new version (see above).

3. **Update [`CHANGELOG.md`](CHANGELOG.md)**:<br>
   Rename the `## [Unreleased]` heading to `## [X.Y.Z] - YYYY-MM-DD` (today's date), leave a fresh
   empty `## [Unreleased]` section above it, and update the comparison links at the bottom of the file.
   The notes you write here are the source for the GitHub release notes used later.

4. **Build a clean Release binary**, see [`README.md`](README.md) for build info.

5. **Verify the version took** and smoke test:
   - Right-click `x64/Release/GammaHotkey.exe` → **Properties → Details**. *Product version*
     reads the three-part `X.Y.Z`; *File version* reads the four-part `X.Y.Z.0`.
   - Launch it and open **About**, the version line reads the three-part `X.Y.Z`.

6. **Commit the bump**.
   ```
   git add resources/Resource.h resources/GammaHotkey.manifest CHANGELOG.md
   git commit -m "Bump version to 1.0.0"
   ```

7. **Tag the release.** Use an **annotated** tag (carries a message and date; lightweight
   tags don't), prefixed with `v`:
   ```
   git tag -a v1.0.0 -m "GammaHotkey 1.0.0"
   ```
   The tag should point at the version-bump commit from step 6.

8. **Push the commit and the tag**:
   ```
   git push origin main
   git push origin v1.0.0
   ```

9. **Create the GitHub Release**:
   - On GitHub → *Releases* → *Draft a new release* → pick the `v1.0.0` tag.
   - Paste the release notes from `CHANGELOG.md`, and attach the built `GammaHotkey.exe`.
