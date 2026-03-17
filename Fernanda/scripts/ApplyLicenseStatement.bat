@echo off
setlocal

set SRC_DIR=%~dp0..\src
set LICENSE_FILE=%~dp0..\docs\LicenseStatement.txt

if not exist "%LICENSE_FILE%" (
    echo License statement not found: %LICENSE_FILE%
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$utf8 = New-Object System.Text.UTF8Encoding $false;" ^
    "$license = [System.IO.File]::ReadAllText('%LICENSE_FILE%', $utf8);" ^
    "$license = $license.TrimEnd(\"`r\", \"`n\") + \"`r`n\";" ^
    "$files = Get-ChildItem -Path '%SRC_DIR%' -Recurse -Include '*.h','*.cpp','*.mm';" ^
    "$count = 0;" ^
    "foreach ($f in $files) {" ^
    "    $content = [System.IO.File]::ReadAllText($f.FullName, $utf8);" ^
    "    if ($content.StartsWith('/*')) {" ^
    "        $i = $content.IndexOf('*/');" ^
    "        if ($i -ge 0) { $content = $content.Substring($i + 2).TrimStart(\"`r\", \"`n\") }" ^
    "    }" ^
    "    [System.IO.File]::WriteAllText($f.FullName, ($license + \"`r`n\" + $content), $utf8);" ^
    "    Write-Host ('  Updated: ' + $f.Name);" ^
    "    $count++;" ^
    "}" ^
    "Write-Host \"`nDone. Updated $count file(s).\";"

pause
endlocal