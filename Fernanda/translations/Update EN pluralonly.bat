@echo off

set LUPDATE=C:\Qt\6.10.1\msvc2022_64\bin\lupdate.exe
set SOURCE=C:\Dev\Fernanda
set TS_FILE=.\Translation_en.ts

"%LUPDATE%" "%SOURCE%" -ts "%TS_FILE%" -pluralonly

if %ERRORLEVEL% neq 0 (
    echo lupdate failed with error %ERRORLEVEL%
) else (
    echo Done. Output: %TS_FILE%
)

pause