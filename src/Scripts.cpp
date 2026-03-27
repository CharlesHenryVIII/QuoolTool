#include "Scripts.h"

#define CSV_CONVERT_TEXT LR"term(ConvertTo-Csv -NoTypeInformation)term"// | ForEach-Object {$_ -replace '\"',''}")term"

const wchar_t* g_script_netstat_tcp_text = LR"term(powershell -command "Get-NetTCPConnection | )term"
L"Select-Object LocalAddress,LocalPort,RemoteAddress,RemotePort,State,CreationTime,OwningProcess,@{Name='Process';Expression={(Get-Process -Id $_.OwningProcess).ProcessName}} | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_netstat_udp_text = LR"term(powershell -command "Get-NetUDPEndpoint | )term"
L"select LocalAddress,LocalPort,CreationTime,OwningProcess,@{Name='Process';Expression={(Get-Process -Id $_.OwningProcess).ProcessName}} | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_programs_text = LR"term(powershell -command "Get-ItemProperty 'HKLM:/Software/Microsoft/Windows/CurrentVersion/Uninstall/*' | )term"
"Where {$_.DisplayName} | Select DisplayName,DisplayVersion | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_systeminfo_text = 
LR"term(powershell -command "$()term"
LR"term(Get-ComputerInfo | Select-Object )term"
LR"term(WindowsBuildLabEx, WindowsCurrentVersion, WindowsEditionId, WindowsInstallationType, WindowsInstallDateFromRegistry, WindowsProductId, WindowsProductName, WindowsSystemRoot, WindowsVersion, OSDisplayVersion, BiosBuildNumber, BiosCaption, BiosCodeSet, BiosCurrentLanguage, BiosDescription, BiosEmbeddedControllerMajorVersion, BiosEmbeddedControllerMinorVersion, BiosFirmwareType, BiosIdentificationCode, BiosInstallableLanguages, BiosInstallDate, BiosManufacturer, BiosName, BiosPrimaryBIOS, BiosReleaseDate, BiosSeralNumber, BiosSMBIOSBIOSVersion, BiosSMBIOSMajorVersion, BiosSMBIOSMinorVersion, BiosSMBIOSPresent, BiosSoftwareElementState, BiosStatus, BiosSystemBiosMajorVersion, BiosSystemBiosMinorVersion, BiosVersion, CsAutomaticManagedPagefile, CsAutomaticResetBootOption, CsAutomaticResetCapability, CsBootROMSupported, CsBootStatus, CsBootupState, CsCaption, CsChassisBootupState, CsChassisSKUNumber, CsCurrentTimeZone, CsDaylightInEffect, CsDescription, CsDNSHostName, CsDomain, CsDomainRole, CsEnableDaylightSavingsTime, CsFrontPanelResetStatus, CsHypervisorPresent, CsInfraredSupported, CsKeyboardPasswordStatus, CsManufacturer, CsModel, CsName, CsNetworkAdapters, CsNetworkServerModeEnabled, CsNumberOfLogicalProcessors, CsNumberOfProcessors, CsProcessors, CsOEMStringArray, CsPartOfDomain, CsPauseAfterReset, CsPCSystemType, CsPCSystemTypeEx, CsPowerManagementCapabilities, CsPowerManagementSupported, CsPowerOnPasswordStatus, CsPowerState, CsPowerSupplyState, CsPrimaryOwnerContact, CsPrimaryOwnerName | )term"
LR"term(ForEach-Object { $_.PSObject.Properties } | Select Name,Value; )term"
LR"term(Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfEnabledCore, NumberOfLogicalProcessors, ThreadCount, MaxClockSpeed | )term"
LR"term(ForEach-Object { $_.PSObject.Properties } | Select Name,Value) | )term"
CSV_CONVERT_TEXT;

const wchar_t* g_script_ipconfig_text = LR"term(ipconfig /all)term";
const wchar_t* g_script_network_text = LR"term(powershell -NoProfile -Command "Get-CimInstance Win32_NetworkAdapterConfiguration | )term"
LR"term(Select-Object Description, IPEnabled, IPAddress, IPSubnet, SettingID, DefaultIPGateway, DHCPEnabled, DHCPServer, DNSDomain, DNSDomainSuffixSearchOrder, DNSHostName, DomainDNSRegistrationEnabled, MACAddress | )term"
LR"term(ConvertTo-Xml -As String")term";

#if 1
//const wchar_t* g_script_disk_text = LR"term(powershell -NoProfile -Command "$DiskInfo = Get-WmiObject Win32_DiskDrive | ForEach-Object { $disk = $_; $partitions = 'ASSOCIATORS OF {Win32_DiskDrive.DeviceID=''' + $disk.DeviceID + '''} WHERE AssocClass = Win32_DiskDriveToDiskPartition'; Get-WmiObject -Query $partitions | ForEach-Object { $partition = $_; $drives = 'ASSOCIATORS OF {Win32_DiskPartition.DeviceID=''' + $partition.DeviceID + '''} WHERE AssocClass = Win32_LogicalDiskToPartition'; Get-WmiObject -Query $drives | ForEach-Object { New-Object -Type PSCustomObject -Property @{ Disk = $disk.DeviceID; DiskSize = $disk.Size; DiskModel = $disk.Model; Partition = $partition.Name; RawSize = $partition.Size; DriveLetter = $_.DeviceID; VolumeName = $_.VolumeName; Size = $_.Size; FreeSpace = $_.FreeSpace; } } } }; $DiskInfo | Select-Object Disk, DiskModel, DiskSize, Partition, RawSize, DriveLetter, VolumeName, Size, FreeSpace | ConvertTo-Csv -NoTypeInformation ")term";
const wchar_t* g_script_disk_text = LR"term(powershell -NoProfile -Command "function f($b){ if(!$b){return '0 B'}; $bytes=[uint64]$b; if($bytes -ge 1TB){return '{0:N2} TB' -f ($bytes/1TB)}elseif($bytes -ge 1GB){return '{0:N2} GB' -f ($bytes/1GB)}elseif($bytes -ge 1MB){return '{0:N2} MB' -f ($bytes/1MB)}else{return '{0} B' -f $bytes} }; $DiskInfo = Get-WmiObject Win32_DiskDrive | ForEach-Object { $disk = $_; $partitions = 'ASSOCIATORS OF {Win32_DiskDrive.DeviceID=''' + $disk.DeviceID + '''} WHERE AssocClass = Win32_DiskDriveToDiskPartition'; Get-WmiObject -Query $partitions | ForEach-Object { $partition = $_; $drives = 'ASSOCIATORS OF {Win32_DiskPartition.DeviceID=''' + $partition.DeviceID + '''} WHERE AssocClass = Win32_LogicalDiskToPartition'; Get-WmiObject -Query $drives | ForEach-Object { New-Object -Type PSCustomObject -Property @{ Disk = $disk.DeviceID; DiskSize = f $disk.Size; DiskModel = $disk.Model; Partition = $partition.Name; RawSize = f $partition.Size; DriveLetter = $_.DeviceID; VolumeName = $_.VolumeName; Size = f $_.Size; FreeSpace = f $_.FreeSpace; } } } }; $DiskInfo | Select-Object Disk, DiskModel, DiskSize, Partition, RawSize, DriveLetter, VolumeName, Size, FreeSpace | ConvertTo-Csv -NoTypeInformation")term";
//const wchar_t* g_script_disk_text =
////LR"term($DiskInfo = Get-WmiObject Win32_DiskDrive | ForEach-Object { $disk = $_ $partitions = "ASSOCIATORS OF " + "{Win32_DiskDrive.DeviceID='$($disk.DeviceID)'} " + "WHERE AssocClass = Win32_DiskDriveToDiskPartition" Get-WmiObject -Query $partitions | ForEach-Object { $partition = $_ $drives = "ASSOCIATORS OF " + "{Win32_DiskPartition.DeviceID='$($partition.DeviceID)'} " + "WHERE AssocClass = Win32_LogicalDiskToPartition" Get-WmiObject -Query $drives | ForEach-Object { New-Object -Type PSCustomObject -Property @{ Disk        = $disk.DeviceID DiskSize    = $disk.Size DiskModel   = $disk.Model Partition   = $partition.Name RawSize     = $partition.Size DriveLetter = $_.DeviceID VolumeName  = $_.VolumeName Size        = $_.Size FreeSpace   = $_.FreeSpace } } } } $DiskInfo | Select-Object Disk, DiskModel, DiskSize, Partition, RawSize, DriveLetter, VolumeName, Size, FreeSpace | ConvertTo-Csv -NoTypeInformation | ForEach-Object { $_ -replace '"','' } | Out-File -FilePath ".\disk_mapping.csv" -Encoding ASCII)term";
//LR"term($DiskInfo = Get-WmiObject Win32_DiskDrive | ForEach-Object { $disk = $_ $partitions = "ASSOCIATORS OF " + "{Win32_DiskDrive.DeviceID='$($disk.DeviceID)'} " + "WHERE AssocClass = Win32_DiskDriveToDiskPartition" Get-WmiObject -Query $partitions | ForEach-Object { $partition = $_ $drives = "ASSOCIATORS OF " + "{Win32_DiskPartition.DeviceID='$($partition.DeviceID)'} " + "WHERE AssocClass = Win32_LogicalDiskToPartition" Get-WmiObject -Query $drives | ForEach-Object { New-Object -Type PSCustomObject -Property @{ Disk        = $disk.DeviceID DiskSize    = $disk.Size DiskModel   = $disk.Model Partition   = $partition.Name RawSize     = $partition.Size DriveLetter = $_.DeviceID VolumeName  = $_.VolumeName Size        = $_.Size FreeSpace   = $_.FreeSpace } } } }; $DiskInfo | Select-Object Disk, DiskModel, DiskSize, Partition, RawSize, DriveLetter, VolumeName, Size, FreeSpace | ConvertTo-Csv -NoTypeInformation | ForEach-Object { $_ -replace '"','' })term";


#else
const wchar_t* g_script_disk_text = LR"term(powershell -command "Get-PhysicalDisk | Select FriendlyName, Size, MediaType, BusType | )term"
CSV_CONVERT_TEXT;
#endif

#undef CSV_CONVERT_TEXT