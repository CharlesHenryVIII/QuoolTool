@echo off
setlocal ENABLEDELAYEDEXPANSION


:: Get system name
set SYSNAME=%COMPUTERNAME%

:: Create output folder
set OUTDIR=%CD%\%SYSNAME%
mkdir "%OUTDIR%" 2>nul

echo Collecting data into %OUTDIR%
echo.

echo WindowsXP 1 > "%OUTDIR%\type.txt"

:: -----------------------------
:: NETSTAT
:: -----------------------------
echo 1/8 Netstat...
netstat -ano > "%OUTDIR%\netstat.txt"

:: -----------------------------
:: Installed Programs
:: -----------------------------
echo 2/8 Installed Programs...
reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall" /s > "%OUTDIR%\programs.txt"

:: -----------------------------
:: Processor Info
:: -----------------------------
echo 3/8 CPU Info...
wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed /format:csv > "%OUTDIR%\processor.csv"

:: -----------------------------
:: System Info (OS + Computer + BIOS + Timezone)
:: -----------------------------
echo 4/8 System Info...

wmic computersystem get Name,Manufacturer,Model,SystemType,UserName,TotalPhysicalMemory /format:csv > "%OUTDIR%\computersystem.csv"

wmic os get Caption,Version,BuildNumber,OSArchitecture,OSType,SystemDirectory,WindowsDirectory,Locale,OSLanguage,InstallDate,LastBootUpTime /format:csv > "%OUTDIR%\os.csv"

wmic bios get Name,Manufacturer,SMBIOSBIOSVersion,SerialNumber,ReleaseDate /format:csv > "%OUTDIR%\bios.csv"

wmic timezone get Caption,Bias /format:csv > "%OUTDIR%\timezone.csv"

:: -----------------------------
:: GPU Info
:: -----------------------------
echo 5/8 GPU Info...
wmic path Win32_VideoController get Name,DriverVersion,AdapterRAM,VideoProcessor,CurrentHorizontalResolution,CurrentVerticalResolution /format:csv > "%OUTDIR%\gpu.csv"

:: -----------------------------
:: Disk Info (Logical)
:: -----------------------------
echo 6/8 Disk Info (Logical Drives)...
wmic logicaldisk where "DriveType=3" get DeviceID,FileSystem,Size,FreeSpace,VolumeName /format:csv > "%OUTDIR%\logical_disks.csv"

:: -----------------------------
:: Physical Disk Info
:: -----------------------------
echo 7/8 Physical Disk Info...
wmic diskdrive get Model,InterfaceType,Size,SerialNumber /format:csv > "%OUTDIR%\physical_disks.csv"

:: -----------------------------
:: IP Config
:: -----------------------------
echo 8/8 Network Info...
ipconfig /all > "%OUTDIR%\ipconfig.txt"

echo.
echo Done.
pause
