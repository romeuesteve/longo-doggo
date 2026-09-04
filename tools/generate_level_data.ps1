[CmdletBinding()]
param(
    [string]$ManifestPath = (Join-Path $PSScriptRoot '..\recovered\analysis\room-manifest.txt'),
    [string]$AssetsPath = (Join-Path $PSScriptRoot '..\recovered\exported-assets'),
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\src\level_data.h')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$GridUnit = 16
$PlayableRoomNames = @(
    'rm_tutorial',
    'rm_level1',
    'rm_level2',
    'rm_level3',
    'rm_level4',
    'rm_level5',
    'rm_level6'
)
$MetadataRoomNames = @('rm_title_screen', 'rm_credits', 'rm_editor')

if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw "Room manifest not found: $ManifestPath"
}
if (-not (Test-Path -LiteralPath $AssetsPath -PathType Container)) {
    throw "Recovered asset directory not found: $AssetsPath"
}

function Parse-InvariantNumber {
    param([Parameter(Mandatory = $true)][string]$Text)

    return [double]::Parse($Text.Trim(), [Globalization.CultureInfo]::InvariantCulture)
}

function Parse-CommaDecimal {
    param(
        [Parameter(Mandatory = $true)][string]$Whole,
        [Parameter(Mandatory = $true)][string]$Fraction
    )

    return Parse-InvariantNumber ('{0}.{1}' -f $Whole.Trim(), $Fraction.Trim())
}

function Parse-ManifestScale {
    param([Parameter(Mandatory = $true)][string]$RawScale)

    # The recovered manifest was written with a comma decimal separator while
    # also using commas between x/y. Thus (1,12,5) means (1,12.5), while
    # (8,5,1) means (8.5,1), and (0,6666667,0,6666667) means (0.6666667,0.6666667).
    $parts = @($RawScale.Split(',') | ForEach-Object { $_.Trim() })
    switch ($parts.Count) {
        2 {
            return @(
                (Parse-InvariantNumber $parts[0]),
                (Parse-InvariantNumber $parts[1])
            )
        }
        3 {
            if ($parts[0] -eq '1' -and $parts[2] -ne '1') {
                return @(
                    (Parse-InvariantNumber $parts[0]),
                    (Parse-CommaDecimal $parts[1] $parts[2])
                )
            }
            if ($parts[2] -eq '1') {
                return @(
                    (Parse-CommaDecimal $parts[0] $parts[1]),
                    (Parse-InvariantNumber $parts[2])
                )
            }
            throw "Ambiguous three-part scale '$RawScale'."
        }
        4 {
            return @(
                (Parse-CommaDecimal $parts[0] $parts[1]),
                (Parse-CommaDecimal $parts[2] $parts[3])
            )
        }
        default {
            throw "Unsupported scale '$RawScale'."
        }
    }
}

function ConvertTo-CStringLiteral {
    param([Parameter(Mandatory = $true)][string]$Text)

    $escaped = $Text.Replace('\', '\\').Replace('"', '\"')
    return '"' + $escaped + '"'
}

function Get-RoomSymbol {
    param([Parameter(Mandatory = $true)][string]$RoomName)

    return 'longo_level_' + $RoomName.Substring(3)
}

function Get-IntegerFootprint {
    param(
        [Parameter(Mandatory = $true)][double]$Scale,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $value = [math]::Round($GridUnit * $Scale, 0, [MidpointRounding]::AwayFromZero)
    if ($value -lt 1) {
        throw "Scale for $Description produced an invalid footprint: $Scale"
    }
    return [int]$value
}

$kindByObject = @{
    'oBlock' = 'LEVEL_OBJECT_BLOCK'
    'oBox' = 'LEVEL_OBJECT_BOX'
    'oApple' = 'LEVEL_OBJECT_APPLE'
    'oPear' = 'LEVEL_OBJECT_PEAR'
    'oSkull' = 'LEVEL_OBJECT_SKULL'
    'oButton' = 'LEVEL_OBJECT_BUTTON'
    'oDoor' = 'LEVEL_OBJECT_DOOR'
    'oHole' = 'LEVEL_OBJECT_HOLE'
    'oDog' = 'LEVEL_OBJECT_DOG'
    'oDogPart' = 'LEVEL_OBJECT_DOG_PART'
    'oGoal' = 'LEVEL_OBJECT_GOAL'
    'oGoalUp' = 'LEVEL_OBJECT_GOAL_UP'
    'oWin' = 'LEVEL_OBJECT_WIN'
    'oHouseSpawner' = 'LEVEL_OBJECT_HOUSE_SPAWNER'
    'oFlower' = 'LEVEL_OBJECT_FLOWER'
    'oButterfly' = 'LEVEL_OBJECT_BUTTERFLY'
    'oTutorial' = 'LEVEL_OBJECT_TUTORIAL'
    'oShadows' = 'LEVEL_OBJECT_SHADOWS'
    'oTitle' = 'LEVEL_OBJECT_TITLE'
    'oTransition' = 'LEVEL_OBJECT_TRANSITION'
    'oMouse' = 'LEVEL_OBJECT_MOUSE'
    'oOne' = 'LEVEL_OBJECT_ONE'
    'oSmoke' = 'LEVEL_OBJECT_SMOKE'
    'oBark' = 'LEVEL_OBJECT_BARK'
    'oDogSpawner' = 'LEVEL_OBJECT_DOG_SPAWNER'
    'oStupidBlock' = 'LEVEL_OBJECT_STUPID_BLOCK'
    'oPostEffects' = 'LEVEL_OBJECT_POST_EFFECTS'
    'obj_bloom_appsrf' = 'LEVEL_OBJECT_BLOOM_SURFACE'
    'par_module' = 'LEVEL_OBJECT_PARTICLE_MODULE'
}

$lines = Get-Content -LiteralPath $ManifestPath
$rooms = [ordered]@{}
$currentRoom = $null
$roomPattern = [regex]::new('^ROOM\s+(\d+):\s+(\S+)\s*$')
$sizePattern = [regex]::new('^\s+size=(\d+)x(\d+)')
$instancePattern = [regex]::new('^\s+INSTANCE\s+id=(\d+)\s+object=(\S+)\s+x=(-?\d+)\s+y=(-?\d+)\s+scale=\(([^)]*)\).*imageIndex=(-?\d+(?:\.\d+)?)')

foreach ($line in $lines) {
    $roomMatch = $roomPattern.Match($line)
    if ($roomMatch.Success) {
        $currentRoom = [pscustomobject]@{
            Index = [int]$roomMatch.Groups[1].Value
            Name = $roomMatch.Groups[2].Value
            Width = 0
            Height = 0
            Objects = New-Object 'System.Collections.Generic.List[object]'
        }
        $rooms[$currentRoom.Name] = $currentRoom
        continue
    }

    if ($null -eq $currentRoom) {
        continue
    }

    $sizeMatch = $sizePattern.Match($line)
    if ($sizeMatch.Success) {
        $currentRoom.Width = [int]$sizeMatch.Groups[1].Value
        $currentRoom.Height = [int]$sizeMatch.Groups[2].Value
        continue
    }

    $instanceMatch = $instancePattern.Match($line)
    if ($instanceMatch.Success) {
        $currentRoom.Objects.Add([pscustomobject]@{
            Id = [int]$instanceMatch.Groups[1].Value
            ObjectName = $instanceMatch.Groups[2].Value
            X = [int]$instanceMatch.Groups[3].Value
            Y = [int]$instanceMatch.Groups[4].Value
            RawScale = $instanceMatch.Groups[5].Value
            Frame = [int][math]::Round((Parse-InvariantNumber $instanceMatch.Groups[6].Value), 0, [MidpointRounding]::AwayFromZero)
        })
    }
}

foreach ($roomName in ($PlayableRoomNames + $MetadataRoomNames)) {
    if (-not $rooms.Contains($roomName)) {
        throw "Required room '$roomName' was not found in $ManifestPath"
    }
    if ($rooms[$roomName].Width -le 0 -or $rooms[$roomName].Height -le 0) {
        throw "Room '$roomName' has no valid size in $ManifestPath"
    }
}

$dogLength = 5
$dogCreatePath = Join-Path $AssetsPath 'code\gml_Object_oDog_Create_0.gml'
if (Test-Path -LiteralPath $dogCreatePath -PathType Leaf) {
    $dogCreateText = Get-Content -Raw -LiteralPath $dogCreatePath
    $dogLengthMatch = [regex]::Match($dogCreateText, '(?m)^\s*length\s*=\s*(\d+)\s*;')
    if ($dogLengthMatch.Success) {
        $dogLength = [int]$dogLengthMatch.Groups[1].Value
    }
}

$kindTokens = [System.Collections.Generic.List[string]]::new()
foreach ($roomName in ($PlayableRoomNames + $MetadataRoomNames)) {
    foreach ($sourceObject in $rooms[$roomName].Objects) {
        if (-not $kindByObject.ContainsKey($sourceObject.ObjectName)) {
            $kindTokens.Add($sourceObject.ObjectName)
        }
    }
}
if ($kindTokens.Count -gt 0) {
    $unknownKinds = ($kindTokens | Sort-Object -Unique) -join ', '
    Write-Warning "Unmapped source objects will use LEVEL_OBJECT_UNKNOWN: $unknownKinds"
}

$outputDirectory = Split-Path -Parent $OutputPath
if (-not (Test-Path -LiteralPath $outputDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}

$output = New-Object 'System.Collections.Generic.List[string]'
$output.Add('/*')
$output.Add(' * Generated by tools/generate_level_data.ps1.')
$output.Add(' * Source: recovered/analysis/room-manifest.txt and recovered/exported-assets.')
$output.Add(' *')
$output.Add(' * Coordinates and footprints use the original GameMaker room space. The')
$output.Add(' * gameplay grid is 16 world units per cell (the original art is 16 px).')
$output.Add(' * x/y are instance origins from the manifest. width/height are logical')
$output.Add(' * footprints derived from the instance scale and rounded to whole world units.')
$output.Add(' *')
$output.Add(' * The data has internal linkage so this header can be included by any C99')
$output.Add(' * translation unit without requiring a separate .c definition file.')
$output.Add(' */')
$output.Add('#ifndef LONGO_DOGGO_LEVEL_DATA_H')
$output.Add('#define LONGO_DOGGO_LEVEL_DATA_H')
$output.Add('')
$output.Add('#define LONGO_GRID_UNIT 16')
$output.Add('')
$output.Add('typedef enum LevelObjectKind {')
$output.Add('    LEVEL_OBJECT_NONE = 0,')
$output.Add('    LEVEL_OBJECT_BLOCK,')
$output.Add('    LEVEL_OBJECT_BOX,')
$output.Add('    LEVEL_OBJECT_APPLE,')
$output.Add('    LEVEL_OBJECT_PEAR,')
$output.Add('    LEVEL_OBJECT_SKULL,')
$output.Add('    LEVEL_OBJECT_BUTTON,')
$output.Add('    LEVEL_OBJECT_DOOR,')
$output.Add('    LEVEL_OBJECT_HOLE,')
$output.Add('    LEVEL_OBJECT_DOG,')
$output.Add('    LEVEL_OBJECT_DOG_PART,')
$output.Add('    LEVEL_OBJECT_GOAL,')
$output.Add('    LEVEL_OBJECT_GOAL_UP,')
$output.Add('    LEVEL_OBJECT_WIN,')
$output.Add('    LEVEL_OBJECT_HOUSE_SPAWNER,')
$output.Add('    LEVEL_OBJECT_FLOWER,')
$output.Add('    LEVEL_OBJECT_BUTTERFLY,')
$output.Add('    LEVEL_OBJECT_TUTORIAL,')
$output.Add('    LEVEL_OBJECT_SHADOWS,')
$output.Add('    LEVEL_OBJECT_TITLE,')
$output.Add('    LEVEL_OBJECT_TRANSITION,')
$output.Add('    LEVEL_OBJECT_MOUSE,')
$output.Add('    LEVEL_OBJECT_ONE,')
$output.Add('    LEVEL_OBJECT_SMOKE,')
$output.Add('    LEVEL_OBJECT_BARK,')
$output.Add('    LEVEL_OBJECT_DOG_SPAWNER,')
$output.Add('    LEVEL_OBJECT_STUPID_BLOCK,')
$output.Add('    LEVEL_OBJECT_POST_EFFECTS,')
$output.Add('    LEVEL_OBJECT_BLOOM_SURFACE,')
$output.Add('    LEVEL_OBJECT_PARTICLE_MODULE,')
$output.Add('    LEVEL_OBJECT_UNKNOWN')
$output.Add('} LevelObjectKind;')
$output.Add('')
$output.Add('typedef struct LevelObject {')
$output.Add('    LevelObjectKind kind;')
$output.Add('    int x;')
$output.Add('    int y;')
$output.Add('    int width;')
$output.Add('    int height;')
$output.Add('    int frame;')
$output.Add('} LevelObject;')
$output.Add('')
$output.Add('typedef struct LevelData {')
$output.Add('    const char *name;')
$output.Add('    int width;')
$output.Add('    int height;')
$output.Add('    const LevelObject *objects;')
$output.Add('    int object_count;')
$output.Add('    int dog_length;')
$output.Add('} LevelData;')
$output.Add('')

foreach ($roomName in $PlayableRoomNames) {
    $room = $rooms[$roomName]
    $symbol = Get-RoomSymbol $roomName
    $output.Add("static const LevelObject ${symbol}_objects[] = {")
    foreach ($sourceObject in $room.Objects) {
        $scale = @(Parse-ManifestScale $sourceObject.RawScale)
        $kind = if ($kindByObject.ContainsKey($sourceObject.ObjectName)) {
            $kindByObject[$sourceObject.ObjectName]
        } else {
            'LEVEL_OBJECT_UNKNOWN'
        }
        $width = Get-IntegerFootprint $scale[0] "$roomName/$($sourceObject.ObjectName)#$($sourceObject.Id)"
        $height = Get-IntegerFootprint $scale[1] "$roomName/$($sourceObject.ObjectName)#$($sourceObject.Id)"
        $output.Add(('    {{ {0}, {1}, {2}, {3}, {4}, {5} }},' -f $kind, $sourceObject.X, $sourceObject.Y, $width, $height, $sourceObject.Frame))
    }
    $output.Add('};')
    $output.Add('')
    $output.Add("static const LevelData ${symbol} = {")
    $output.Add(('    {0}, {1}, {2}, {3}_objects, (int)(sizeof({3}_objects) / sizeof({3}_objects[0])), {4}' -f (ConvertTo-CStringLiteral $room.Name), $room.Width, $room.Height, $symbol, $dogLength))
    $output.Add('};')
    $output.Add('')
}

$output.Add('static const LevelData *const longo_playable_levels[] = {')
foreach ($roomName in $PlayableRoomNames) {
    $output.Add(('    &{0},' -f (Get-RoomSymbol $roomName)))
}
$output.Add('};')
$output.Add('#define LONGO_PLAYABLE_LEVEL_COUNT ((int)(sizeof(longo_playable_levels) / sizeof(longo_playable_levels[0])))')
$output.Add('')

$output.Add('/* Non-playable rooms retained as size/name metadata for the runtime. */')
foreach ($roomName in $MetadataRoomNames) {
    $room = $rooms[$roomName]
    $symbol = Get-RoomSymbol $roomName
    $output.Add("static const LevelData ${symbol} = {")
    $output.Add(('    {0}, {1}, {2}, (const LevelObject *)0, 0, 0' -f (ConvertTo-CStringLiteral $room.Name), $room.Width, $room.Height))
    $output.Add('};')
}
$output.Add('')
$output.Add('#endif /* LONGO_DOGGO_LEVEL_DATA_H */')

$utf8 = New-Object System.Text.UTF8Encoding($false)
$resolvedOutputPath = [IO.Path]::GetFullPath($OutputPath)
[IO.File]::WriteAllText($resolvedOutputPath, ($output -join [Environment]::NewLine) + [Environment]::NewLine, $utf8)
Write-Output "Wrote $OutputPath"
