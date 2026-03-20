# Get system name
$SystemName = $env:COMPUTERNAME

# Create output directory
$OutDir = Join-Path -Path (Get-Location) -ChildPath $SystemName
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

# Helper function for clean CSV (removes quotes like your macro)
function Export-CleanCsv {
    param($Data, $Path)
    $Data | ConvertTo-Csv -NoTypeInformation |
        ForEach-Object { $_ -replace '"' , '' } |
        Set-Content $Path
}

function Format-Bytes {
    param($bytes)
    if ($bytes -ge 1TB) { return "{0:N2} TB" -f ($bytes / 1TB) }
    elseif ($bytes -ge 1GB) { return "{0:N2} GB" -f ($bytes / 1GB) }
    elseif ($bytes -ge 1MB) { return "{0:N2} MB" -f ($bytes / 1MB) }
    else { return "$bytes B" }
}


"Windows7 1" | Set-Content (Join-Path $OutDir "type.txt")


# -----------------------------
# TCP Connections (netstat fallback)
# -----------------------------
Write-Host "1/6 Gathering TCP Connections..."
$tcp = netstat -ano -p tcp | Select-Object -Skip 4 | ForEach-Object {
    $parts = ($_ -split '\s+') | Where-Object { $_ -ne '' }
    if ($parts.Count -ge 5) {
        $procId = $parts[4]
        $proc = (Get-WmiObject Win32_Process -Filter "ProcessId=$procId" -ErrorAction SilentlyContinue).Name
        [PSCustomObject]@{
            LocalAddress  = ($parts[1] -split ':')[0]
            LocalPort     = ($parts[1] -split ':')[-1]
            RemoteAddress = ($parts[2] -split ':')[0]
            RemotePort    = ($parts[2] -split ':')[-1]
            State         = $parts[3]
            OwningProcess = $procId
            Process       = $proc
        }
    }
}
Export-CleanCsv $tcp (Join-Path $OutDir "netstat_tcp.csv")

# -----------------------------
# UDP Endpoints (netstat fallback)
# -----------------------------
Write-Host "2/6 Gathering UDP Endpoints..."
$udp = netstat -ano -p udp | Select-Object -Skip 4 | ForEach-Object {
    $parts = ($_ -split '\s+') | Where-Object { $_ -ne '' }
    if ($parts.Count -ge 4) {
        $procId = $parts[3]
        $proc = (Get-WmiObject Win32_Process -Filter "ProcessId=$procId" -ErrorAction SilentlyContinue).Name
        [PSCustomObject]@{
            LocalAddress  = ($parts[1] -split ':')[0]
            LocalPort     = ($parts[1] -split ':')[-1]
            OwningProcess = $procId
            Process       = $proc
        }
    }
}
Export-CleanCsv $udp (Join-Path $OutDir "netstat_udp.csv")

# -----------------------------
# Installed Programs (registry)
# -----------------------------
Write-Host "3/6 Gathering Installed Programs..."
$programs = Get-ItemProperty "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*" |
    Where-Object { $_.DisplayName } |
    Select-Object DisplayName, DisplayVersion

Export-CleanCsv $programs (Join-Path $OutDir "programs.csv")

# -----------------------------
# Processor Info (WMI instead of CIM)
# -----------------------------
Write-Host "4/6 Gathering Processor Info..."
$cpu = Get-WmiObject Win32_Processor |
    Select-Object Name,
                  NumberOfCores,
                  NumberOfLogicalProcessors,
                  MaxClockSpeed

Export-CleanCsv $cpu (Join-Path $OutDir "processor.csv")

# -----------------------------
# System Info (WMI replacement for Get-ComputerInfo)
# -----------------------------
Write-Host "5/6 Gathering System Info..."

$os   = Get-WmiObject Win32_OperatingSystem
$cs   = Get-WmiObject Win32_ComputerSystem
$bios = Get-WmiObject Win32_BIOS
$tz   = Get-WmiObject Win32_TimeZone
$cpu  = Get-WmiObject Win32_Processor | Select-Object -First 1

# Convert WMI datetime
function Convert-WmiDate {
    param($wmiDate)
    if ($wmiDate) {
        return [System.Management.ManagementDateTimeConverter]::ToDateTime($wmiDate)
    }
    return $null
}

$sysinfo = @(
    # Computer System
    [PSCustomObject]@{ Name="CsName"; Value=$cs.Name }
    [PSCustomObject]@{ Name="CsModel"; Value=$cs.Model }
    [PSCustomObject]@{ Name="CsManufacturer"; Value=$cs.Manufacturer }
    [PSCustomObject]@{ Name="CsSystemType"; Value=$cs.SystemType }
    [PSCustomObject]@{ Name="CsUserName"; Value=$cs.UserName }
    [PSCustomObject]@{ Name="CsTotalPhysicalMemory"; Value=(Format-Bytes $cs.TotalPhysicalMemory) }

    # OS Info
    [PSCustomObject]@{ Name="OsName"; Value=$os.Caption }
    [PSCustomObject]@{ Name="OsVersion"; Value=$os.Version }
    [PSCustomObject]@{ Name="OsBuildNumber"; Value=$os.BuildNumber }
    [PSCustomObject]@{ Name="OsArchitecture"; Value=$os.OSArchitecture }
    [PSCustomObject]@{ Name="OsType"; Value=$os.OSType }
    [PSCustomObject]@{ Name="OsSystemDirectory"; Value=$os.SystemDirectory }
    [PSCustomObject]@{ Name="OsWindowsDirectory"; Value=$os.WindowsDirectory }
    [PSCustomObject]@{ Name="OsLocale"; Value=$os.Locale }
    [PSCustomObject]@{ Name="OsLanguage"; Value=$os.OSLanguage }
    [PSCustomObject]@{ Name="OsInstallDate"; Value=(Convert-WmiDate $os.InstallDate) }
    [PSCustomObject]@{ Name="OsLastBootUpTime"; Value=(Convert-WmiDate $os.LastBootUpTime) }
    [PSCustomObject]@{ Name="OsUptime"; Value=((Get-Date) - (Convert-WmiDate $os.LastBootUpTime)) }

    # BIOS
    [PSCustomObject]@{ Name="BiosName"; Value=$bios.Name }
    [PSCustomObject]@{ Name="BiosManufacturer"; Value=$bios.Manufacturer }
    [PSCustomObject]@{ Name="BiosVersion"; Value=($bios.SMBIOSBIOSVersion -join ' ') }
    [PSCustomObject]@{ Name="BiosSerialNumber"; Value=$bios.SerialNumber }
    [PSCustomObject]@{ Name="BiosReleaseDate"; Value=(Convert-WmiDate $bios.ReleaseDate) }

    # CPU (basic)
    [PSCustomObject]@{ Name="CpuName"; Value=$cpu.Name }
    [PSCustomObject]@{ Name="CpuCores"; Value=$cpu.NumberOfCores }
    [PSCustomObject]@{ Name="CpuLogicalProcessors"; Value=$cpu.NumberOfLogicalProcessors }

    # Timezone
    [PSCustomObject]@{ Name="TimeZone"; Value=$tz.Caption }
    [PSCustomObject]@{ Name="TimeZoneOffset"; Value=$tz.Bias }
)

# -----------------------------
# GPU Info
# -----------------------------
$gpus = Get-WmiObject Win32_VideoController

$gpuIndex = 0
foreach ($gpu in $gpus) {
    $sysinfo += [PSCustomObject]@{ Name="Gpu${gpuIndex}Name"; Value=$gpu.Name }
    $sysinfo += [PSCustomObject]@{ Name="Gpu${gpuIndex}DriverVersion"; Value=$gpu.DriverVersion }
    $sysinfo += [PSCustomObject]@{ Name="Gpu${gpuIndex}AdapterRAM"; Value=(Format-Bytes $gpu.AdapterRAM) }
    $sysinfo += [PSCustomObject]@{ Name="Gpu${gpuIndex}VideoProcessor"; Value=$gpu.VideoProcessor }
    $sysinfo += [PSCustomObject]@{ Name="Gpu${gpuIndex}CurrentResolution"; Value=("$($gpu.CurrentHorizontalResolution)x$($gpu.CurrentVerticalResolution)") }
    $gpuIndex++
}

# -----------------------------
# Disk Info (Logical Drives)
# -----------------------------
$disks = Get-WmiObject Win32_LogicalDisk -Filter "DriveType=3"

foreach ($disk in $disks) {
    $name = $disk.DeviceID.Replace(":", "")

    $sysinfo += [PSCustomObject]@{ Name="Disk${name}FileSystem"; Value=$disk.FileSystem }
    $sysinfo += [PSCustomObject]@{ Name="Disk${name}Size"; Value=(Format-Bytes $disk.Size) }
    $sysinfo += [PSCustomObject]@{ Name="Disk${name}FreeSpace"; Value=(Format-Bytes $disk.FreeSpace) }
    $sysinfo += [PSCustomObject]@{ Name="Disk${name}VolumeName"; Value=$disk.VolumeName }
}

# -----------------------------
# Physical Disk Info
# -----------------------------
$physicalDisks = Get-WmiObject Win32_DiskDrive

$diskIndex = 0
foreach ($pd in $physicalDisks) {
    $sysinfo += [PSCustomObject]@{ Name="PhysicalDisk${diskIndex}Model"; Value=$pd.Model }
    $sysinfo += [PSCustomObject]@{ Name="PhysicalDisk${diskIndex}Interface"; Value=$pd.InterfaceType }
    $sysinfo += [PSCustomObject]@{ Name="PhysicalDisk${diskIndex}Size"; Value=(Format-Bytes $pd.Size) }
    $sysinfo += [PSCustomObject]@{ Name="PhysicalDisk${diskIndex}SerialNumber"; Value=$pd.SerialNumber }
    $diskIndex++
}

Export-CleanCsv $sysinfo (Join-Path $OutDir "systeminfo.csv")

# -----------------------------
# IP Config (raw command output)
# -----------------------------
Write-Host "6/6 Gathering IPConfig..."
ipconfig /all | Set-Content (Join-Path $OutDir "ipconfig.txt")

Write-Host "Export complete. Output folder: $OutDir"
