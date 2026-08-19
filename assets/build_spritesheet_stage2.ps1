Add-Type -AssemblyName System.Drawing -ErrorAction Stop

$assetsDir = "C:\Users\ZenoxArt\Documents\!CODING\.claude\worktrees\nintendo-ds-homebrew-game-e07d4e\NDSGame\assets"
$canvas = New-Object System.Drawing.Bitmap("$assetsDir\spritesheet_stage1.png")
$gCanvasReal = [System.Drawing.Graphics]::FromImage($canvas)
$gCanvasReal.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$gCanvasReal.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

# ---- palette ----
$metBody      = [System.Drawing.Color]::FromArgb(255,120,145,185)
$metOutline   = [System.Drawing.Color]::FromArgb(255,35,40,60)
$metHighlight = [System.Drawing.Color]::FromArgb(255,190,205,225)
$metCrater    = [System.Drawing.Color]::FromArgb(255,70,85,120)
$crackGlow    = [System.Drawing.Color]::FromArgb(255,255,175,70)
$crackGlow2   = [System.Drawing.Color]::FromArgb(255,255,225,120)

$expWhite  = [System.Drawing.Color]::FromArgb(255,255,250,220)
$expYellow = [System.Drawing.Color]::FromArgb(255,255,200,80)
$expOrange = [System.Drawing.Color]::FromArgb(255,255,140,50)
$expRed    = [System.Drawing.Color]::FromArgb(255,200,60,40)
$expSmoke  = [System.Drawing.Color]::FromArgb(255,60,55,65)
$shipNavy  = [System.Drawing.Color]::FromArgb(255,25,30,55)

# Grid is a hashtable keyed by "x,y" -> Color. (A true .NET Color[,] gets
# mangled by PowerShell's pipeline/array handling across function returns,
# so a hashtable is the reliable choice here.)
function New-Grid16 {
    $grid = @{}
    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            $key = "$x,$y"
            $grid[$key] = [System.Drawing.Color]::FromArgb(0,255,255,255)
        }
    }
    return $grid
}

function Set-GridPixel($grid, [int]$x, [int]$y, [System.Drawing.Color]$color) {
    $key = "$x,$y"
    $grid[$key] = $color
}

function Blit-Grid16([System.Drawing.Graphics]$gCanvas, $grid, [int]$destX, [int]$destY) {
    $tmp = New-Object System.Drawing.Bitmap(16,16)
    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            $key = "$x,$y"
            $px = $grid[$key]
            $tmp.SetPixel($x, $y, $px)
        }
    }
    $big = New-Object System.Drawing.Bitmap(32,32)
    $g2 = [System.Drawing.Graphics]::FromImage($big)
    $g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $g2.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g2.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $g2.DrawImage($tmp, 0, 0, 32, 32)
    $g2.Dispose()
    $gCanvas.DrawImage($big, $destX, $destY)
    $tmp.Dispose()
    $big.Dispose()
}

# ------------------------------------------------------------
# Meteor rock builder (jittered blob, no tail, with outline)
# jitter: 8 values (one per 45deg octant) added to base radius
# ------------------------------------------------------------
function Build-MeteorGrid([double[]]$jitter, [int]$craterDX, [int]$craterDY, [bool]$cracked) {
    $grid = New-Grid16
    [double]$cx = 7.5
    [double]$cy = 7.5
    [double]$baseR = 6.2
    [double]$pi = [Math]::PI

    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            [double]$dx = $x - $cx
            [double]$dy = $y - $cy
            [double]$dist = [Math]::Sqrt($dx*$dx + $dy*$dy)
            [double]$ang = [Math]::Atan2($dy, $dx)
            if ($ang -lt 0) { $ang += 2*$pi }
            [int]$bucket = [int]([Math]::Floor($ang / (2*$pi/8))) % 8
            [double]$r = $baseR + $jitter[$bucket]

            if ($dist -le ($r + 1.0)) {
                Set-GridPixel $grid $x $y $metOutline
            }
            if ($dist -le $r) {
                [double]$hdx = $x - 5.0
                [double]$hdy = $y - 5.0
                [double]$cdx = $x - (7.5 + $craterDX)
                [double]$cdy = $y - (7.5 + $craterDY)
                if (($hdx*$hdx + $hdy*$hdy) -le 4.5) {
                    Set-GridPixel $grid $x $y $metHighlight
                } elseif (($cdx*$cdx + $cdy*$cdy) -le 3.2) {
                    Set-GridPixel $grid $x $y $metCrater
                } else {
                    Set-GridPixel $grid $x $y $metBody
                }
            }
        }
    }

    if ($cracked) {
        # jagged crack from upper-right edge toward center, with glowing pixels
        $crackPts = @(
            @(12,3), @(11,4), @(11,5), @(10,6), @(9,7), @(9,8), @(8,9)
        )
        foreach ($p in $crackPts) {
            Set-GridPixel $grid $p[0] $p[1] $shipNavy
        }
        $glowPts = @(@(11,4), @(9,7), @(8,9))
        foreach ($p in $glowPts) {
            Set-GridPixel $grid $p[0] $p[1] $crackGlow
        }
        Set-GridPixel $grid 10 6 $crackGlow2
    }

    return $grid
}

$jitterA = @(0.6, -0.4, 0.3, -0.6, 0.5, -0.3, 0.4, -0.5)
$jitterB = @(-0.3, 0.5, -0.5, 0.4, -0.4, 0.6, -0.6, 0.3)

$m1 = Build-MeteorGrid $jitterA -1 1 $false
$m2 = Build-MeteorGrid $jitterB 1 -1 $false
$m3 = Build-MeteorGrid $jitterA 1 1 $true

Blit-Grid16 $gCanvasReal $m1 0 32
Blit-Grid16 $gCanvasReal $m2 32 32
Blit-Grid16 $gCanvasReal $m3 64 32

# ------------------------------------------------------------
# Ship explosion frame (row3, frame3 -> x=64,y=96)
# ------------------------------------------------------------
function Build-ExplosionGrid {
    $grid = New-Grid16
    [double]$cx = 7.5
    [double]$cy = 7.5

    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            [double]$dx = $x - $cx
            [double]$dy = $y - $cy
            [double]$dist = [Math]::Sqrt($dx*$dx + $dy*$dy)
            if ($dist -le 1.8) {
                Set-GridPixel $grid $x $y $expWhite
            } elseif ($dist -le 3.2) {
                Set-GridPixel $grid $x $y $expYellow
            } elseif ($dist -le 4.6) {
                Set-GridPixel $grid $x $y $expOrange
            } elseif ($dist -le 5.8) {
                Set-GridPixel $grid $x $y $expRed
            }
        }
    }

    # radiating spikes (8 directions)
    $spikes = @(
        @(7,0),@(8,0), @(7,15),@(8,15), @(0,7),@(0,8), @(15,7),@(15,8),
        @(2,2),@(13,2), @(2,13),@(13,13)
    )
    foreach ($p in $spikes) {
        Set-GridPixel $grid $p[0] $p[1] $expOrange
    }
    $spikeTips = @(@(7,0),@(0,7),@(15,8),@(8,15))
    foreach ($p in $spikeTips) {
        Set-GridPixel $grid $p[0] $p[1] $expYellow
    }

    # a few dark smoke / debris flecks
    $debris = @(@(3,4),@(12,5),@(4,11),@(11,12))
    foreach ($p in $debris) {
        Set-GridPixel $grid $p[0] $p[1] $expSmoke
    }
    $hullBits = @(@(6,9),@(9,6))
    foreach ($p in $hullBits) {
        Set-GridPixel $grid $p[0] $p[1] $shipNavy
    }

    return $grid
}

$exp = Build-ExplosionGrid
Blit-Grid16 $gCanvasReal $exp 64 96

$gCanvasReal.Dispose()
$canvas.Save("$assetsDir\spritesheet_final.png", [System.Drawing.Imaging.ImageFormat]::Png)
$canvas.Dispose()
Write-Host "stage2 done"
