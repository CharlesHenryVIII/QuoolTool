#include "Scripts.h"

const wchar_t* g_script_netstat_tcp_text = LR"term(powershell -command "Get-NetTCPConnection | )term"
LR"term(Select-Object LocalAddress,LocalPort,RemoteAddress,RemotePort,State,CreationTime,OwningProcess,@{Name='Process';Expression={(Get-Process -Id $_.OwningProcess).ProcessName}} |)term"
LR"term(ConvertTo-Csv -NoTypeInformation | ForEach-Object {$_ -replace '\"',''}")term";

const wchar_t* g_script_netstat_udp_text = LR"term(powershell -command "Get-NetUDPEndpoint | )term"
LR"term(select LocalAddress,LocalPort,CreationTime,OwningProcess,@{Name='Process';Expression={(Get-Process -Id $_.OwningProcess).ProcessName}} | )term"
LR"term(ConvertTo-Csv -NoTypeInformation | ForEach-Object {$_ -replace '\"',''}")term";

const wchar_t* g_script_programs_text = LR"term(powershell -command "Get-ItemProperty 'HKLM:/Software/Microsoft/Windows/CurrentVersion/Uninstall/*' | )term"
LR"term(Where {$_.DisplayName} | )term"
LR"term(Select DisplayName,DisplayVersion")term";

const wchar_t* g_script_processor_text = LR"term(powershell -command "Get-CimInstance Win32_Processor | )term"
LR"term(Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed")term";


const wchar_t* g_script_systeminfo_text = LR"term(powershell -command "$x = Get-ComputerInfo | Select *; $x.PSObject.Properties | Select Name,Value | ConvertTo-Csv -NoTypeInformation | ForEach-Object {$_ -replace '\"',''}")term";
//LR"term(systeminfo)term";
const wchar_t* g_script_ipconfig_text = LR"term(ipconfig)term";
