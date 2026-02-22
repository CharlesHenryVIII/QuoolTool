@echo off
setlocal EnableExtensions EnableDelayedExpansion
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist %VSWHERE% (
    echo ERROR: vswhere.exe not found.
    echo Please install Visual Studio Installer.
    exit /b 1
)

set VS_MAJOR=

for /f "usebackq delims=" %%V in (`
    %VSWHERE% -latest -products * -requires Microsoft.Component.MSBuild -property installationVersion
`) do (
    set VS_VERSION=%%V
)

if not defined VS_VERSION (
    echo ERROR: No supported Visual Studio installation found.
    exit /b 1
)

REM Extract major version (17.x, 16.x, etc.)
for /f "delims=. tokens=1" %%M in ("%VS_VERSION%") do (
    set VS_MAJOR=%%M
)

set PREMAKE_ACTION=

if "%VS_MAJOR%"=="18" set PREMAKE_ACTION=vs2026
if "%VS_MAJOR%"=="17" set PREMAKE_ACTION=vs2022
if "%VS_MAJOR%"=="16" set PREMAKE_ACTION=vs2019
if "%VS_MAJOR%"=="15" set PREMAKE_ACTION=vs2017
if "%VS_MAJOR%"=="14" set PREMAKE_ACTION=vs2015
if "%VS_MAJOR%"=="13" set PREMAKE_ACTION=vs2013
if "%VS_MAJOR%"=="12" set PREMAKE_ACTION=vs2012

if not defined PREMAKE_ACTION (
    echo ERROR: Visual Studio %VS_VERSION% is not supported by Premake.
    exit /b 1
)

make\premake5.exe %PREMAKE_ACTION%
if errorlevel 1 exit /b %errorlevel%

endlocal
