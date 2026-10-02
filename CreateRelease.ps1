<#
.SYNOPSIS
    Builds QuoolTool in Release and packages it into a versioned zip.
    Replacement for the old Packager C++ project.

.DESCRIPTION
    Expects to live in the solution root (next to QuoolTool.sln / QuoolTool.slnx).
    The version is read from src/Version.cpp so it stays the single source of truth.

.PARAMETER NoPause
    Skip the "Press Enter to continue" prompt at the end (useful for CI).
#>
[CmdletBinding()]
param(
    [switch]$NoPause
)

$ErrorActionPreference = 'Stop'

$ProjectName = 'QuoolTool'
$BatchScript = 'QuoolTools_OldWindows.bat'
$VersionFile = 'src/Version.cpp'

function Fail([string]$Message) {
    Write-Host "Error: $Message" -ForegroundColor Red
    $script:failed = $true
}

function Write-Banner([string]$Text) {
    Write-Host '====================='
    Write-Host $Text
    Write-Host '====================='
}

function Get-VersionTag([string]$Path) {
    # Parses: Version g_version = { .major = 1, .minor = 9 };
    $content = Get-Content -Raw -Path $Path
    $m = [regex]::Match($content, 'g_version\s*=\s*\{\s*\.major\s*=\s*(\d+)\s*,\s*\.minor\s*=\s*(\d+)')
    if (-not $m.Success) {
        throw "Could not parse g_version from $Path"
    }
    $major = [int]$m.Groups[1].Value
    $minor = [int]$m.Groups[2].Value
    if ($major -eq 0) {
        throw "Version in $Path is invalid (major is 0)"
    }
    return "v$major.$minor"
}

$script:failed = $false
Push-Location $PSScriptRoot
try {
    Write-Banner ' Quool Tool Packager '
    Write-Host ''

    # --- Version ----------------------------------------------------------
    $versionTag = Get-VersionTag $VersionFile
    Write-Host "Version: $versionTag"
    Write-Host ''

    # --- Generate Project Files -------------------------------------------
    Write-Banner 'Generate Project Files:'
    & .\GenerateProjectFiles.bat
    if ($LASTEXITCODE -ne 0) {
        Fail "GenerateProjectFiles failed with exit code $LASTEXITCODE"
        return
    }

    # --- Locate solution --------------------------------------------------
    $sln = "$ProjectName.sln"
    if (-not (Test-Path $sln)) {
        $sln = "$ProjectName.slnx"
        if (-not (Test-Path $sln)) {
            Fail 'No solution file found after running GenerateProjectFiles'
            return
        }
    }

    # --- Build ------------------------------------------------------------
    if (-not (Get-Command msbuild -ErrorAction SilentlyContinue)) {
        Fail 'msbuild was not found on PATH (run from a Developer PowerShell for VS)'
        return
    }

    # Final outputs (exe, lib, pdb) go here. Must match the Release exe location.
    $buildDir = 'build\release_package'

    # Absolute path with forward slashes (avoids trailing-backslash quoting
    # problems when PowerShell passes the argument to msbuild.exe).
    $outDirAbs = (Join-Path $PSScriptRoot $buildDir).Replace('\', '/') + '/'

    Write-Banner '   Build Output:'

    & msbuild "/t:$ProjectName" /nologo /verbosity:minimal -p:Configuration=Release "-p:OutDir=$outDirAbs" $sln
    if ($LASTEXITCODE -ne 0) {
        Fail "Build failed with exit code $LASTEXITCODE"
        return
    }

    # --- Locate and rename exe -------------------------------------------
    $exe      = Join-Path $buildDir "${ProjectName}_windows_x64_Release.exe"
    $renamed  = Join-Path $buildDir "${ProjectName}_$versionTag.exe"

    if (-not (Test-Path $exe)) {
        Fail "Executable could not be found: $exe"
        return
    }

    try {
        Copy-Item -Path $exe -Destination $renamed -Force
    }
    catch {
        Fail "Failed to rename exe from(`"$exe`") to(`"$renamed`")`n    `"$($_.Exception.Message)`""
        return
    }

    # --- Create zip (files placed at the root of the archive) ------------
    if (-not (Test-Path $BatchScript)) {
        Fail "Batch script not found: $BatchScript"
        return
    }

    $zipName = "${ProjectName}_$versionTag.zip"
    if (Test-Path $zipName) { Remove-Item $zipName -Force }

    Compress-Archive -Path $renamed, $BatchScript -DestinationPath $zipName

    if (-not (Test-Path $zipName)) {
        Fail "Failed to create zip/zip doesn't exist"
        return
    }

    Remove-Item $renamed -Force -ErrorAction SilentlyContinue

    Write-Host "Successfully created zip: $zipName" -ForegroundColor Green
}
catch {
    Fail $_.Exception.Message
}
finally {
    Pop-Location
    if (-not $NoPause) {
        Read-Host 'Press Enter to continue' | Out-Null
    }
}

if ($script:failed) { exit 1 }
