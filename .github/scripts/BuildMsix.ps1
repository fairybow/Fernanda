# Builds Fernanda's MSIX package for the Microsoft Store from a deployment: a
# folder holding Fernanda.exe and what windeployqt put beside it. Run it from a
# Visual Studio Developer PowerShell, which puts the Windows SDK's makeappx and
# makepri on PATH and sets VCToolsRedistDir.
#
# Writes <Output>\layout (the package's files) and the .msix beside it. The
# package is unsigned: Partner Center signs it on submission. To run it locally,
# turn on Developer Mode and register the layout in place:
#
#   Add-AppxPackage -Register <Output>\layout\AppxManifest.xml
#
# and remove it with:
#
#   Get-AppxPackage fairybow.Fernanda | Remove-AppxPackage

param(
    [Parameter(Mandatory)] [string] $Deployment,
    # Major.Minor.Patch, without a prerelease label
    [Parameter(Mandatory)] [string] $Version,
    [Parameter(Mandatory)] [string] $Output
)

$ErrorActionPreference = 'Stop'

# The Store takes only four numbers ending in 0, so the prerelease label has no
# place in the package's version
if ($Version -notmatch '^\d+\.\d+\.\d+$') {
    throw "Version must be Major.Minor.Patch, not '$Version'"
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$msixDir = Join-Path $repo '.github\msix'
$iconsDir = Join-Path $repo 'Fernanda\resources\icons'

$Deployment = (Resolve-Path $Deployment).Path
if (-not (Test-Path (Join-Path $Deployment 'Fernanda.exe'))) {
    throw "No Fernanda.exe in $Deployment"
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null
$Output = (Resolve-Path $Output).Path
$layout = Join-Path $Output 'layout'
$assets = Join-Path $layout 'Assets'

function Find-Tool([string] $Name) {
    $command = Get-Command "$Name.exe" -ErrorAction SilentlyContinue
    if (-not $command) {
        throw "$Name.exe not found. Run this from a Visual Studio Developer PowerShell"
    }
    return $command.Source
}

function Invoke-Tool([string] $Tool, [string[]] $Arguments) {
    & $Tool @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$(Split-Path -Leaf $Tool) exited with $LASTEXITCODE"
    }
}

$makeappx = Find-Tool 'makeappx'
$makepri = Find-Tool 'makepri'

# --- Files ---

# NB: A layout registered with Add-AppxPackage -Register runs from this folder,
# so it can't be cleared while that package is installed
if (Test-Path $layout) {
    Remove-Item -Recurse -Force $layout
}
New-Item -ItemType Directory -Path $assets | Out-Null

# The installer runs vc_redist itself; a package can't run an installer
Copy-Item -Path (Join-Path $Deployment '*') -Destination $layout -Recurse `
    -Exclude 'vc_redist*.exe'

# The C++ runtime goes beside the exe, where Windows looks for a DLL first, so
# the runtime that loads is the one Fernanda and Qt were built against, not
# whatever version of Microsoft's runtime package the machine has
if (-not $env:VCToolsRedistDir) {
    throw "VCToolsRedistDir isn't set. Run this from a Visual Studio Developer PowerShell"
}
$crt = @(Get-ChildItem -Directory -Path (Join-Path $env:VCToolsRedistDir 'x64') `
    -Filter 'Microsoft.VC*.CRT')
if ($crt.Count -ne 1) {
    throw "Expected one Microsoft.VC*.CRT folder in $env:VCToolsRedistDir\x64, found $($crt.Count)"
}
Copy-Item -Path (Join-Path $crt[0].FullName '*.dll') -Destination $layout

# --- Manifest ---

$manifest = [IO.File]::ReadAllText((Join-Path $msixDir 'AppxManifest.xml.in'))
$manifest = $manifest.Replace('@VERSION@', "$Version.0")
[IO.File]::WriteAllText((Join-Path $layout 'AppxManifest.xml'), $manifest)

# --- Assets ---

# The tiles, the Store logo, and the scaled app icon come from MakeMsixAssets.ps1
Copy-Item -Path (Join-Path $msixDir 'Assets\*.png') -Destination $assets

# The app icon at fixed pixel sizes is the app's own icon at those sizes. The
# unplated forms are what the taskbar and Start show on dark and light
# backgrounds; without them, Windows draws the icon on a colored square
foreach ($size in 16, 24, 32, 48, 256) {
    $png = Join-Path $iconsDir "Fernanda-$size.png"
    foreach ($form in '', '_altform-unplated', '_altform-lightunplated') {
        Copy-Item -Path $png `
            -Destination (Join-Path $assets "Square44x44Logo.targetsize-$size$form.png")
    }
}

# --- Package ---

# resources.pri is the index Windows uses to pick among the assets' variants.
# Its config and the index itself are written outside the layout, so makepri
# doesn't index its own files, and only the index is moved in.
#
# NB: makepri's default config splits each scale into its own .pri, for the
# resource packages of an .msixbundle. This is a single package, where Windows
# reads only resources.pri, so the split would hide every scale-200 asset. The
# config's packaging element is what asks for the split, so it is removed
$priConfig = Join-Path $Output 'priconfig.xml'
$pri = Join-Path $Output 'resources.pri'
Invoke-Tool $makepri @('createconfig', '/cf', $priConfig, '/dq', 'en-US', '/pv', '10.0.0', '/o')

$config = [xml]([IO.File]::ReadAllText($priConfig))
$packaging = $config.SelectSingleNode('/resources/packaging')
if ($packaging) {
    $packaging.ParentNode.RemoveChild($packaging) | Out-Null
}
$config.Save($priConfig)

Invoke-Tool $makepri @(
    'new',
    '/pr', $layout,
    '/cf', $priConfig,
    '/mn', (Join-Path $layout 'AppxManifest.xml'),
    '/of', $pri,
    '/o'
)
Move-Item -Path $pri -Destination $layout

$msix = Join-Path $Output "Fernanda_${Version}_Windows-x64.msix"
Invoke-Tool $makeappx @('pack', '/d', $layout, '/p', $msix, '/o')

Write-Host "Built $msix"
