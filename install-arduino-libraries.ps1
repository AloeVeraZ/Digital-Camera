[CmdletBinding()]
param(
    [string]$SketchbookPath
)

$ErrorActionPreference = 'Stop'

if (-not $SketchbookPath) {
    $oneDriveDocuments = Join-Path $env:USERPROFILE 'OneDrive\Documents'
    $documents = if (Test-Path -LiteralPath $oneDriveDocuments) {
        $oneDriveDocuments
    } else {
        [Environment]::GetFolderPath([Environment+SpecialFolder]::MyDocuments)
    }
    $SketchbookPath = Join-Path $documents 'Arduino'
}

$bundlePath = Join-Path $PSScriptRoot 'Freenove-Libraries'
$libraryPath = Join-Path $SketchbookPath 'libraries'
$archives = @(
    'lvgl.zip',
    'TFT_eSPI_Setups.zip',
    'TFT_eSPI.zip',
    'Freenove_WS2812_Lib_for_ESP32.zip'
)

if (-not (Test-Path -LiteralPath $bundlePath)) {
    throw "Bundled library folder not found: $bundlePath"
}

New-Item -ItemType Directory -Force -Path $libraryPath | Out-Null
$libraryPath = (Resolve-Path -LiteralPath $libraryPath).Path
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$backupPath = Join-Path $SketchbookPath "library-backups\$stamp"

Add-Type -AssemblyName System.IO.Compression.FileSystem

foreach ($archiveName in $archives) {
    $archivePath = Join-Path $bundlePath $archiveName
    if (-not (Test-Path -LiteralPath $archivePath)) {
        throw "Required bundled library is missing: $archivePath"
    }

    $archive = [IO.Compression.ZipFile]::OpenRead($archivePath)
    try {
        $roots = @($archive.Entries |
            Where-Object { $_.FullName -and -not $_.FullName.StartsWith('__MACOSX/') } |
            ForEach-Object { ($_.FullName -split '/')[0] } |
            Sort-Object -Unique)
    } finally {
        $archive.Dispose()
    }

    if ($roots.Count -ne 1 -or -not $roots[0]) {
        throw "Unexpected ZIP structure in $archiveName"
    }

    $targetPath = Join-Path $libraryPath $roots[0]
    $fullTargetPath = [IO.Path]::GetFullPath($targetPath)
    if (-not $fullTargetPath.StartsWith(
        $libraryPath + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing library path outside the Arduino sketchbook: $fullTargetPath"
    }

    if (Test-Path -LiteralPath $fullTargetPath) {
        New-Item -ItemType Directory -Force -Path $backupPath | Out-Null
        Move-Item -LiteralPath $fullTargetPath -Destination $backupPath
    }

    Expand-Archive -LiteralPath $archivePath -DestinationPath $libraryPath
    Write-Host "Installed $($roots[0])"
}

Write-Host ''
Write-Host "Freenove libraries installed in: $libraryPath"
if (Test-Path -LiteralPath $backupPath) {
    Write-Host "Previous library folders were preserved in: $backupPath"
}
Write-Host 'Restart Arduino IDE before compiling.'
