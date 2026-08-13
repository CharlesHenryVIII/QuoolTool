@echo off
pushd %~dp0

REM --- SET YOUR PROJECT NAME HERE ---
set "PROJECT_NAME=QuoolTool"
REM ----------------------------------

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
call make\windows\premake5.exe ecc

set "COMPILETAB=    "
echo %BAR%
echo %ROOTTAB% %COMPILETAB% COMPILING:
echo %BAR%

if not exist "%PROJECT_NAME%.slnx" (
    msbuild /t:%PROJECT_NAME% /nologo /verbosity:minimal -p:Configuration=Debug %PROJECT_NAME%.sln
) else (
    msbuild /t:%PROJECT_NAME% /nologo /verbosity:minimal -p:Configuration=Debug %PROJECT_NAME%.slnx
)


popd

