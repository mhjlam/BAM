[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',
    [switch] $Clean,
    [switch] $Rebuild,
    [switch] $Test
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$TigreSourceDir = Join-Path $RepositoryRoot 'Source\Tigre'
$BamSourceDir = Join-Path $RepositoryRoot 'Source\Bam'
$SharedSourceDir = Join-Path $RepositoryRoot 'Source\Shared'
$OutputRoot = Join-Path $PSScriptRoot 'Out'
$LocalWatcomRoot = Join-Path $RepositoryRoot 'Tools\open-watcom-1.9'

$TigreSources = @(
    'Api.cpp', 'ApiDlg.cpp', 'ApiEvt.cpp', 'ApiFont.cpp', 'ApiGraph.cpp',
    'ApiMem.cpp', 'ApiRes.cpp', 'ClipRect.cpp', 'Clock.cpp', 'Comp.cpp',
    'Comm.cpp', 'CommMgr.cpp', 'Config.cpp', 'Context.cpp', 'Debug.cpp',
    'Dialog.cpp', 'Dpmi.cpp', 'EventMgr.cpp', 'FontMgr.cpp', 'File.cpp',
    'GraphMgr.cpp', 'KeyList.cpp', 'List.cpp', 'Manager.cpp', 'Mem.cpp',
    'MemMgr.cpp', 'Mono.cpp', 'Mouse.cpp', 'MouseInt.cpp', 'MouseScr.cpp',
    'Object.cpp', 'ObjMgr.cpp', 'Os.cpp', 'OsGrph.asm', 'Palette.cpp',
    'Periodic.cpp', 'Rect.cpp', 'ResMgr.cpp', 'Resource.cpp', 'SaveBase.cpp',
    'SaveMgr.cpp', 'SConfig.cpp', 'ScrImage.cpp', 'Serial.cpp', 'SoundMgr.cpp',
    'Stream.cpp', 'Srle.cpp', 'T12.cpp', 'Text.cpp', 'Trle.cpp',
    'VgaBuf.cpp', 'XModeDisp.cpp', 'ModeX.asm'
)

$TigreRelease486Sources = @(
    'Api.cpp', 'ApiDlg.cpp', 'ApiEvt.cpp', 'ApiFont.cpp', 'ApiGraph.cpp',
    'ApiMem.cpp', 'ApiRes.cpp', 'Comm.cpp', 'EventMgr.cpp', 'FontMgr.cpp',
    'KeyList.cpp', 'List.cpp', 'Mem.cpp', 'MemMgr.cpp', 'Object.cpp',
    'Rect.cpp', 'Resource.cpp', 'ScrImage.cpp', 'VgaBuf.cpp'
)

$BamSources = @(
    'Ai.cpp', 'Assess.cpp', 'Bam.cpp', 'BamDg.cpp', 'BamFuncs.cpp',
    'BamFunc2.cpp', 'BamGuy.cpp', 'BamPopup.cpp', 'BamRoom.cpp', 'Cine.cpp',
    'Choose.cpp', 'Credits.cpp', 'Death.cpp', 'Encyclo.cpp', 'EncyMenu.cpp',
    'Fade.cpp', 'FlicSmk.cpp', 'FmtLbm.cpp', 'Hall.cpp', 'IntroHall.cpp',
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
        throw "Refusing to clean a path outside Build\Out: $ComponentRoot"
    }
    if (Test-Path -LiteralPath $ComponentRoot) {
        Remove-Item -LiteralPath $ComponentRoot -Recurse -Force
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

        $ObjectName = [System.IO.Path]::GetFileNameWithoutExtension($SourceName) + '.obj'
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
                $IncludeOption = "/i=$SourceDirectory;$TigreSourceDir;$SharedSourceDir"
                # Watcom 10 treated bool as an extension. Open Watcom has a native bool,
                # while Types.hpp only suppresses its compatibility enum when TRUE exists.
                # The shipped resource maps use byte-packed Watcom structures.
                # Without /zp1, MapIdxRec is larger than the 18-byte MAP.IDX
                # record and the resource manager silently loads zero maps.
                $CompatibilityOptions = @('/dTRUE=1', '/dFALSE=0', '/zp1')
                $Arguments = @('/bt=dos') + $Flags + $CompatibilityOptions + @($IncludeOption, "/fo=$ObjectPath", $SourcePath)
                Invoke-WatcomTool -Executable 'wcl386.exe' -Arguments $Arguments -WorkingDirectory $OutputDirectory
            }
        }
    }

    return $Objects
}

function Build-Tigre {
    $OutputDirectory = Join-Path $OutputRoot "Tigre\$Configuration"
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

    $LibraryPath = Join-Path $OutputDirectory 'Tigre.lib'
    $NewestObjectTime = ($Objects | Get-Item | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
    if (-not (Test-Path -LiteralPath $LibraryPath) -or (Get-Item -LiteralPath $LibraryPath).LastWriteTimeUtc -lt $NewestObjectTime) {
        $ResponsePath = Join-Path $OutputDirectory 'Tigre.rsp'
        $Objects | ForEach-Object { '-+' + [System.IO.Path]::GetFileName($_) } | Set-Content -LiteralPath $ResponsePath -Encoding Ascii
        if (Test-Path -LiteralPath $LibraryPath) {
            Remove-Item -LiteralPath $LibraryPath -Force
        }
        Write-Host 'Creating Tigre.lib'
        Invoke-WatcomTool -Executable 'wlib.exe' `
            -Arguments @('/b', '/c', '/l', '/n', '/p=32', '/q', 'Tigre.lib', '@Tigre.rsp') `
            -WorkingDirectory $OutputDirectory
    }
}

function Build-Bam {
    $TigreLibrary = Join-Path $OutputRoot "Tigre\$Configuration\Tigre.lib"
    if (-not (Test-Path -LiteralPath $TigreLibrary)) {
        Build-Tigre
    }

    $OutputDirectory = Join-Path $OutputRoot "Bam\$Configuration"
    $RuntimeFiles = @('Dos4gw.exe', 'Main.stf', 'Main.map', 'Map.idx', 'ResCfg.hpp', 'Sound.cfg')
    foreach ($RuntimeFile in $RuntimeFiles) {
        $RuntimePath = Join-Path $RepositoryRoot "Game\$RuntimeFile"
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

    $Libraries = @()
    Copy-Item -LiteralPath $TigreLibrary -Destination (Join-Path $OutputDirectory 'Tigre.lib') -Force
    foreach ($Library in $Libraries) {
        Copy-Item -LiteralPath (Join-Path $TigreSourceDir $Library) -Destination (Join-Path $OutputDirectory $Library) -Force
    }

    $ExecutablePath = Join-Path $OutputDirectory 'Bam.exe'
    $LinkInputs = $Objects + @((Join-Path $OutputDirectory 'Tigre.lib')) + @($Libraries | ForEach-Object { Join-Path $OutputDirectory $_ })
    $NewestInputTime = ($LinkInputs | Get-Item | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
    if (-not (Test-Path -LiteralPath $ExecutablePath) -or (Get-Item -LiteralPath $ExecutablePath).LastWriteTimeUtc -lt $NewestInputTime) {
        $ResponsePath = Join-Path $OutputDirectory 'Bam.lnk'
        $LinkCommands = @('system dos4g', 'debug all', 'option map=Bam.map', 'option stack=8k', 'name Bam.exe')
        $LinkCommands += $Objects | ForEach-Object { 'file ' + [System.IO.Path]::GetFileName($_) }
        $LinkCommands += 'library Tigre.lib'
        $LinkCommands | Set-Content -LiteralPath $ResponsePath -Encoding Ascii
        Write-Host 'Linking Bam.exe'
        Invoke-WatcomTool -Executable 'wlink.exe' -Arguments @('@Bam.lnk') -WorkingDirectory $OutputDirectory
    }

    # Stage the compact shipped runtime set so Bam.exe can be launched directly
    # from its output directory. Main.stf contains the packed game resources;
    # the much larger raw asset tree is intentionally not duplicated.
    foreach ($RuntimeFile in $RuntimeFiles) {
        Copy-Item -LiteralPath (Join-Path $RepositoryRoot "Game\$RuntimeFile") `
            -Destination (Join-Path $OutputDirectory $RuntimeFile) -Force
    }
}

function Start-BamInDosBox {
    $DosBox = Get-Command 'dosbox.exe' -ErrorAction SilentlyContinue
    if (-not $DosBox) {
        $DosBox = Get-Command 'dosbox' -ErrorAction SilentlyContinue
    }
    if (-not $DosBox) {
        throw 'DOSBox was not found on PATH.'
    }

    $OutputDirectory = Join-Path $OutputRoot "Bam\$Configuration"
    $ExecutablePath = Join-Path $OutputDirectory 'Bam.exe'
    if (-not (Test-Path -LiteralPath $ExecutablePath)) {
        throw "The game executable was not found after building: $ExecutablePath"
    }

    $MountCommand = 'mount c "' + $OutputDirectory + '"'
    Write-Host "Starting $ExecutablePath in DOSBox"
    & $DosBox.Source -c $MountCommand -c 'c:' -c 'Bam.exe' -c 'exit'
    $ExitCodeVariable = Get-Variable -Name LASTEXITCODE -ErrorAction SilentlyContinue
    if ($ExitCodeVariable -and $ExitCodeVariable.Value -ne 0) {
        throw "DOSBox exited with code $($ExitCodeVariable.Value)."
    }
}

if ($Clean -or $Rebuild) {
    Remove-BuildOutput -Component 'Bam'
    Remove-BuildOutput -Component 'Tigre'
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
