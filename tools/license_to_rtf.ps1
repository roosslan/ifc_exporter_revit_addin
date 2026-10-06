<#
    Преобразование текстового файла лицензии в RTF для диалога лицензии MSI (WixSharp LicenceFile).
#>
param(
    [string] $Source,
    [Parameter(Mandatory = $true)] [string] $Destination
)

$ErrorActionPreference = 'Stop'

# В Windows PowerShell 5.1 $PSScriptRoot пуст в значениях параметров по умолчанию
if (-not $Source) { $Source = Join-Path $PSScriptRoot '..\LICENSE' }

function ConvertTo-RtfText([string] $line) {
    $sb = New-Object System.Text.StringBuilder
    foreach ($ch in $line.ToCharArray()) {
        $code = [int] $ch
        if ($ch -eq '\' -or $ch -eq '{' -or $ch -eq '}') {
            [void] $sb.Append('\').Append($ch)
        }
        elseif ($code -eq 9) {
            [void] $sb.Append('\tab ')
        }
        elseif ($code -gt 127) {
            # \uN — знаковое 16-битное значение, '?' — замена для программ без Unicode
            if ($code -gt 32767) { $code -= 65536 }
            [void] $sb.Append('\u').Append($code).Append('?')
        }
        else {
            [void] $sb.Append($ch)
        }
    }
    return $sb.ToString()
}

$lines = [System.IO.File]::ReadAllLines((Resolve-Path $Source), [System.Text.Encoding]::UTF8)

$rtf = New-Object System.Text.StringBuilder
[void] $rtf.AppendLine('{\rtf1\ansi\ansicpg1252\deff0{\fonttbl{\f0\fmodern Consolas;}}')
[void] $rtf.AppendLine('\viewkind4\uc1\pard\f0\fs16')
foreach ($line in $lines) {
    [void] $rtf.Append((ConvertTo-RtfText $line)).AppendLine('\par')
}
[void] $rtf.AppendLine('}')

[System.IO.File]::WriteAllText($Destination, $rtf.ToString(), [System.Text.Encoding]::ASCII)
Write-Host "Создан $Destination ($($lines.Count) строк)"
