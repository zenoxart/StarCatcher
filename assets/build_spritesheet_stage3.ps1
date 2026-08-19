Add-Type -AssemblyName System.Drawing -ErrorAction Stop

$assetsDir = "C:\Users\ZenoxArt\Documents\!CODING\.claude\worktrees\nintendo-ds-homebrew-game-e07d4e\NDSGame\assets"
$canvas = New-Object System.Drawing.Bitmap("$assetsDir\spritesheet_final.png")
$gCanvasReal = [System.Drawing.Graphics]::FromImage($canvas)
$gCanvasReal.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$gCanvasReal.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

$expWhite  = [System.Drawing.Color]::FromArgb(255,255,250,220)
$expYellow = [System.Drawing.Color]::FromArgb(255,255,200,80)
$expOrange = [System.Drawing.Color]::FromArgb(255,255,140,50)
$expRed    = [System.Drawing.Color]::FromArgb(255,190,55,40)
$expDarkRed = [System.Drawing.Color]::FromArgb(255,120,30,30)
$expSmoke  = [System.Drawing.Color]::FromArgb(255,70,65,75)
$expSmoke2 = [System.Drawing.Color]::FromArgb(255,110,105,115)
$shipNavy  = [System.Drawing.Color]::FromArgb(255,25,30,55)
$shipHull  = [System.Drawing.Color]::FromArgb(255,150,170,200)
# erase (paint over the old sun-like frame with transparency first)
$clear = [System.Drawing.Color]::FromArgb(0,255,255,255)

function New-Grid16 {
    $grid = @{}
    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            $grid["$x,$y"] = [System.Drawing.Color]::FromArgb(0,255,255,255)
        }
    }
    return $grid
}

function Set-GridPixel($grid, [int]$x, [int]$y, [System.Drawing.Color]$color) {
    if ($x -ge 0 -and $x -lt 16 -and $y -ge 0 -and $y -lt 16) {
        $grid["$x,$y"] = $color
    }
}

function Blit-Grid16([System.Drawing.Graphics]$gCanvas, $grid, [int]$destX, [int]$destY) {
    $tmp = New-Object System.Drawing.Bitmap(16,16)
    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            $px = $grid["$x,$y"]
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

function Build-ExplosionGrid {
    $grid = New-Grid16
    [double]$cx = 7.5
    [double]$cy = 7.5
    [double]$pi = [Math]::PI

    # jagged burst silhouette: 12 angle buckets with dramatic, uneven jitter
    $jitter = @(5.5, 2.5, 4.5, 1.8, 6.0, 2.2, 4.0, 1.5, 5.0, 2.8, 3.5, 1.2)
    [int]$buckets = $jitter.Length

    for ($y = 0; $y -lt 16; $y++) {
        for ($x = 0; $x -lt 16; $x++) {
            [double]$dx = $x - $cx
            [double]$dy = $y - $cy
            [double]$dist = [Math]::Sqrt($dx*$dx + $dy*$dy)
            [double]$ang = [Math]::Atan2($dy, $dx)
            if ($ang -lt 0) { $ang += 2*$pi }
            [int]$bucket = [int]([Math]::Floor($ang / (2*$pi/$buckets))) % $buckets
            [double]$r = 1.5 + $jitter[$bucket]

            if ($dist -le $r) {
                [double]$t = $dist / $r
                if ($t -le 0.28) {
                    Set-GridPixel $grid $x $y $expWhite
                } elseif ($t -le 0.5) {
                    Set-GridPixel $grid $x $y $expYellow
                } elseif ($t -le 0.75) {
                    Set-GridPixel $grid $x $y $expOrange
                } elseif ($t -le 0.92) {
                    Set-GridPixel $grid $x $y $expRed
                } else {
                    Set-GridPixel $grid $x $y $expDarkRed
                }
            }
        }
    }

    # angular dark hull debris chunks flying outward (asymmetric, not centered rings)
    Set-GridPixel $grid 2 2 $shipNavy
    Set-GridPixel $grid 3 2 $shipNavy
    Set-GridPixel $grid 2 3 $shipHull

    Set-GridPixel $grid 13 3 $shipNavy
    Set-GridPixel $grid 14 4 $shipNavy
    Set-GridPixel $grid 13 4 $shipHull

    Set-GridPixel $grid 1 12 $shipNavy
    Set-GridPixel $grid 2 13 $shipNavy
    Set-GridPixel $grid 1 13 $shipHull

    Set-GridPixel $grid 12 13 $shipNavy
    Set-GridPixel $grid 13 12 $shipHull

    # detached smoke puffs, offset from the main burst
    Set-GridPixel $grid 0 6 $expSmoke
    Set-GridPixel $grid 0 7 $expSmoke2
    Set-GridPixel $grid 15 9 $expSmoke
    Set-GridPixel $grid 15 10 $expSmoke2
    Set-GridPixel $grid 6 15 $expSmoke
    Set-GridPixel $grid 7 15 $expSmoke2

    return $grid
}

# clear the old (sun-like) frame3 first (opaque white, so it truly overwrites
# rather than alpha-blending transparently over the old attempt), then draw the new one
$blank = @{}
for ($yy = 0; $yy -lt 16; $yy++) {
    for ($xx = 0; $xx -lt 16; $xx++) {
        $blank["$xx,$yy"] = [System.Drawing.Color]::White
    }
}
Blit-Grid16 $gCanvasReal $blank 64 96

$exp = Build-ExplosionGrid
Blit-Grid16 $gCanvasReal $exp 64 96

$gCanvasReal.Dispose()
$canvas.Save("$assetsDir\spritesheet_final2.png", [System.Drawing.Imaging.ImageFormat]::Png)
$canvas.Dispose()
Write-Host "stage3 done"
