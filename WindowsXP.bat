@echo off
setlocal ENABLEDELAYEDEXPANSION


:: Get system name
set SYSNAME=%COMPUTERNAME%

:: Create output folder
set OUTDIR=%CD%\%SYSNAME%
mkdir "%OUTDIR%" 2>nul

echo Collecting data into %OUTDIR%
echo.

echo Windows XP 1 > "%OUTDIR%\type.txt"

:: -----------------------------
:: NETSTAT
:: -----------------------------
echo 01/11 Netstat...
netstat -ano > "%OUTDIR%\netstat.txt"

:: -----------------------------
:: Installed Programs
:: -----------------------------
echo 02/11 Gathering Installed Programs (THis may take a moment)...
::reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall" /s > "%OUTDIR%\programs.txt"

REM Initialize the XML file with standard headers
> "%OUTDIR%\programs.xml" echo ^<?xml version="1.0" encoding="UTF-8"?^>
>> "%OUTDIR%\programs.xml" echo ^<Programs^>

REM Step 1: Loop through all subkeys in the Uninstall registry path
FOR /F "delims=" %%A IN ('reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall" ^| findstr "HKEY_"') DO (
    
    set "appName="
    set "appVer="
    
REM Step 2: Query the DisplayName. (Tokens 1=DisplayName, 2=REG_SZ, 3*=The actual name)
    FOR /F "tokens=2,*" %%B IN ('reg query "%%A" /v DisplayName 2^>nul ^| findstr /I "DisplayName"') DO (
        set "appName=%%C"
    )
    
REM Step 3: If the program has a DisplayName, get the version and write the XML block
    if defined appName (
        
REM Get DisplayVersion (Silently hide errors if a program doesn't have a version)
        FOR /F "tokens=2,*" %%B IN ('reg query "%%A" /v DisplayVersion 2^>nul ^| findstr /I "DisplayVersion"') DO (
            set "appVer=%%C"
        )
        
REM Step 4: Safely escape illegal XML characters (&, <, >)
REM Because this is inside quotes, the batch interpreter won't crash on the & symbol
        set "appName=!appName:&=&amp;!"
        set "appName=!appName:<=&lt;!"
        set "appName=!appName:>=&gt;!"
        
        if defined appVer (
            set "appVer=!appVer:&=&amp;!"
            set "appVer=!appVer:<=&lt;!"
            set "appVer=!appVer:>=&gt;!"
        )
        
REM Step 5: Write the formatted XML block to the file
        >> "%OUTDIR%\programs.xml" echo   ^<Program^>
        >> "%OUTDIR%\programs.xml" echo     ^<DisplayName^>!appName!^</DisplayName^>
        >> "%OUTDIR%\programs.xml" echo     ^<DisplayVersion^>!appVer!^</DisplayVersion^>
        >> "%OUTDIR%\programs.xml" echo   ^</Program^>
    )
)

REM Close the main XML tag
>> "%OUTDIR%\programs.xml" echo ^</Programs^>



:: -----------------------------
:: Processor Info
:: -----------------------------
echo 03/11 CPU Info...
wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed /format:rawxml > "%OUTDIR%\processor.xml"

:: -----------------------------
:: System Info (OS + Computer + BIOS + Timezone)
:: -----------------------------
echo 04/11 System Info...
wmic computersystem get Name,Manufacturer,Model,SystemType,UserName,TotalPhysicalMemory /format:rawxml > "%OUTDIR%\computersystem.xml"

echo 05/11 OS Info...
wmic os get /format:csv > "%OUTDIR%\os.csv"

echo 06/11 BIOS Info...
wmic bios get Name,Manufacturer,SMBIOSBIOSVersion,SerialNumber,ReleaseDate /format:rawxml > "%OUTDIR%\bios.xml"

echo 07/11 Timezone Info...
wmic timezone get Caption,Bias /format:rawxml > "%OUTDIR%\timezone.xml"

:: -----------------------------
:: GPU Info
:: -----------------------------
echo 08/11 GPU Info...
wmic path Win32_VideoController get Name,DriverVersion,AdapterRAM,VideoProcessor,CurrentHorizontalResolution,CurrentVerticalResolution /format:rawxml > "%OUTDIR%\gpu.xml"

:: -----------------------------
:: Disk Info (Logical)
:: -----------------------------
echo 09/11 Disk Info (Logical Drives)...
::wmic logicaldisk where "DriveType=3" get DeviceID,FileSystem,Size,FreeSpace,VolumeName /format:rawxml > "%OUTDIR%\logical_disks.xml"
wmic logicaldisk get /format:rawxml > "%OUTDIR%\logical_disks.xml"

:: -----------------------------
:: Physical Disk Info
:: -----------------------------
echo 10/11 Physical Disk Info...
::wmic diskdrive get Model,InterfaceType,Size,SerialNumber /format:csv > "%OUTDIR%\physical_disks.csv"
wmic diskdrive get /format:rawxml > "%OUTDIR%\physical_disks.xml"

REM :: Define a temporary VBScript file
REM set "VBSFILE=%TEMP%\get_disks_safe.vbs"
REM 
REM :: Write the Byte Formatting Helper Function
REM > "%VBSFILE%" echo Function F(b)
REM >> "%VBSFILE%" echo If IsNull(b) Or b = "" Then F = "0 B" : Exit Function
REM >> "%VBSFILE%" echo Dim v : v = CDbl(b)
REM >> "%VBSFILE%" echo If v ^>= 1099511627776 Then
REM >> "%VBSFILE%" echo     F = Round(v/1099511627776, 2) ^& " TB"
REM >> "%VBSFILE%" echo ElseIf v ^>= 1073741824 Then
REM >> "%VBSFILE%" echo     F = Round(v/1073741824, 2) ^& " GB"
REM >> "%VBSFILE%" echo ElseIf v ^>= 1048576 Then
REM >> "%VBSFILE%" echo     F = Round(v/1048576, 2) ^& " MB"
REM >> "%VBSFILE%" echo Else
REM >> "%VBSFILE%" echo     F = v ^& " B"
REM >> "%VBSFILE%" echo End If
REM >> "%VBSFILE%" echo End Function
REM 
REM >> "%VBSFILE%" echo On Error Resume Next
REM >> "%VBSFILE%" echo Set wmi = GetObject("winmgmts:{impersonationLevel=impersonate}^!\\.\root\cimv2")
REM 
REM :: Loop 1: Physical Hardware
REM >> "%VBSFILE%" echo WScript.Echo "--- PHYSICAL DISKS ---"
REM >> "%VBSFILE%" echo WScript.Echo "DeviceID,Model,Interface,Size"
REM >> "%VBSFILE%" echo For Each d In wmi.ExecQuery("Select * From Win32_DiskDrive")
REM >> "%VBSFILE%" echo   modName = Replace(d.Model ^& "", ",", " ")
REM >> "%VBSFILE%" echo   WScript.Echo d.DeviceID ^& "," ^& modName ^& "," ^& d.InterfaceType ^& "," ^& F(d.Size)
REM >> "%VBSFILE%" echo Next
REM 
REM :: Loop 2: Windows Drive Letters
REM >> "%VBSFILE%" echo WScript.Echo ""
REM >> "%VBSFILE%" echo WScript.Echo "--- LOGICAL DRIVES ---"
REM >> "%VBSFILE%" echo WScript.Echo "DriveLetter,VolumeName,Size,FreeSpace"
REM >> "%VBSFILE%" echo For Each l In wmi.ExecQuery("Select * From Win32_LogicalDisk Where DriveType=3")
REM >> "%VBSFILE%" echo   volName = Replace(l.VolumeName ^& "", ",", " ")
REM >> "%VBSFILE%" echo   WScript.Echo l.DeviceID ^& "," ^& volName ^& "," ^& F(l.Size) ^& "," ^& F(l.FreeSpace)
REM >> "%VBSFILE%" echo Next
REM 
REM :: Execute the script and save the output
REM cscript //nologo "%VBSFILE%" > "%OUTDIR%\disks.csv"
REM 
REM :: Clean up the temp file
REM del "%VBSFILE%"

:: -----------------------------
:: IP Config
:: -----------------------------
echo 11/11 Network Info...
ipconfig /all > "%OUTDIR%\ipconfig.txt"

echo.
echo Done.
pause
