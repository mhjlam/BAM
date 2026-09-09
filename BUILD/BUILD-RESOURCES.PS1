[CmdletBinding()]
param(
    [switch] $Publish,
    [switch] $Clean
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$GameDirectory = Join-Path $RepositoryRoot 'GAME'
$DataDirectory = Join-Path $GameDirectory 'DATA'
$ResourceToolsDirectory = Join-Path $RepositoryRoot 'TOOLS\RESOURCE'
$DosBoxPath = Join-Path $RepositoryRoot 'TOOLS\DOSBOX\DOSBox.exe'
$OutputDirectory = Join-Path $PSScriptRoot 'OUT\RESOURCES'
$StageDirectory = Join-Path $OutputDirectory 'STAGE'
$ResultFiles = @('MAIN.STF', 'MAIN.MAP', 'MAP.IDX')

function Remove-ResourceOutput {
    $ResolvedOutput = [System.IO.Path]::GetFullPath($OutputDirectory)
    $ExpectedRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'OUT')) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $ResolvedOutput.StartsWith($ExpectedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside BUILD\OUT: $ResolvedOutput"
    }
    if (Test-Path -LiteralPath $ResolvedOutput) {
        Remove-Item -LiteralPath $ResolvedOutput -Recurse -Force
    }
}

Remove-ResourceOutput
if ($Clean) {
    return
}

$RequiredFiles = @(
    (Join-Path $ResourceToolsDirectory 'MAKESTUF.EXE'),
    (Join-Path $ResourceToolsDirectory 'STUFF.DAT'),
    (Join-Path $GameDirectory 'RES_CFG.HPP'),
    (Join-Path $GameDirectory 'DOS4GW.EXE'),
    $DosBoxPath
)
foreach ($RequiredFile in $RequiredFiles) {
    if (-not (Test-Path -LiteralPath $RequiredFile)) {
        throw "A required resource-build file was not found: $RequiredFile"
    }
}

$ManifestNames = @(Get-Content -LiteralPath (Join-Path $ResourceToolsDirectory 'STUFF.DAT') | ForEach-Object {
    if ($_ -match '^\s*([0-9]+\.[A-Za-z0-9]+)\s*$') {
        $Matches[1].ToUpperInvariant()
    }
})
$AssetNames = @(Get-ChildItem -LiteralPath $DataDirectory -File | ForEach-Object { $_.Name.ToUpperInvariant() })
$MissingAssets = @($ManifestNames | Where-Object { $_ -notin $AssetNames })
if ($MissingAssets.Count -ne 0) {
    throw "STUFF.DAT references missing assets: $($MissingAssets -join ', ')"
}

New-Item -ItemType Directory -Path $StageDirectory -Force | Out-Null
Get-ChildItem -LiteralPath $DataDirectory -File | Copy-Item -Destination $StageDirectory
Copy-Item -LiteralPath (Join-Path $ResourceToolsDirectory 'MAKESTUF.EXE') -Destination $StageDirectory
Copy-Item -LiteralPath (Join-Path $ResourceToolsDirectory 'STUFF.DAT') -Destination $StageDirectory
Copy-Item -LiteralPath (Join-Path $GameDirectory 'RES_CFG.HPP') -Destination $StageDirectory
Copy-Item -LiteralPath (Join-Path $GameDirectory 'DOS4GW.EXE') -Destination $StageDirectory

$ExistingDosBoxIds = @(Get-Process -Name 'DOSBox' -ErrorAction SilentlyContinue | ForEach-Object { $_.Id })
$MountCommand = 'mount c "' + $StageDirectory + '"'
Write-Host "Packing $($ManifestNames.Count) resources with MAKESTUF.EXE"
& $DosBoxPath -c $MountCommand -c 'c:' -c 'MAKESTUF.EXE > PACK.LOG' -c 'exit'

$Deadline = [DateTime]::UtcNow.AddMinutes(3)
do {
    Start-Sleep -Milliseconds 250
    $ResourcePackers = @(Get-Process -Name 'DOSBox' -ErrorAction SilentlyContinue |
        Where-Object { $_.Id -notin $ExistingDosBoxIds })
} while ($ResourcePackers.Count -ne 0 -and [DateTime]::UtcNow -lt $Deadline)

if ($ResourcePackers.Count -ne 0) {
    foreach ($Process in $ResourcePackers) {
        Stop-Process -Id $Process.Id -Force
    }
    throw 'MAKESTUF did not finish within three minutes.'
}

foreach ($ResultFile in $ResultFiles) {
    $ResultPath = Join-Path $StageDirectory $ResultFile
    if (-not (Test-Path -LiteralPath $ResultPath)) {
        throw "MAKESTUF did not create $ResultFile. See BUILD\OUT\RESOURCES\STAGE\PACK.LOG."
    }
    Copy-Item -LiteralPath $ResultPath -Destination $OutputDirectory
}
Copy-Item -LiteralPath (Join-Path $StageDirectory 'PACK.LOG') -Destination $OutputDirectory

$PackLog = Get-Content -LiteralPath (Join-Path $OutputDirectory 'PACK.LOG') -Raw
if ($PackLog -notmatch 'Operation complete\.') {
    throw 'MAKESTUF did not report successful completion. See BUILD\OUT\RESOURCES\PACK.LOG.'
}

Remove-Item -LiteralPath $StageDirectory -Recurse -Force

if ($Publish) {
    foreach ($ResultFile in $ResultFiles) {
        Copy-Item -LiteralPath (Join-Path $OutputDirectory $ResultFile) -Destination $GameDirectory -Force
    }
    Write-Host "Published MAIN.STF, MAIN.MAP, and MAP.IDX to $GameDirectory"
}
else {
    Write-Host "Generated resource archive under $OutputDirectory"
    Write-Host 'Use -Publish to replace the runnable copies under GAME.'
}
