# Reads ../src/Version.h, reconstructs VERSION_FULL_STRING, writes to ../VERSION

# param(
#     [Parameter(Mandatory)]
#     [string]$OutputDir
# )

# If we decide to use the param, can pass $(TargetDir) to pre-build command line

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$headerPath = Join-Path $scriptDir "..\src\Version.h"
# $outputPath = Join-Path $OutputDir "Version.txt"
$outputPath = Join-Path $scriptDir "..\..\x64\Release\Version.txt"

$headerPath = [System.IO.Path]::GetFullPath($headerPath)
$outputPath = [System.IO.Path]::GetFullPath($outputPath)

Write-Host "Reading: $headerPath"

if (-not (Test-Path $headerPath)) {
    Write-Host "ERROR: File not found: $headerPath"
    exit 1
}

$content = Get-Content $headerPath -Raw

$major = [regex]::Match($content, '#define\s+VERSION_MAJOR\s+(\d+)').Groups[1].Value
$minor = [regex]::Match($content, '#define\s+VERSION_MINOR\s+(\d+)').Groups[1].Value
$patch = [regex]::Match($content, '#define\s+VERSION_PATCH\s+(\d+)').Groups[1].Value
$pre   = [regex]::Match($content, '#define\s+VERSION_PRERELEASE_STRING\s+"([^"]+)"').Groups[1].Value
$hasPrerelease = [regex]::Match($content, '#define\s+VERSION_IS_PRERELEASE\s+(\d+)').Groups[1].Value -eq '1'

Write-Host "Parsed: MAJOR=$major MINOR=$minor PATCH=$patch PRE=$pre HAS_PRE=$hasPrerelease"

$version = "$major.$minor.$patch"
if ($hasPrerelease -and $pre) {
    $version += "-$pre"
}

# Set-Content $outputPath $version -NoNewline -Encoding UTF8
[System.IO.File]::WriteAllText($outputPath, $version)

Write-Host "Wrote '$version' to: $outputPath"
