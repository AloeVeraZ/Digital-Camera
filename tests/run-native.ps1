[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$ZigPath,
  [Parameter(Mandatory=$true)][string]$LvglPath,
  [Parameter(Mandatory=$true)][string]$PythonPath,
  [string]$OutputPath = (Join-Path $env:TEMP 'digital-camera-native')
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
$OutputPath = (Resolve-Path -LiteralPath $OutputPath).Path
$config = Get-Content (Join-Path $LvglPath 'src/lv_conf.h') -Raw
$config = $config.Replace('#define LV_USE_FS_FATFS 1', '#define LV_USE_FS_FATFS 0')
$config = $config.Replace('"Arduino.h"', '"test_tick.h"').Replace('(millis())', '(native_tick())')
Set-Content -LiteralPath (Join-Path $OutputPath 'lv_conf.h') -Value $config
& $PythonPath (Join-Path $PSScriptRoot 'generate_fixtures.py') (Join-Path $OutputPath 'sd')
if ($LASTEXITCODE -ne 0) { throw 'Fixture generation failed' }
$sourceNames = @('main_ui','picture_ui','photo_image','ui_theme','battery_status','camera_view','diagnostics')
$wrappers = foreach ($name in $sourceNames) {
  $wrapper = Join-Path $OutputPath ($name + '.cpp')
  $source = (Join-Path $repo "mainCamera/$name.cpp").Replace('\','/')
  Set-Content -LiteralPath $wrapper -Value "#include `"test_platform.h`"`n#include `"$source`""
  $wrapper
}
$configDefine = '-DLV_CONF_PATH=' + (Join-Path $OutputPath 'lv_conf.h').Replace('\','/')
$arguments = @('-std=c++17','-O1',$configDefine,
  '-I' + $OutputPath, '-I' + (Join-Path $PSScriptRoot 'native'),
  '-I' + (Join-Path $repo 'mainCamera'), '-I' + $LvglPath, '-I' + (Join-Path $LvglPath 'src'),
  '-o', (Join-Path $OutputPath 'test-ui.exe'), (Join-Path $PSScriptRoot 'native/test_ui.cpp'))
$arguments += $wrappers
$cArguments = @('-lc','-O','ReleaseFast',$configDefine,
  '-I' + $OutputPath, '-I' + (Join-Path $PSScriptRoot 'native'), '-I' + $LvglPath,
  ('-femit-bin=' + (Join-Path $OutputPath 'lvgl.lib')))
$cArguments += @(Get-ChildItem (Join-Path $LvglPath 'src') -Recurse -Filter '*.c' | Select-Object -ExpandProperty FullName)
$cResponse = Join-Path $OutputPath 'compile-c.rsp'
Set-Content -LiteralPath $cResponse -Value (($cArguments | ForEach-Object { '"' + $_.Replace('\','/') + '"' }) -join "`n")
& $ZigPath build-lib "@$cResponse"
if ($LASTEXITCODE -ne 0) { throw 'Native LVGL build failed' }
$arguments += Join-Path $OutputPath 'lvgl.lib'
# A response file keeps the command below Windows' command-line length limit.
$escaped = $arguments | ForEach-Object { '"' + $_.Replace('\','/') + '"' }
$response = Join-Path $OutputPath 'compile.rsp'
Set-Content -LiteralPath $response -Value ($escaped -join "`n")
& $ZigPath c++ "@$response"
if ($LASTEXITCODE -ne 0) { throw 'Native UI build failed' }
& (Join-Path $OutputPath 'test-ui.exe') (Join-Path $OutputPath 'sd') $OutputPath
if ($LASTEXITCODE -ne 0) { throw 'Native UI checks failed' }
Write-Host "Rendered UI snapshots: $OutputPath"
