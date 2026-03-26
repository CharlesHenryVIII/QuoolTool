@echo off
setlocal ENABLEDELAYEDEXPANSION


:: Get system name
set SYSNAME=%COMPUTERNAME%

:: Create output folder
set OUTDIR=%CD%\%SYSNAME%
mkdir "%OUTDIR%" 2>nul

echo Collecting data into %OUTDIR%
echo.

REM echo Windows XP 1 > "%OUTDIR%\type.txt"
set "SYS_FAMILY=Windows"
set "SYS_TYPE=XP"
set "SYS_VER=1"

:: Write the structured XML file
> "%OUTDIR%\quooltoolinfo.xml" echo ^<?xml version="1.0" encoding="UTF-8"?^>
>> "%OUTDIR%\quooltoolinfo.xml" echo ^<SystemIdentity^>
>> "%OUTDIR%\quooltoolinfo.xml" echo   ^<ComputerName^>%COMPUTERNAME%^</ComputerName^>
>> "%OUTDIR%\quooltoolinfo.xml" echo   ^<Family^>%SYS_FAMILY%^</Family^>
>> "%OUTDIR%\quooltoolinfo.xml" echo   ^<Type^>%SYS_TYPE%^</Type^>
>> "%OUTDIR%\quooltoolinfo.xml" echo   ^<Version^>%SYS_VER%^</Version^>
>> "%OUTDIR%\quooltoolinfo.xml" echo ^</SystemIdentity^>

:: -----------------------------
:: NETSTAT
:: -----------------------------
echo 01/11 Netstat...
netstat -ano > "%OUTDIR%\netstat.txt"

:: -----------------------------
:: Installed Programs
:: -----------------------------
echo 02/11 Gathering Installed Programs (This may take a moment)...
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
wmic os get /format:rawxml > "%OUTDIR%\os.xml"

echo 06/11 BIOS Info...
wmic bios get Name,Manufacturer,SMBIOSBIOSVersion,SerialNumber,ReleaseDate /format:rawxml > "%OUTDIR%\bios.xml"

echo 07/11 Timezone Info...
wmic timezone get Caption /format:rawxml > "%OUTDIR%\timezone.xml"

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
wmic diskdrive get Model,MediaType,Name,Description,Size,Manufacturer,Partitions,Status,InterfaceType,FirmwareRevision,DeviceID /format:rawxml > "%OUTDIR%\physical_disks.xml"

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
echo 11/12 Network Info...
REM ipconfig /all > "%OUTDIR%\ipconfig.txt"
REM wmic nicconfig where "IPEnabled='TRUE'" get Description,MACAddress,IPAddress,IPSubnet,DefaultIPGateway,DHCPServer,DNSServerSearchOrder /format:rawxml > "%OUTDIR%\network.xml"
REM wmic nicconfig get * /format:rawxml > "%OUTDIR%\network.xml"
wmic nicconfig get Description,IPEnabled,SettingID,DHCPEnabled,MACAddress /format:rawxml > "%OUTDIR%\networks.xml"

:: -----------------------------
:: Saved IP Configurations (Pure Batch XML)
:: -----------------------------
echo 12/12 Extracting Complete Network Profile...

:: 1. Write the standard XML header
> "%OUTDIR%\network_settings.xml" echo ^<?xml version="1.0" encoding="UTF-8"?^>
>> "%OUTDIR%\network_settings.xml" echo ^<NetworkInterfaces^>

:: 2. Loop through every network interface GUID
FOR /F "delims=" %%A IN ('reg query "HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces" ^| findstr "HKEY_"') DO (
    
    REM Extract just the {GUID} from the full registry path
    set "fullPath=%%A"
    set "guid=!fullPath:HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\=!"
    
    REM Reset all variables for this specific interface
    set "val_EnableDHCP="
    set "val_IPAddress="
    set "val_SubnetMask="
    set "val_DefaultGateway="
    set "val_DefaultGatewayMetric="
    set "val_DhcpIPAddress="
    set "val_DhcpSubnetMask="
    set "val_DhcpServer="
    set "val_DhcpNameServer="
    set "val_DhcpDefaultGateway="
    set "val_DhcpDomain="
    set "val_NameServer="
    set "val_Domain="

    REM 3. Query the entire folder at once. 
    REM findstr "REG_" ensures we only parse valid registry keys (Name, Type, Data)
    FOR /F "tokens=1,2,*" %%B IN ('reg query "%%A" 2^>nul ^| findstr "REG_"') DO (
        
        set "vName=%%B"
        set "vData=%%D"
        
        REM Clean up multiple IPs (REG_MULTI_SZ) separated by null characters (\0)
        if defined vData set "vData=!vData:\0=,!"

        REM Map the data to our XML variables
        if /I "!vName!"=="EnableDHCP" set "val_EnableDHCP=!vData!"
        if /I "!vName!"=="IPAddress" set "val_IPAddress=!vData!"
        if /I "!vName!"=="SubnetMask" set "val_SubnetMask=!vData!"
        if /I "!vName!"=="DefaultGateway" set "val_DefaultGateway=!vData!"
        if /I "!vName!"=="DefaultGatewayMetric" set "val_DefaultGatewayMetric=!vData!"
        if /I "!vName!"=="DhcpIPAddress" set "val_DhcpIPAddress=!vData!"
        if /I "!vName!"=="DhcpSubnetMask" set "val_DhcpSubnetMask=!vData!"
        if /I "!vName!"=="DhcpServer" set "val_DhcpServer=!vData!"
        if /I "!vName!"=="DhcpNameServer" set "val_DhcpNameServer=!vData!"
        if /I "!vName!"=="DhcpDefaultGateway" set "val_DhcpDefaultGateway=!vData!"
        if /I "!vName!"=="DhcpDomain" set "val_DhcpDomain=!vData!"
        if /I "!vName!"=="NameServer" set "val_NameServer=!vData!"
        if /I "!vName!"=="Domain" set "val_Domain=!vData!"
    )

    REM 4. Check if this is an actual configured interface. 
    REM If EnableDHCP exists (even if it is 0x0), it's a real network card.
    if defined val_EnableDHCP (
        
        >> "%OUTDIR%\network_settings.xml" echo   ^<Interface GUID="!guid!"^>
        
        REM Only write the XML tag if the value actually exists in the registry
        if defined val_EnableDHCP >> "%OUTDIR%\network_settings.xml" echo     ^<EnableDHCP^>!val_EnableDHCP!^</EnableDHCP^>
        if defined val_IPAddress >> "%OUTDIR%\network_settings.xml" echo     ^<IPAddress^>!val_IPAddress!^</IPAddress^>
        if defined val_SubnetMask >> "%OUTDIR%\network_settings.xml" echo     ^<SubnetMask^>!val_SubnetMask!^</SubnetMask^>
        if defined val_DefaultGateway >> "%OUTDIR%\network_settings.xml" echo     ^<DefaultGateway^>!val_DefaultGateway!^</DefaultGateway^>
        if defined val_DefaultGatewayMetric >> "%OUTDIR%\network_settings.xml" echo     ^<DefaultGatewayMetric^>!val_DefaultGatewayMetric!^</DefaultGatewayMetric^>
        if defined val_DhcpIPAddress >> "%OUTDIR%\network_settings.xml" echo     ^<DhcpIPAddress^>!val_DhcpIPAddress!^</DhcpIPAddress^>
        if defined val_DhcpSubnetMask >> "%OUTDIR%\network_settings.xml" echo     ^<DhcpSubnetMask^>!val_DhcpSubnetMask!^</DhcpSubnetMask^>
        if defined val_DhcpServer >> "%OUTDIR%\network_settings.xml" echo     ^<DhcpServer^>!val_DhcpServer!^</DhcpServer^>
        if defined val_DhcpNameServer >> "%OUTDIR%\network_settings.xml" echo     ^<DhcpNameServer^>!val_DhcpNameServer!^</DhcpNameServer^>
        if defined val_DhcpDefaultGateway >> "%OUTDIR%\network_settings.xml" echo     ^<DhcpDefaultGateway^>!val_DhcpDefaultGateway!^</DhcpDefaultGateway^>
        if defined val_DhcpDomain >> "%OUTDIR%\network_settings.xml" echo     ^<DhcpDomain^>!val_DhcpDomain!^</DhcpDomain^>
        if defined val_NameServer >> "%OUTDIR%\network_settings.xml" echo     ^<NameServer^>!val_NameServer!^</NameServer^>
        if defined val_Domain >> "%OUTDIR%\network_settings.xml" echo     ^<Domain^>!val_Domain!^</Domain^>
        
        >> "%OUTDIR%\network_settings.xml" echo   ^</Interface^>
    )
)

>> "%OUTDIR%\network_settings.xml" echo ^</NetworkInterfaces^>
echo Done! Network data saved to %OUTDIR%\network_settings.xml



echo.
echo Done.
pause
