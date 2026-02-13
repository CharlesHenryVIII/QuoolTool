@echo off

pushd %~dp0

REM if "%SHELLSETUP%"=="" (
REM     call shell.bat
REM ) ELSE (
REM     echo Skipping setup
REM )

REM echo premake5.exe --file=premake5.lua vs2015
REM premake5.exe --file=premake5.lua vs2015

echo GENERATE PROJECT FILES
call GenerateProjectFiles.bat
echo GENERATE BUILD COMMANDS
call make\premake5.exe ecc
echo GENERATE EXE
msbuild /t:ScadaBackup /nologo /verbosity:minimal -p:Configuration=Debug ScadaBackup.slnx


popd

