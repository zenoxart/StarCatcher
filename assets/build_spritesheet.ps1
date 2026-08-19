Add-Type -AssemblyName System.Drawing -ErrorAction Stop

$assetsDir = "C:\Users\ZenoxArt\Documents\!CODING\.claude\worktrees\nintendo-ds-homebrew-game-e07d4e\NDSGame\assets"
$srcPath = "$assetsDir\spritesheet.png"
$src = New-Object System.Drawing.Bitmap($srcPath)

$W = 96
$H = 128
$canvas = New-Object System.Drawing.Bitmap($W, $H)
$gCanvas = [System.Drawing.Graphics]::FromImage($canvas)
$gCanvas.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$gCanvas.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$gCanvas.Clear([System.Drawing.Color]::White)

function Copy-Cell([System.Drawing.Graphics]$g, [System.Drawing.Bitmap]$source, [int]$sx, [int]$sy, [int]$dx, [int]$dy) {
    $srcRect = New-Object System.Drawing.Rectangle($sx, $sy, 32, 32)
    $g.DrawImage($source, $dx, $dy, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
}

function Extract-Cell([System.Drawing.Bitmap]$source, [int]$sx, [int]$sy) {
    $cell = New-Object System.Drawing.Bitmap(32, 32)
    $g = [System.Drawing.Graphics]::FromImage($cell)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.DrawImage($source, (New-Object System.Drawing.Rectangle(0,0,32,32)), (New-Object System.Drawing.Rectangle($sx,$sy,32,32)), [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    return $cell
}

function Rotate-Cell([System.Drawing.Bitmap]$cell, [double]$angle, [double]$scale) {
    $out = New-Object System.Drawing.Bitmap(32, 32)
    $g = [System.Drawing.Graphics]::FromImage($out)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $g.TranslateTransform(16, 16)
    $g.RotateTransform($angle)
    $g.ScaleTransform($scale, $scale)
    $g.DrawImage($cell, -16, -16, 32, 32)
    $g.Dispose()
    return $out
}

# ============================================================
# ROW 0: STAR  (y = 0)
# frame1: keep as-is. frame2: frame1 rotated 45deg. frame3: keep (already reads as "collected").
# ============================================================
Copy-Cell $gCanvas $src 0 0 0 0

$star1 = Extract-Cell $src 0 0
$star2 = Rotate-Cell $star1 45 0.72
$gCanvas.DrawImage($star2, 32, 0)
$star1.Dispose(); $star2.Dispose()

Copy-Cell $gCanvas $src 64 0 64 0

# ============================================================
# ROW 2 (index 2): PORTAL (y = 64)
# Reuse the existing (already purple/teal) art, boost saturation/contrast,
# and rotate progressively across the 3 frames for a real spin animation.
# ============================================================
$portalBase = Extract-Cell $src 0 64

$cm = New-Object System.Drawing.Imaging.ColorMatrix
# boost saturation + slight brightness/contrast lift
$sat = 1.35
$lumR = 0.3086; $lumG = 0.6094; $lumB = 0.0820
$sr = (1 - $sat) * $lumR; $sg = (1 - $sat) * $lumG; $sb = (1 - $sat) * $lumB
$cm.Matrix00 = $sr + $sat; $cm.Matrix01 = $sr;         $cm.Matrix02 = $sr
$cm.Matrix10 = $sg;        $cm.Matrix11 = $sg + $sat;  $cm.Matrix12 = $sg
$cm.Matrix20 = $sb;        $cm.Matrix21 = $sb;         $cm.Matrix22 = $sb + $sat
$cm.Matrix33 = 1
$cm.Matrix44 = 1
$attr = New-Object System.Drawing.Imaging.ImageAttributes
$attr.SetColorMatrix($cm)

function Enhance-Portal([System.Drawing.Bitmap]$cell, [System.Drawing.Imaging.ImageAttributes]$attr) {
    $out = New-Object System.Drawing.Bitmap(32, 32)
    $g = [System.Drawing.Graphics]::FromImage($out)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $destRect = New-Object System.Drawing.Rectangle(0,0,32,32)
    $g.DrawImage($cell, $destRect, 0, 0, 32, 32, [System.Drawing.GraphicsUnit]::Pixel, $attr)
    $g.Dispose()
    return $out
}

$portalEnhanced = Enhance-Portal $portalBase $attr
$portalF1 = Rotate-Cell $portalEnhanced 0 1.0
$portalF2 = Rotate-Cell $portalEnhanced 35 1.0
$portalF3 = Rotate-Cell $portalEnhanced 70 1.0
$gCanvas.DrawImage($portalF1, 0, 64)
$gCanvas.DrawImage($portalF2, 32, 64)
$gCanvas.DrawImage($portalF3, 64, 64)
$portalBase.Dispose(); $portalEnhanced.Dispose(); $portalF1.Dispose(); $portalF2.Dispose(); $portalF3.Dispose()

# ============================================================
# ROW 3 (index 3): SHIP facing north (y = 96)
# frame1 (idle) + frame2 (lights blinking): keep existing art.
# frame3: NEW explosion frame, built below.
# ============================================================
Copy-Cell $gCanvas $src 0 96 0 96
Copy-Cell $gCanvas $src 32 96 32 96

$attr.Dispose()
$gCanvas.Dispose()
$canvas.Save("$assetsDir\spritesheet_stage1.png", [System.Drawing.Imaging.ImageFormat]::Png)
$canvas.Dispose()
$src.Dispose()
Write-Host "stage1 done"
