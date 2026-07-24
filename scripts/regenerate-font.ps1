# Regenerates the embedded UI font header from the TTF in resources/fonts/. See FONT.md.
# Run with no arguments to rebuild the shipped font; the output is deterministic, so an
# unchanged input produces an unchanged header.
#
# Subsetting needs fontTools, which is installed into a throwaway virtualenv under TEMP and
# deleted on exit - nothing is added to the machine's Python.

[CmdletBinding()]
param(
    [string]$FontPath   = "resources/fonts/CascadiaMono-Regular.ttf",
    [string]$Unicodes   = "U+0020-00FF",
    [string]$SymbolName = "CascadiaMono",
    [string]$OutputPath = "src/ui/Font_CascadiaMono.h"
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Resolve-RepoPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) { return $Path }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

# Path.GetRelativePath() is .NET Core only, and this runs under Windows PowerShell.
function Get-RepoRelativePath([string]$Path) {
    $full = [System.IO.Path]::GetFullPath($Path)
    $root = $repoRoot.TrimEnd("\") + "\"
    if ($full.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $full.Substring($root.Length).Replace("\", "/")
    }
    return $full
}

# Native tools signal failure through the exit code, not by throwing.
function Invoke-Checked([string]$What, [scriptblock]$Command) {
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "$What failed (exit code $LASTEXITCODE)." }
}

$FontPath   = Resolve-RepoPath $FontPath
$OutputPath = Resolve-RepoPath $OutputPath

if (-not (Test-Path $FontPath)) {
    throw "Font not found: $FontPath"
}

$converterSrc = Join-Path $repoRoot "external\imgui\misc\fonts\binary_to_compressed_c.cpp"
if (-not (Test-Path $converterSrc)) {
    throw "$converterSrc not found. Initialise the submodule: git submodule update --init"
}

# The Microsoft Store ships a 'python' stub that only opens the Store, so require the launcher.
$pyLauncher = (Get-Command py -ErrorAction SilentlyContinue).Source
if (-not $pyLauncher) {
    throw "Python launcher 'py' not found. Install Python 3 from python.org."
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Is Visual Studio installed?"
}
$vsPath = & $vswhere -latest -prerelease -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath | Select-Object -First 1
if (-not $vsPath) {
    throw "MSVC toolset not found via vswhere. Install the 'Desktop development with C++' workload."
}
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found under $vsPath."
}

$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) "gammahotkey-font-$([guid]::NewGuid().ToString('N').Substring(0, 8))"
New-Item -ItemType Directory -Path $tempDir | Out-Null

try {
    Write-Host "==> Installing fontTools into a throwaway virtualenv"
    $venvDir = Join-Path $tempDir "venv"
    Invoke-Checked "Virtualenv creation" { & $pyLauncher -3 -m venv $venvDir }
    $venvPython = Join-Path $venvDir "Scripts\python.exe"
    Invoke-Checked "fontTools install" {
        & $venvPython -m pip install --quiet --disable-pip-version-check fonttools
    }

    Write-Host "==> Inspecting $([System.IO.Path]::GetFileName($FontPath))"
        # Hyphenated so the filename cannot shadow a stdlib module on sys.path.
    $inspectScript = Join-Path $tempDir "read-font-info.py"
    @'
import sys
from fontTools.ttLib import TTFont
font = TTFont(sys.argv[1], lazy=True)
def name(id):
    return font["name"].getDebugName(id) or ""
# Version strings often carry a build-tool suffix after a semicolon; keep the version itself.
version = name(5).split(";")[0].strip()
print("\t".join([name(1), name(2), version, "1" if "fvar" in font else "0"]))
'@ | Set-Content -Path $inspectScript -Encoding ascii
    $inspected = & $venvPython $inspectScript $FontPath
    if ($LASTEXITCODE -ne 0) { throw "Could not read the font's name table." }
    $family, $subfamily, $version, $isVariable = ($inspected | Select-Object -Last 1).Split("`t")

    # ImGui rasterizes with stb_truetype, which has no variable-font support: a variable file
    # compiles and embeds fine but renders at the wrong weight. Reject it here rather than let
    # it reach the binary.
    if ($isVariable -eq "1") {
        throw "$FontPath is a variable font. Use the static instance (ttf/static/ in the release zip)."
    }

    Write-Host "==> Subsetting to $Unicodes"
    $subsetPath = Join-Path $tempDir "$SymbolName-Subset.ttf"
    Invoke-Checked "Subsetting" {
        & $venvPython -m fontTools.subset $FontPath "--unicodes=$Unicodes" "--output-file=$subsetPath"
    }

    Write-Host "==> Building binary_to_compressed_c"
    $buildBat = Join-Path $tempDir "build-converter.bat"
    @"
@echo off
call "$vcvars" >nul 2>&1 || exit /b 1
cd /d "$tempDir" || exit /b 1
cl /nologo /O2 /EHsc "$converterSrc" /Fe:binary_to_compressed_c.exe >nul || exit /b 1
"@ | Set-Content -Path $buildBat -Encoding oem
    Invoke-Checked "Converter build" { & cmd.exe /c $buildBat }

    Write-Host "==> Converting to a compressed C array"
    # Run from the temp directory and pass a bare filename: the converter echoes its argument
    # into a comment, and a relative name keeps the generated header free of machine-specific
    # paths (and therefore byte-identical between runs).
    Push-Location $tempDir
    try {
        $body = & (Join-Path $tempDir "binary_to_compressed_c.exe") "$SymbolName-Subset.ttf" $SymbolName
    }
    finally {
        Pop-Location
    }
    if ($LASTEXITCODE -ne 0) { throw "Conversion failed (exit code $LASTEXITCODE)." }

    # The licence file kept beside the font, named per FONT.md. Only referenced when there is
    # exactly one, so a differently organised fonts/ directory degrades to the FONT.md pointer.
    $licences = @(Get-ChildItem -Path (Split-Path $FontPath) -Filter "LICENSE-*.txt" -File)
    $licenceLine = if ($licences.Count -eq 1) {
        "// Third-party font: see $(Get-RepoRelativePath $licences[0].FullName) for its licence, and FONT.md"
    }
    else {
        "// Third-party font: see FONT.md for its licence, and"
    }

    $preamble = @(
        "// Copyright (c) 2025 Max Godman",
        "",
        "// GENERATED FILE - DO NOT HAND-EDIT.",
        "//",
        "// $family $subfamily (static instance), $version, subset to $Unicodes and",
        "// stb-compressed for embedding.",
        "//",
        $licenceLine,
        "// for the obligations that come with shipping it.",
        "//",
        "// Regenerate with:",
        "//   powershell -NoProfile -File scripts/regenerate-font.ps1",
        "//",
        "// The array is owned by this translation unit, not by the font atlas:",
        "// AddFontFromMemoryCompressedTTF() does not take ownership of compressed data, so ``static const``",
        "// storage in a header is exactly right and nothing is ever freed.",
        "",
        "#pragma once",
        ""
    )

    $previous = if (Test-Path $OutputPath) { [System.IO.File]::ReadAllText($OutputPath) } else { $null }
    $header = (($preamble + $body) -join "`r`n") + "`r`n"
    [System.IO.File]::WriteAllText($OutputPath, $header, (New-Object System.Text.UTF8Encoding($false)))

    $compressed = [regex]::Match(($body -join "`n"), "compressed_size\s*=\s*(\d+)").Groups[1].Value
    $status = if ($previous -eq $header) { "unchanged" } else { "updated" }

    Write-Host ""
    Write-Host ("Source TTF   {0,10:N0} bytes  {1}" -f (Get-Item $FontPath).Length, [System.IO.Path]::GetFileName($FontPath))
    Write-Host ("Subset TTF   {0,10:N0} bytes  {1}" -f (Get-Item $subsetPath).Length, $Unicodes)
    Write-Host ("Embedded     {0,10:N0} bytes  stb-compressed" -f [int]$compressed)
    Write-Host ""
    Write-Host "$(Get-RepoRelativePath $OutputPath) $status. Rebuild to pick it up."
}
finally {
    Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}
