@echo off
pushd %~dp0

set "SHADER_DIR=contrib\CashUtil\include\Shaders"
set "SHDC=contrib\sokol-tools-bin\bin\win32\sokol-shdc.exe"

for %%f in ("%SHADER_DIR%\*.glsl") do (
    echo    Compiling Shader %%~nf
    call "%SHDC%" --input "%%f" --output "%SHADER_DIR%\%%~nf.h" --slang glsl430:hlsl5:metal_macos --format sokol_impl --reflection
)

popd
