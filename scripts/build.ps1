# Builds GammaHotkey from the command line (used by the VS Code build tasks).
# Locates MSBuild via vswhere so it works regardless of Visual Studio edition or install path.

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64",

    [ValidateSet("Build", "Rebuild", "Clean")]
    [string]$Target = "Build"
)

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found. Is Visual Studio installed?"
    exit 1
}

$msbuild = & $vswhere -latest -prerelease -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) {
    Write-Error "MSBuild not found via vswhere. Is the MSBuild component installed?"
    exit 1
}

$project = Join-Path $PSScriptRoot "..\GammaHotkey.vcxproj"
& $msbuild $project "/p:Configuration=$Configuration" "/p:Platform=$Platform" "/t:$Target" /m /v:minimal /nologo
exit $LASTEXITCODE
