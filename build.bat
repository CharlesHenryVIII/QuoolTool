@echo off

REM --- SET YOUR PROJECT NAME HERE ---
set "PROJECT_NAME=QuoolTool"
REM ----------------------------------

pushd %~dp0

REM if "%SHELLSETUP%"=="" (
REM     call shell.bat
REM ) ELSE (
REM     echo Skipping setup
REM )

REM echo premake5.exe --file=premake5.lua vs2015
REM premake5.exe --file=premake5.lua vs2015

set "ROOTTAB=  "
set BAR===============================

set "PRJFILESTAB="
echo %BAR%
echo %ROOTTAB% %PRJFILESTAB% GENERATE PROJECT FILES:
echo %BAR%
call GenerateProjectFiles.bat

set "COMMANDSTAB="
echo %BAR%
echo %ROOTTAB% %COMMANDSTAB% GENERATE BUILD COMMANDS:
echo %BAR%
call make\premake5.exe ecc

set "COMPILETAB=    "
echo %BAR%
echo %ROOTTAB% %COMPILETAB% GENERATE EXE:
echo %BAR%

if not exist "%PROJECT_NAME%.slnx" (
    msbuild /t:%PROJECT_NAME% /nologo /verbosity:minimal -p:Configuration=Debug %PROJECT_NAME%.sln
) else (
    msbuild /t:%PROJECT_NAME% /nologo /verbosity:minimal -p:Configuration=Debug %PROJECT_NAME%.slnx
)


popd

