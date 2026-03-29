@echo off
setlocal ENABLEDELAYEDEXPANSION


:: Get system name
set SYSNAME=%COMPUTERNAME%

:: Create output folder
set OUTDIR=%CD%\%SYSNAME%
mkdir "%OUTDIR%" 2>nul

set "TOTAL_STEPS=12"
set "STEP_COUNT=0"

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

call :LogStep "Netstat..."
netstat -ano > "%OUTDIR%\netstat.txt"

:: -----------------------------
:: Installed Programs
:: -----------------------------
call :LogStep "Gathering Installed Programs (Optimized)..."

REM Initialize the XML file
> "%OUTDIR%\programs.xml" echo ^<?xml version="1.0" encoding="UTF-8"?^>
>> "%OUTDIR%\programs.xml" echo ^<Programs^>

set "appName="
set "appVer="

REM Step 1: Run ONE query (/s) and parse the text output in memory
REM tokens 1=Name (DisplayName), 2=Type (REG_SZ), 3*=Value (The actual program name)
FOR /F "tokens=1,2,*" %%A IN ('reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall" /s 2^>nul') DO (
    
    set "key=%%A"
    
    REM Step 2: If the line starts with HKEY_, we entered a new program's folder
    if /I "!key:~0,5!"=="HKEY_" (
        
        REM If the previous folder had a DisplayName, write it out before moving on!
        if defined appName call :WriteProgramXML
        
        REM Reset variables for this newly discovered folder
        set "appName="
        set "appVer="
        
    ) else if /I "!key!"=="DisplayName" (
        REM Step 3: Capture the DisplayName
        set "appName=%%C"
    ) else if /I "!key!"=="DisplayVersion" (
        REM Step 4: Capture the DisplayVersion
        set "appVer=%%C"
    )
)

REM Step 5: Catch the very last program in the list 
REM (Because there is no subsequent HKEY_ line to trigger the final write)
if defined appName call :WriteProgramXML

REM Close the main XML tag and skip over the helper function
>> "%OUTDIR%\programs.xml" echo ^</Programs^>
goto :DoneWithPrograms


:: -----------------------------------------
:: Inline Helper Function for XML Escaping
:: -----------------------------------------
:WriteProgramXML
REM Safely escape illegal XML characters
set "safeName=!appName:&=&amp;!"
set "safeName=!safeName:<=&lt;!"
set "safeName=!safeName:>=&gt;!"

set "safeVer="
if defined appVer (
    set "safeVer=!appVer:&=&amp;!"
    set "safeVer=!safeVer:<=&lt;!"
    set "safeVer=!safeVer:>=&gt;!"
)

REM Write the formatted XML block
>> "%OUTDIR%\programs.xml" echo   ^<Program^>
>> "%OUTDIR%\programs.xml" echo     ^<DisplayName^>!safeName!^</DisplayName^>
>> "%OUTDIR%\programs.xml" echo     ^<DisplayVersion^>!safeVer!^</DisplayVersion^>
>> "%OUTDIR%\programs.xml" echo   ^</Program^>

goto :EOF
:: -----------------------------------------


:DoneWithPrograms






call :LogStep "CPU Info..."
wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed /format:rawxml > "%OUTDIR%\processor.xml"

call :LogStep "System Info..."
wmic computersystem get Name,Manufacturer,Model,SystemType,UserName,TotalPhysicalMemory /format:rawxml > "%OUTDIR%\computersystem.xml"

call :LogStep "OS Info..."
wmic os get /format:rawxml > "%OUTDIR%\os.xml"

call :LogStep "BIOS Info..."
wmic bios get Name,Manufacturer,SMBIOSBIOSVersion,SerialNumber,ReleaseDate /format:rawxml > "%OUTDIR%\bios.xml"

call :LogStep "Timezone Info..."
wmic timezone get Caption /format:rawxml > "%OUTDIR%\timezone.xml"

call :LogStep "GPU Info..."
wmic path Win32_VideoController get Name,DriverVersion,AdapterRAM,VideoProcessor,CurrentHorizontalResolution,CurrentVerticalResolution /format:rawxml > "%OUTDIR%\gpu.xml"

call :LogStep "Logical Disk Info..."
wmic logicaldisk get /format:rawxml > "%OUTDIR%\logical_disks.xml"

call :LogStep "Physical Disk Info..."
wmic diskdrive get Model,MediaType,Name,Description,Size,Manufacturer,Partitions,Status,InterfaceType,FirmwareRevision,DeviceID /format:rawxml > "%OUTDIR%\physical_disks.xml"

call :LogStep "Network Info..."
wmic nicconfig get Description,IPEnabled,SettingID,DHCPEnabled,MACAddress /format:rawxml > "%OUTDIR%\networks.xml"

:: -----------------------------
:: Network Adapters
:: -----------------------------
call :LogStep "Extracting Network Adapters..."

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


REM -----------------------------
REM Helper Functions
REM -----------------------------
:LogStep
set /a STEP_COUNT+=1

if !STEP_COUNT! LSS 10 (
    set "PAD=0!STEP_COUNT!"
) else (
    set "PAD=!STEP_COUNT!"
)

REM Slice the string to only keep the last 2 characters (!PAD:~-2!)
REM This turns "01" into "01", and "010" into "10"
echo !PAD:~-2!/%TOTAL_STEPS% %~1

goto :EOF



echo.
echo Done.
pause
