[CmdletBinding()]
param(
    [ValidateSet('Demo', 'Full')]
    [string] $Edition = 'Full',
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',
    [ValidateSet('waveout', 'dsound', 'auto')]
    [string] $DosBoxAudioDriver = 'waveout',
    [switch] $Clean,
    [switch] $Rebuild,
    [switch] $Test
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$TigreSourceDir = Join-Path $RepositoryRoot 'CODE\TIGRE'
$BamSourceDir = Join-Path $RepositoryRoot 'CODE\SRC'
$GameDirectory = Join-Path $RepositoryRoot 'GAME'
if ($Edition -eq 'Demo') {
    $GameDirectory = Join-Path $GameDirectory 'DEMO'
}
$OutputRoot = Join-Path $PSScriptRoot 'OUT'
$OutputRoot = Join-Path $OutputRoot $Edition
$LocalWatcomRoot = Join-Path $RepositoryRoot 'TOOLS\WATCOM'
$BundledDosBox = Join-Path $RepositoryRoot 'TOOLS\DOSBOX\DOSBox.exe'

$TigreSources = @(
    'Api.cpp', 'ApiDlg.cpp', 'ApiEvt.cpp', 'ApiFont.cpp', 'ApiGraph.cpp',
    'ApiMem.cpp', 'ApiRes.cpp', 'ClipRect.cpp', 'Clock.cpp', 'Comp.cpp',
    'Comm.cpp', 'CommMgr.cpp', 'Config.cpp', 'Context.cpp', 'Debug.cpp',
    'Dialog.cpp', 'Dpmi.cpp', 'EventMgr.cpp', 'FontMgr.cpp', 'File.cpp',
    'GraphMgr.cpp', 'KeyList.cpp', 'List.cpp', 'Manager.cpp', 'Mem.cpp',
    'MemMgr.cpp', 'Mono.cpp', 'Mouse.cpp', 'MouseInt.cpp', 'MouseScr.cpp',
    'Object.cpp', 'ObjMgr.cpp', 'Os.cpp', 'OsGrph.asm', 'Palette.cpp',
    'Periodic.cpp', 'Rect.cpp', 'ResMgr.cpp', 'Resource.cpp', 'SaveBase.cpp',
    'SaveMgr.cpp', 'SConfig.cpp', 'ScrImage.cpp', 'Serial.cpp', 'SoundMgr_SOS.cpp',
    'Stream.cpp', 'Srle.cpp', 'T12.cpp', 'Text.cpp', 'Trle.cpp',
    'VgaBuf.cpp', 'XMODDISP.CPP', 'ModeX.asm'
)

$TigreRelease486Sources = @(
    'Api.cpp', 'ApiDlg.cpp', 'ApiEvt.cpp', 'ApiFont.cpp', 'ApiGraph.cpp',
    'ApiMem.cpp', 'ApiRes.cpp', 'Comm.cpp', 'EventMgr.cpp', 'FontMgr.cpp',
    'KeyList.cpp', 'List.cpp', 'Mem.cpp', 'MemMgr.cpp', 'Object.cpp',
    'Rect.cpp', 'Resource.cpp', 'ScrImage.cpp', 'VgaBuf.cpp'
)

$BamSources = @(
    'Ai.cpp', 'Assess.cpp', 'Bam.cpp', 'BAM_DG.CPP', 'BamFuncs.cpp',
    'BamFunc2.cpp', 'BamGuy.cpp', 'BamPopup.cpp', 'BamRoom.cpp', 'Cine.cpp',
    'Choose.cpp', 'Credits.cpp', 'Death.cpp', 'Encyclo.cpp', 'EncyMenu.cpp',
    'Fade.cpp', 'FlicSmk_SMACKER.cpp', 'FMT_LBM.CPP', 'Hall.cpp', 'INTROHAL.CPP',
    'Items.cpp', 'LegendOp.cpp', 'MainMenu.cpp', 'MakeChar.cpp', 'Maps.cpp',
    'MapsBase.cpp', 'NetChar.cpp', 'NetHall.cpp', 'NetStory.cpp', 'Option.cpp',
    'Option2.cpp', 'Option3.cpp', 'Pather.cpp', 'SaveMenu.cpp', 'Snap.cpp',
    'SpendExp.cpp', 'Story.cpp', 'Tutorial.cpp', 'Units.cpp', 'UnitLib.cpp',
    'ViewPort.cpp', 'WinLose.cpp', 'WinLose2.cpp', 'World.cpp', 'WorldMap.cpp'
)

function Assert-WatcomEnvironment {
    $CompilerOnPath = Get-Command 'wcl386.exe' -ErrorAction SilentlyContinue
    if (-not $CompilerOnPath -and (Test-Path -LiteralPath (Join-Path $LocalWatcomRoot 'binnt\wcl386.exe'))) {
        $env:WATCOM = $LocalWatcomRoot
        $env:PATH = (Join-Path $LocalWatcomRoot 'binnt') + ';' + (Join-Path $LocalWatcomRoot 'binw') + ';' + $env:PATH
        $env:EDPATH = Join-Path $LocalWatcomRoot 'eddat'
        $env:INCLUDE = (Join-Path $LocalWatcomRoot 'h') + ';' + (Join-Path $LocalWatcomRoot 'h\nt')
        Write-Host "Using repository-local Open Watcom 1.9: $LocalWatcomRoot"
    }

    $RequiredTools = @('wcl386.exe', 'wasm.exe', 'wlib.exe', 'wlink.exe')
    $MissingTools = @($RequiredTools | Where-Object { -not (Get-Command $_ -ErrorAction SilentlyContinue) })
    if ($MissingTools.Count -ne 0) {
        throw "The Watcom toolchain is not initialized. Missing from PATH: $($MissingTools -join ', '). See Docs\Building.md."
    }
}

function Invoke-WatcomTool {
    param(
        [Parameter(Mandatory = $true)][string] $Executable,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory
    )

    Push-Location -LiteralPath $WorkingDirectory
    try {
        $ToolOutput = @(& $Executable @Arguments 2>&1)
        $ToolExitCode = $LASTEXITCODE
        if ($ToolExitCode -ne 0) {
            foreach ($Line in $ToolOutput) {
                Write-Host $Line
            }
            throw "$Executable failed with exit code $ToolExitCode."
        }
    }
    finally {
        Pop-Location
    }
}

function Get-NewestHeaderTime {
    param([Parameter(Mandatory = $true)][string[]] $Directories)

    $Headers = foreach ($Directory in $Directories) {
        Get-ChildItem -LiteralPath $Directory -File | Where-Object { $_.Extension -in '.h', '.hpp', '.inc' }
    }
    return ($Headers | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
}

function Remove-BuildOutput {
    param([Parameter(Mandatory = $true)][string] $Component)

    $ComponentRoot = [System.IO.Path]::GetFullPath((Join-Path $OutputRoot "$Component\$Configuration"))
    $ExpectedRoot = [System.IO.Path]::GetFullPath($OutputRoot) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $ComponentRoot.StartsWith($ExpectedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside BUILD\OUT: $ComponentRoot"
    }
    if (Test-Path -LiteralPath $ComponentRoot) {
        Remove-Item -LiteralPath $ComponentRoot -Recurse -Force
    }
}

function Get-PublishedExecutablePath {
    $ExecutableName = if ($Configuration -eq 'Debug') { 'BAMDBG.EXE' } else { 'BAM.EXE' }
    return Join-Path $GameDirectory $ExecutableName
}

function Remove-PublishedExecutable {
    $ExecutablePath = [System.IO.Path]::GetFullPath((Get-PublishedExecutablePath))
    $ExpectedRoot = [System.IO.Path]::GetFullPath($GameDirectory) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $ExecutablePath.StartsWith($ExpectedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside GAME: $ExecutablePath"
    }
    if (Test-Path -LiteralPath $ExecutablePath) {
        Remove-Item -LiteralPath $ExecutablePath -Force
    }
}

function Build-Sources {
    param(
        [Parameter(Mandatory = $true)][string] $SourceDirectory,
        [Parameter(Mandatory = $true)][string] $OutputDirectory,
        [Parameter(Mandatory = $true)][string[]] $Sources,
        [Parameter(Mandatory = $true)][datetime] $NewestHeaderTime,
        [Parameter(Mandatory = $true)][scriptblock] $GetCppFlags
    )

    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    $Objects = @()

    foreach ($SourceName in $Sources) {
        $SourcePath = Join-Path $SourceDirectory $SourceName
        if (-not (Test-Path -LiteralPath $SourcePath)) {
            throw "Source listed by the historical makefile was not found: $SourcePath"
        }

        $ObjectName = [System.IO.Path]::GetFileNameWithoutExtension($SourceName).ToUpperInvariant() + '.OBJ'
        $ObjectPath = Join-Path $OutputDirectory $ObjectName
        $Objects += $ObjectPath
        $SourceTime = (Get-Item -LiteralPath $SourcePath).LastWriteTimeUtc
        $NeedsCompile = -not (Test-Path -LiteralPath $ObjectPath)
        if (-not $NeedsCompile) {
            $ObjectTime = (Get-Item -LiteralPath $ObjectPath).LastWriteTimeUtc
            $NeedsCompile = $ObjectTime -lt $SourceTime -or $ObjectTime -lt $NewestHeaderTime
        }

        if ($NeedsCompile) {
            Write-Host "Compiling $SourceName"
            if ([System.IO.Path]::GetExtension($SourceName) -ieq '.ASM') {
                $Arguments = @('-3r', '-mf', '-zq', "-fo=$ObjectPath", $SourcePath)
                Invoke-WatcomTool -Executable 'wasm.exe' -Arguments $Arguments -WorkingDirectory $OutputDirectory
            }
            else {
                $Flags = @(& $GetCppFlags $SourceName)
                $IncludeOption = "/i=$SourceDirectory;$TigreSourceDir"
                # Watcom 10 treated bool as an extension. Open Watcom has a native bool,
                # while Types.hpp only suppresses its compatibility enum when TRUE exists.
                # The shipped resource maps use byte-packed Watcom structures.
                # Without /zp1, MapIdxRec is larger than the 18-byte MAP.IDX
                # record and the resource manager silently loads zero maps.
                $CompatibilityOptions = @('/dTRUE=1', '/dFALSE=0', '/zp1')
                if ($Edition -eq 'Full') {
                    $CompatibilityOptions += '/dBAM_FULL_GAME=1'
                }
                $Arguments = @('/bt=dos') + $Flags + $CompatibilityOptions + @($IncludeOption, "/fo=$ObjectPath", $SourcePath)
                Invoke-WatcomTool -Executable 'wcl386.exe' -Arguments $Arguments -WorkingDirectory $OutputDirectory
            }
        }
    }

    return $Objects
}

function Build-Tigre {
    $OutputDirectory = Join-Path $OutputRoot "TIGRE\$Configuration"
    $NewestHeaderTime = Get-NewestHeaderTime -Directories @($TigreSourceDir)
    $FlagSelector = {
        param($SourceName)
        if ($Configuration -eq 'Debug') {
            return @('/c', '/j', '/s', '/zq', '/w3', '/d2', '/3r')
        }
        if ($TigreRelease486Sources -contains $SourceName) {
            return @('/c', '/j', '/s', '/zq', '/w3', '/4r')
        }
        return @('/c', '/j', '/s', '/zq', '/w3', '/d1', '/3r')
    }
    $Objects = Build-Sources -SourceDirectory $TigreSourceDir -OutputDirectory $OutputDirectory `
        -Sources $TigreSources -NewestHeaderTime $NewestHeaderTime -GetCppFlags $FlagSelector

    $LibraryPath = Join-Path $OutputDirectory 'TIGRE.LIB'
    $NewestObjectTime = ($Objects | Get-Item | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
    if (-not (Test-Path -LiteralPath $LibraryPath) -or (Get-Item -LiteralPath $LibraryPath).LastWriteTimeUtc -lt $NewestObjectTime) {
        $ResponsePath = Join-Path $OutputDirectory 'TIGRE.RSP'
        $Objects | ForEach-Object { '-+' + [System.IO.Path]::GetFileName($_) } | Set-Content -LiteralPath $ResponsePath -Encoding Ascii
        if (Test-Path -LiteralPath $LibraryPath) {
            Remove-Item -LiteralPath $LibraryPath -Force
        }
        Write-Host 'Creating TIGRE.LIB'
        Invoke-WatcomTool -Executable 'wlib.exe' `
            -Arguments @('/b', '/c', '/l', '/n', '/p=32', '/q', 'TIGRE.LIB', '@TIGRE.RSP') `
            -WorkingDirectory $OutputDirectory
    }
}

function Build-Bam {
    $TigreLibrary = Join-Path $OutputRoot "TIGRE\$Configuration\TIGRE.LIB"
    if (-not (Test-Path -LiteralPath $TigreLibrary)) {
        Build-Tigre
    }

    $OutputDirectory = Join-Path $OutputRoot "BAM\$Configuration"
    $RuntimeFiles = @('DOS4GW.EXE', 'MAIN.STF', 'MAIN.MAP', 'MAP.IDX', 'RES_CFG.HPP', 'SOUND.CFG')
    if ($Edition -eq 'Full') {
        $RuntimeFiles += @('CINES.MAP', 'STFS\CINES.STF')
    }
    foreach ($RuntimeFile in $RuntimeFiles) {
        $RuntimePath = Join-Path $GameDirectory $RuntimeFile
        if (-not (Test-Path -LiteralPath $RuntimePath)) {
            throw "A required runtime file was not found: $RuntimePath"
        }
    }
    $NewestHeaderTime = Get-NewestHeaderTime -Directories @($BamSourceDir, $TigreSourceDir)
    $FlagSelector = {
        param($SourceName)
        if ($Configuration -eq 'Debug') {
            return @('/c', '/j', '/zq', '/w9', '/d2', '/3r', '/fm')
        }
        return @('/c', '/j', '/s', '/zq', '/w3', '/4r', '/fm')
    }
    $Objects = Build-Sources -SourceDirectory $BamSourceDir -OutputDirectory $OutputDirectory `
        -Sources $BamSources -NewestHeaderTime $NewestHeaderTime -GetCppFlags $FlagSelector

    $Libraries = @('SOSDW1CR.LIB', 'SOSMW1CR.LIB', 'SMACK.LIB')
    Copy-Item -LiteralPath $TigreLibrary -Destination (Join-Path $OutputDirectory 'TIGRE.LIB') -Force
    foreach ($Library in $Libraries) {
        Copy-Item -LiteralPath (Join-Path $TigreSourceDir $Library) -Destination (Join-Path $OutputDirectory $Library) -Force
    }

    $ExecutablePath = Join-Path $OutputDirectory 'BAM.EXE'
    $LinkInputs = $Objects + @((Join-Path $OutputDirectory 'TIGRE.LIB')) + @($Libraries | ForEach-Object { Join-Path $OutputDirectory $_ })
    $NewestInputTime = ($LinkInputs | Get-Item | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
    if (-not (Test-Path -LiteralPath $ExecutablePath) -or (Get-Item -LiteralPath $ExecutablePath).LastWriteTimeUtc -lt $NewestInputTime) {
        $ResponsePath = Join-Path $OutputDirectory 'BAM.LNK'
        $LinkCommands = @('system dos4g', 'debug all', 'option map=BAM.MAP', 'option stack=8k', 'name BAM.EXE')
        $LinkCommands += $Objects | ForEach-Object { 'file ' + [System.IO.Path]::GetFileName($_) }
        $LinkCommands += 'library TIGRE.LIB'
        $LinkCommands += $Libraries | ForEach-Object { 'library ' + $_ }
        $LinkCommands | Set-Content -LiteralPath $ResponsePath -Encoding Ascii
        Write-Host 'Linking BAM.EXE'
        Invoke-WatcomTool -Executable 'wlink.exe' -Arguments @('@BAM.LNK') -WorkingDirectory $OutputDirectory
    }

    $PublishedExecutablePath = Get-PublishedExecutablePath
    Copy-Item -LiteralPath $ExecutablePath -Destination $PublishedExecutablePath -Force
    Write-Host "Published $PublishedExecutablePath"
}

function Start-BamInDosBox {
    if (Test-Path -LiteralPath $BundledDosBox) {
        $DosBoxPath = $BundledDosBox
    }
    else {
        $DosBox = Get-Command 'dosbox.exe' -ErrorAction SilentlyContinue
        if (-not $DosBox) {
            $DosBox = Get-Command 'dosbox' -ErrorAction SilentlyContinue
        }
        if (-not $DosBox) {
            throw 'Bundled DOSBox is missing and DOSBox was not found on PATH.'
        }
        $DosBoxPath = $DosBox.Source
    }

    $ExecutablePath = Get-PublishedExecutablePath
    if (-not (Test-Path -LiteralPath $ExecutablePath)) {
        throw "The game executable was not found after building: $ExecutablePath"
    }

    $ExecutableName = Split-Path -Leaf $ExecutablePath
    $MountCommand = 'mount c "' + $GameDirectory + '"'
    Write-Host "Starting $ExecutablePath in DOSBox"
    # SDL 1.2 calls its DirectSound backend "dsound", not "directsound".
    # Use waveOut by default and isolate the override to this child process.
    $PreviousAudioDriver = [Environment]::GetEnvironmentVariable('SDL_AUDIODRIVER', 'Process')
    try {
        if ($DosBoxAudioDriver -eq 'auto') {
            [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', $null, 'Process')
        }
        else {
            [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', $DosBoxAudioDriver, 'Process')
        }
        & $DosBoxPath -conf (Join-Path $PSScriptRoot 'DOSBOX.CONF') -c $MountCommand -c 'c:' -c $ExecutableName -c 'exit'
    }
    finally {
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', $PreviousAudioDriver, 'Process')
    }
    $ExitCodeVariable = Get-Variable -Name LASTEXITCODE -ErrorAction SilentlyContinue
    if ($ExitCodeVariable -and $ExitCodeVariable.Value -ne 0) {
        throw "DOSBox exited with code $($ExitCodeVariable.Value)."
    }
}

if ($Clean -or $Rebuild) {
    Remove-BuildOutput -Component 'BAM'
    Remove-BuildOutput -Component 'TIGRE'
    Remove-PublishedExecutable
    if ($Clean -and -not $Rebuild) {
        return
    }
}

Assert-WatcomEnvironment
Build-Tigre
Build-Bam
if ($Test) {
    Start-BamInDosBox
}
