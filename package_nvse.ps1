# NVSE Packaging Script
# Packages NVSE release files into a .7z archive

param(
    [Parameter(Mandatory=$true)]
    [string]$Version  # Format: x_x_x (e.g., 6_3_0)
)

$ErrorActionPreference = "Stop"

# Define paths
$ScriptDir = $PSScriptRoot
$ReleaseDir = Join-Path $ScriptDir "nvse\Release"
$ReleaseCsDir = Join-Path $ScriptDir "nvse\Release CS"
$OutputFile = Join-Path $ScriptDir "nvse_$Version.7z"
$TempDir = Join-Path $ScriptDir "nvse_package_temp"

# Files to include from Release folder
$ReleaseFiles = @(
    "nvse_1_4.dll",
    "nvse_1_4.pdb",
    "nvse_loader.exe",
    "nvse_loader.pdb",
    "nvse_steam_loader.dll",
    "nvse_steam_loader.pdb"
)

# Files to include from Release CS folder
$ReleaseCsFiles = @(
    "nvse_editor_1_4.dll",
    "nvse_editor_1_4.pdb"
)

# nvse_config.ini content
$ConfigContent = @"
; If an INI option is not written in the INI, then it is given its default value.

[Release]

; Determines if NVSE script errors should be reported via a corner UI message popup.
; Does not prevent console prints for errors from occuring.
; Recommended only for developers / testers, otherwise it can get distracting.
; *Default value = 0 (off)
bWarnScriptErrors = 0

; If non-zero (true), prevents scripts from stopping execution if a command returns false or there's a missing reference.
; *Default value = 1 (on)
bAllowScriptsToCrash = 1

; If non-zero (true), will hide warnings about the NVSE cosave having exceeded a certain size.
; Not recommended to enable, since it could hide legitimate issues with scripts leaking resources.
; *Default value = 0
bNoSaveWarnings = 0

; If non-zero (true), in-game script compilation errors will always be printed to console.
; By default, they are only printed if the console is already open.
; HIGHLY RECOMMENDED for debugging Script Runner files.
; *Default value = 0 (off)
bAlwaysPrintScriptCompilationError = 0

; Determines what to print to the NVSE log.
; The greater the value, the more less-important messages will be printed.
; Valid values:
;  Show Errors = 1
;  Show Warnings = 2,
;  Show Message = 3,
;  Show VerboseMessage = 4,
;  Show DebugMessage = 5
; If set to zero, will default to 3.
; *Default value = 3
LogLevel = 3

; If non-zero (true), will modify Update3D so that it has additional code to handle being called on the player. However, UpdatePlayer3D is a better alternative nowadays.
; *Default value = 0 (false)
AlternateUpdate3D = 0


[Logging]

; If non-zero (true), prints any vanilla game errors to falloutnv_error.log and falloutnv_havok.log in the game folder.
; *Default internal value (if unspecified) = 0 (off)
; *Written default = 1
EnableGameErrorLog = 1

; If zero (false), will erase the vanilla game error files every time a new session starts.
; Otherwise, errors will be appended to those files.
; *Default value = 0
bAppendErrorLogs = 0

; If empty string, NVSE plugin logs will be written in the base game directory.
; Otherwise, plugin logs will be written in the specified path string, relative to the base game directory.
; Functional since 6.3.0, but plugins must opt into using this log path global, exposed in the NVSE API.
; *Written default = ""
sPluginLogPath = ""


; If NVHR (Heap Replacer) is installed, this section does nothing.
[Memory]
; *Default internal value (if unspecified) = 200
; *Written default = 500
; *Maximum = 1000
DefaultHeapInitialAllocMB = 500

; *Default internal value = 64
; *Written default = 64
; *Maximum = 192
FileHeapSizeMB = 64


[Fixes]

; If non-zero (true), will disable NVSE's SCOF fixes.
; *Default value = 0
DisableSCOFfixes = 0


[Tests]

; If non-zero (true), will enable NVSE's runtime tests.
; Notably, script unit test files in "Data/nvse/unit_tests" folder will be executed.
; Only useful for testers / developers.
; *Default value = 0 (off)
; *Added in xNVSE 6.2.7
EnableRuntimeTests = 0
"@

Write-Host "Packaging NVSE version $Version..." -ForegroundColor Cyan

# Clean up any existing temp directory
if (Test-Path $TempDir) {
    Remove-Item -Path $TempDir -Recurse -Force
}

# Create temp directory structure
New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
New-Item -ItemType Directory -Path "$TempDir\Data\NVSE" -Force | Out-Null

# Copy Release files
Write-Host "Copying Release files..." -ForegroundColor Yellow
foreach ($file in $ReleaseFiles) {
    $sourcePath = Join-Path $ReleaseDir $file
    if (Test-Path $sourcePath) {
        Copy-Item -Path $sourcePath -Destination $TempDir
        Write-Host "  Copied: $file" -ForegroundColor Green
    } else {
        Write-Error "File not found: $sourcePath"
        exit 1
    }
}

# Copy Release CS files
Write-Host "Copying Release CS files..." -ForegroundColor Yellow
foreach ($file in $ReleaseCsFiles) {
    $sourcePath = Join-Path $ReleaseCsDir $file
    if (Test-Path $sourcePath) {
        Copy-Item -Path $sourcePath -Destination $TempDir
        Write-Host "  Copied: $file" -ForegroundColor Green
    } else {
        Write-Error "File not found: $sourcePath"
        exit 1
    }
}

# Create nvse_config.ini
Write-Host "Creating nvse_config.ini..." -ForegroundColor Yellow
$ConfigContent | Out-File -FilePath "$TempDir\Data\NVSE\nvse_config.ini" -Encoding UTF8 -NoNewline
Write-Host "  Created: Data\NVSE\nvse_config.ini" -ForegroundColor Green

# Remove existing output file if it exists
if (Test-Path $OutputFile) {
    Remove-Item -Path $OutputFile -Force
}

# Create 7z archive
Write-Host "Creating 7z archive..." -ForegroundColor Yellow
$7zPath = "C:\Program Files\7-Zip\7z.exe"  # Assumes 7z is in PATH; modify if needed (e.g., "C:\Program Files\7-Zip\7z.exe")

# Change to temp directory and create archive
Push-Location $TempDir
try {
    & $7zPath a -t7z "$OutputFile" * -mx=9
    if ($LASTEXITCODE -ne 0) {
        Write-Error "7z compression failed with exit code $LASTEXITCODE"
        exit 1
    }
} finally {
    Pop-Location
}

# Clean up temp directory
Write-Host "Cleaning up..." -ForegroundColor Yellow
Remove-Item -Path $TempDir -Recurse -Force

Write-Host ""
Write-Host "Successfully created: $OutputFile" -ForegroundColor Green
Write-Host "Archive contents:" -ForegroundColor Cyan
& $7zPath l $OutputFile
