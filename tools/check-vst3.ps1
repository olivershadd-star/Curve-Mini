param(
    [Parameter(Mandatory=$true)][string]$Bundle,
    [switch]$AllowSharedRuntime
)
$ErrorActionPreference = 'Stop'
$binary = Join-Path $Bundle 'Contents\x86_64-win\Curve Mini Prototype 0.2.vst3'
if (!(Test-Path $binary -PathType Leaf)) { throw "Binary missing from complete VST3 bundle: $binary" }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation = & $vswhere -latest -products '*' -property installationPath
$dumpbin = Get-ChildItem "$installation\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" | Sort-Object FullName | Select-Object -Last 1
if (!$dumpbin) { throw 'dumpbin.exe not found' }
$headers = & $dumpbin.FullName /headers $binary
if ($LASTEXITCODE -ne 0 -or !($headers -match '8664 machine')) { throw 'Not a valid Windows x64 PE module' }
$exports = & $dumpbin.FullName /exports $binary
if ($LASTEXITCODE -ne 0 -or !($exports -match 'GetPluginFactory') -or !($exports -match 'InitDll') -or !($exports -match 'ExitDll')) { throw 'VST3 entry points are missing' }
$dependencies = & $dumpbin.FullName /dependents $binary
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect module dependencies' }
$dependencies | Write-Output
$sharedRuntime = $dependencies | Where-Object { $_ -match '(?i)(msvcp\d+|vcruntime\d+|concrt\d+)\S*\.dll' }
if ($sharedRuntime -and !$AllowSharedRuntime) { throw "External Microsoft runtime dependency remains: $sharedRuntime" }
Get-FileHash -Algorithm SHA256 $binary | Format-List | Out-String | Write-Output
Write-Output 'PASS: complete x64 VST3 bundle, factory/entry points and runtime dependency check'
