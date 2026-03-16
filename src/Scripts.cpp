#include "Scripts.h"

#define CSV_CONVERT_TEXT LR"term(ConvertTo-Csv -NoTypeInformation | ForEach-Object {$_ -replace '\"',''}")term"

const wchar_t* g_script_netstat_tcp_text = LR"term(powershell -command "Get-NetTCPConnection | )term"
L"Select-Object LocalAddress,LocalPort,RemoteAddress,RemotePort,State,CreationTime,OwningProcess,@{Name='Process';Expression={(Get-Process -Id $_.OwningProcess).ProcessName}} | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_netstat_udp_text = LR"term(powershell -command "Get-NetUDPEndpoint | )term"
L"select LocalAddress,LocalPort,CreationTime,OwningProcess,@{Name='Process';Expression={(Get-Process -Id $_.OwningProcess).ProcessName}} | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_programs_text = LR"term(powershell -command "Get-ItemProperty 'HKLM:/Software/Microsoft/Windows/CurrentVersion/Uninstall/*' | )term"
"Where {$_.DisplayName} | Select DisplayName,DisplayVersion | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_processor_text = LR"term(powershell -command "Get-CimInstance Win32_Processor | )term"
"Select-Object Name, NumberOfCores, NumberOfEnabledCore, NumberOfLogicalProcessors, ThreadCount, MaxClockSpeed | "
CSV_CONVERT_TEXT;

const wchar_t* g_script_systeminfo_text = LR"term(powershell -command "Get-ComputerInfo | Select * | ForEach-Object { $_.PSObject.Properties } | Select Name,Value | )term"
CSV_CONVERT_TEXT;

#define __TEST__ 1

#if __TEST__ == 1
const wchar_t* g_script_ipconfig_text = LR"term(ipconfig /all)term";
#endif

#if __TEST__ == 2
const wchar_t* g_script_ipconfig_text = LR"term(powershell -command "Get-NetIPConfiguration | )term"
"Select-Object InterfaceAlias, InterfaceDescription,"
    LR"term(@{Name=\"IPv4Address\";Expression={$_.IPv4Address.IPAddress}},)term"
    LR"term(@{Name=\"IPv6Address\";Expression={$_.IPv6Address.IPAddress}},)term"
    LR"term(@{Name=\"IPv4Gateway\";Expression={$_.IPv4DefaultGateway.NextHop}},)term"
    LR"term(@{Name=\"DNSServers\";Expression={($_.DNSServer.ServerAddresses -join ';')}} | )term"
CSV_CONVERT_TEXT;

//const wchar_t* g_script_ipconfig_text = LR"term(powershell -command "Get-NetIPConfiguration | Select-Object InterfaceAlias, InterfaceDescription, @{Name="IPv4Address";Expression={$_.IPv4Address.IPAddress}}, @{Name="IPv6Address";Expression={$_.IPv6Address.IPAddress}}, @{Name="IPv4Gateway";Expression={$_.IPv4DefaultGateway.NextHop}}, @{Name="DNSServers";Expression={($_.DNSServer.ServerAddresses -join ';')}} | ConvertTo-Csv -NoTypeInformation")term";
#endif

#if __TEST__ == 3
const wchar_t* g_script_ipconfig_text = LR"term(powershell -command "Get-NetIPConfiguration | )term"
"Select-Object InterfaceAlias, InterfaceDescription, NetProfile.Name"
    LR"term(@{Name="IPv4Address";Expression={$_.IPv4Address.IPAddress}},)term"
    LR"term(@{Name="IPv4Gateway";Expression={$_.IPv4DefaultGateway.NextHop}},)term"
    LR"term(@{Name="DNSServers";Expression={($_.DNSServer.ServerAddresses -join ';')}},)term"
    LR"term(@{Name="DHCP";Expression={$_.NetAdapter.DhcpEnabled}},)term"
    LR"term(@{Name="MAC";Expression={$_.NetAdapter.MacAddress}},)term"
    LR"term(@{Name="LinkSpeed";Expression={$_.NetAdapter.LinkSpeed}} | )term"
CSV_CONVERT_TEXT;
#endif

#if __TEST__ == 4
const wchar_t* g_script_ipconfig_text = LR"term(powershell -command "Get-NetIPConfiguration | Select InterfaceAlias,InterfaceDescription,@{Name='IPv4Address';Expression={$_.IPv4Address.IPAddress}},@{Name='IPv6Address';Expression={$_.IPv6Address.IPAddress}},@{Name='IPv4Gateway';Expression={$_.IPv4DefaultGateway.NextHop}},@{Name='DNSServers';Expression={($_.DNSServer.ServerAddresses -join ';')}},@{Name='MAC';Expression={$_.NetAdapter.MacAddress}},@{Name='DHCP';Expression={$_.NetAdapter.DhcpEnabled}} | ConvertTo-Csv -NoTypeInformation | ForEach-Object {$_ -replace '\"',''}")term";
#endif

#if __TEST__ == 5
const wchar_t* g_script_ipconfig_text = LR"term(powershell -command "$l=ipconfig /all;$r=@();$c=$null;$k=$null;foreach($line in $l){if($line -match 'adapter (.+):$'){if($c){$r+=[pscustomobject]$c};$c=@{Adapter=$Matches[1].Trim()};$k=$null;continue};if($line -match '^\s*([^:]+?)\s*:\s*(.*)$'){$key=($Matches[1]-replace '\.+','').Trim();$val=$Matches[2].Trim();if($c){$c[$key]=$val;$k=$key};continue};if($line -match '^\s+(\S.*)$' -and $k){$c[$k]+='; '+$Matches[1].Trim()}};if($c){$r+=[pscustomobject]$c};$r|ConvertTo-Csv -NoTypeInformation|ForEach-Object{$_ -replace '\"',''}")term";
#endif

#undef CSV_CONVERT_TEXT