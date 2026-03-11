@echo off
set CF="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\bin\clang-format.exe"

for /r "%~dp0..\src" %%f in (*.h *.cpp) do (
    %CF% -i "%%f"
)