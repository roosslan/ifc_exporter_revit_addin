<#
    Раскладка надстройки для одной версии Revit:
      <Destination>\ifc_exporter.addin
      <Destination>\ifc_exporter\ifc_exporter.dll, зависимости и resources\
    Содержимое <Destination> копируется в %APPDATA%\Autodesk\Revit\Addins\<год> (так же его раскладывает MSI).
#>
param(
    [Parameter(Mandatory = $true)] [string] $BuildDir,
    [Parameter(Mandatory = $true)] [string] $Destination,
    [string] $AddinTemplate,
    [string] $ResourcesDir
)

$ErrorActionPreference = 'Stop'

# В Windows PowerShell 5.1 $PSScriptRoot пуст в значениях параметров по умолчанию
if (-not $AddinTemplate) { $AddinTemplate = Join-Path $PSScriptRoot '..\ifc_exporter.addin' }
if (-not $ResourcesDir) { $ResourcesDir = Join-Path $PSScriptRoot '..\copy_into_msi\resources' }

$dll_dir = Join-Path $Destination 'ifc_exporter'
if (Test-Path $Destination) {
    Remove-Item $Destination -Recurse -Force
}
New-Item -ItemType Directory -Force -Path (Join-Path $dll_dir 'resources') | Out-Null

if (-not (Test-Path (Join-Path $BuildDir 'ifc_exporter.dll'))) {
    throw "ifc_exporter.dll не найден в $BuildDir"
}

# Сборки Revit не копируются: их загружает сам Revit
$excluded = @('RevitAPI.dll', 'RevitAPIUI.dll', 'AdWindows.dll', 'Autodesk.IFC.Export.UI.dll')
Get-ChildItem -Path $BuildDir -Filter *.dll -File |
    Where-Object { $excluded -notcontains $_.Name } |
    Copy-Item -Destination $dll_dir

Copy-Item -Path (Join-Path $ResourcesDir '*') -Destination (Join-Path $dll_dir 'resources') -Recurse

# Путь к сборке в манифесте задаётся относительно расположения .addin
$addin = [System.IO.File]::ReadAllText((Resolve-Path $AddinTemplate))
$addin = [regex]::Replace($addin, '<Assembly>[^<]*</Assembly>', '<Assembly>ifc_exporter\ifc_exporter.dll</Assembly>')
[System.IO.File]::WriteAllText((Join-Path $Destination 'ifc_exporter.addin'), $addin, (New-Object System.Text.UTF8Encoding($false)))

Write-Host "Надстройка разложена в $Destination"
Get-ChildItem -Path $Destination -Recurse -File | ForEach-Object { Write-Host "  $($_.FullName.Substring($Destination.Length))" }
