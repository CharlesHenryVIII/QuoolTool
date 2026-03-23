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
echo 02/11 Installed Programs...
::reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall" /s > "%OUTDIR%\programs.txt"

:: Define a temporary VBScript file
set "VBSFILE=%TEMP%\get_programs.vbs"

:: Write the VBScript code into the temp file line-by-line
> "%VBSFILE%" echo Const HKLM = ^&H80000002
>> "%VBSFILE%" echo Set oReg = GetObject("winmgmts:{impersonationLevel=impersonate}^!\\.\root\default:StdRegProv")
>> "%VBSFILE%" echo strKeyPath = "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
>> "%VBSFILE%" echo oReg.EnumKey HKLM, strKeyPath, arrSubKeys
>> "%VBSFILE%" echo WScript.Echo """DisplayName"",""DisplayVersion"""
>> "%VBSFILE%" echo If Not IsNull(arrSubKeys) Then
>> "%VBSFILE%" echo     For Each subkey In arrSubKeys
>> "%VBSFILE%" echo         strSubPath = strKeyPath ^& "\" ^& subkey
>> "%VBSFILE%" echo         oReg.GetStringValue HKLM, strSubPath, "DisplayName", strName
>> "%VBSFILE%" echo         oReg.GetStringValue HKLM, strSubPath, "DisplayVersion", strVersion
>> "%VBSFILE%" echo         If Not IsNull(strName) And strName ^<^> "" Then
>> "%VBSFILE%" echo             strName = Replace(strName, """", "")
>> "%VBSFILE%" echo             If IsNull(strVersion) Then strVersion = ""
>> "%VBSFILE%" echo             strVersion = Replace(strVersion, """", "")
>> "%VBSFILE%" echo             WScript.Echo """" ^& strName ^& """,""" ^& strVersion ^& """"
>> "%VBSFILE%" echo         End If
>> "%VBSFILE%" echo     Next
>> "%VBSFILE%" echo End If

:: Execute the VBScript using the native cscript engine and pipe the output to your CSV
cscript //nologo "%VBSFILE%" > "%OUTDIR%\programs.csv"

:: Delete the temporary script file to leave no trace
del "%VBSFILE%"

echo Done! Programs saved to %OUTDIR%\programs.csv


:: -----------------------------
:: Processor Info
:: -----------------------------
echo 03/11 CPU Info...
wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed /format:csv > "%OUTDIR%\processor.csv"

:: -----------------------------
:: System Info (OS + Computer + BIOS + Timezone)
:: -----------------------------
echo 04/11 System Info...
wmic computersystem get Name,Manufacturer,Model,SystemType,UserName,TotalPhysicalMemory /format:csv > "%OUTDIR%\computersystem.csv"

echo 05/11 OS Info...
wmic os get /format:csv > "%OUTDIR%\os.csv"

echo 06/11 BIOS Info...
wmic bios get Name,Manufacturer,SMBIOSBIOSVersion,SerialNumber,ReleaseDate /format:csv > "%OUTDIR%\bios.csv"

echo 07/11 Timezone Info...
wmic timezone get Caption,Bias /format:csv > "%OUTDIR%\timezone.csv"

:: -----------------------------
:: GPU Info
:: -----------------------------
echo 08/11 GPU Info...
wmic path Win32_VideoController get Name,DriverVersion,AdapterRAM,VideoProcessor,CurrentHorizontalResolution,CurrentVerticalResolution /format:csv > "%OUTDIR%\gpu.csv"

:: -----------------------------
:: Disk Info (Logical)
:: -----------------------------
echo 09/11 Disk Info (Logical Drives)...
wmic logicaldisk where "DriveType=3" get DeviceID,FileSystem,Size,FreeSpace,VolumeName /format:csv > "%OUTDIR%\logical_disks.csv"

:: -----------------------------
:: Physical Disk Info
:: -----------------------------
echo 10/11 Physical Disk Info...
::wmic diskdrive get Model,InterfaceType,Size,SerialNumber /format:csv > "%OUTDIR%\physical_disks.csv"
::wmic diskdrive get /format:csv > "%OUTDIR%\physical_disks.csv"

:: Define a temporary VBScript file
set "VBSFILE=%TEMP%\get_disks_safe.vbs"

:: Write the Byte Formatting Helper Function
> "%VBSFILE%" echo Function F(b)
>> "%VBSFILE%" echo If IsNull(b) Or b = "" Then F = "0 B" : Exit Function
>> "%VBSFILE%" echo Dim v : v = CDbl(b)
>> "%VBSFILE%" echo If v ^>= 1099511627776 Then
>> "%VBSFILE%" echo     F = Round(v/1099511627776, 2) ^& " TB"
>> "%VBSFILE%" echo ElseIf v ^>= 1073741824 Then
>> "%VBSFILE%" echo     F = Round(v/1073741824, 2) ^& " GB"
>> "%VBSFILE%" echo ElseIf v ^>= 1048576 Then
>> "%VBSFILE%" echo     F = Round(v/1048576, 2) ^& " MB"
>> "%VBSFILE%" echo Else
>> "%VBSFILE%" echo     F = v ^& " B"
>> "%VBSFILE%" echo End If
>> "%VBSFILE%" echo End Function

>> "%VBSFILE%" echo On Error Resume Next
>> "%VBSFILE%" echo Set wmi = GetObject("winmgmts:{impersonationLevel=impersonate}^!\\.\root\cimv2")

:: Loop 1: Physical Hardware
>> "%VBSFILE%" echo WScript.Echo "--- PHYSICAL DISKS ---"
>> "%VBSFILE%" echo WScript.Echo "DeviceID,Model,Interface,Size"
>> "%VBSFILE%" echo For Each d In wmi.ExecQuery("Select * From Win32_DiskDrive")
>> "%VBSFILE%" echo   modName = Replace(d.Model ^& "", ",", " ")
>> "%VBSFILE%" echo   WScript.Echo d.DeviceID ^& "," ^& modName ^& "," ^& d.InterfaceType ^& "," ^& F(d.Size)
>> "%VBSFILE%" echo Next

:: Loop 2: Windows Drive Letters
>> "%VBSFILE%" echo WScript.Echo ""
>> "%VBSFILE%" echo WScript.Echo "--- LOGICAL DRIVES ---"
>> "%VBSFILE%" echo WScript.Echo "DriveLetter,VolumeName,Size,FreeSpace"
>> "%VBSFILE%" echo For Each l In wmi.ExecQuery("Select * From Win32_LogicalDisk Where DriveType=3")
>> "%VBSFILE%" echo   volName = Replace(l.VolumeName ^& "", ",", " ")
>> "%VBSFILE%" echo   WScript.Echo l.DeviceID ^& "," ^& volName ^& "," ^& F(l.Size) ^& "," ^& F(l.FreeSpace)
>> "%VBSFILE%" echo Next

:: Execute the script and save the output
cscript //nologo "%VBSFILE%" > "%OUTDIR%\disks.csv"

:: Clean up the temp file
del "%VBSFILE%"

:: -----------------------------
:: IP Config
:: -----------------------------
echo 11/11 Network Info...
ipconfig /all > "%OUTDIR%\ipconfig.txt"

echo.
echo Done.
pause
