@echo off
set CF="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"

for /r "%~dp0..\src" %%f in (*.h *.cpp *.mm) do (
    %CF% -i "%%f"
)

for /r "%~dp0..\resources" %%f in (*.rc) do (
    %CF% -i "%%f"
)

for /r "%~dp0..\tests" %%f in (*.cpp) do (
    %CF% -i "%%f"
)