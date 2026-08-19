Add-Type -AssemblyName System.Drawing -ErrorAction Stop

$root = "C:\Users\ZenoxArt\Documents\!CODING\.claude\worktrees\nintendo-ds-homebrew-game-e07d4e\NDSGame"
$framesDir = "$root\assets\Starfighter\quant"

# NOTE: ffmpeg's paletteuse writes output frames starting at index 1 (not 0)
# unless "-start_number 0" is passed, so quant/ contains frame_0001..NNNN
# rather than frame_0000..(NNNN-1). Harmless here: this script just sorts by
# name and appends in that order, so the only effect is which rendered pose
# ends up as animation index 0 - cosmetically invisible on a looping spin.
$paletteImg = New-Object System.Drawing.Bitmap("$root\assets\Starfighter\palette.png")

# ffmpeg's palettegen always emits a 16x16 (256-slot) image even when
# max_colors is lower; collapse it down to the actual distinct colors in
# first-seen order so we get a compact 0-127 palette that fits inside the
# free 100-227 slot range of the HUD's shared BG_PALETTE_SUB (see hud.c -
# indices 1-40 are the gradient, 50-52 header/border/footer, 15 is the
# console's text color).
$paletteColors = New-Object System.Collections.Generic.List[System.Drawing.Color]
$colorToIndex = @{}
for ($y = 0; $y -lt $paletteImg.Height; $y++) {
    for ($x = 0; $x -lt $paletteImg.Width; $x++) {
        $c = $paletteImg.GetPixel($x, $y)
        $key = "$($c.R),$($c.G),$($c.B)"
        if (-not $colorToIndex.ContainsKey($key)) {
            $colorToIndex[$key] = $paletteColors.Count
            $paletteColors.Add($c)
        }
    }
}
$paletteImg.Dispose()
Write-Host "Collapsed palette to $($paletteColors.Count) distinct colors."
if ($paletteColors.Count -gt 128) {
    throw "Palette has more than 128 colors ($($paletteColors.Count)) - won't fit in the reserved sub-palette range."
}

# Write the palette as RGB555 (matches BG_PALETTE_SUB's native format).
$palBytes = New-Object byte[] ($paletteColors.Count * 2)
for ($i = 0; $i -lt $paletteColors.Count; $i++) {
    $c = $paletteColors[$i]
    $r5 = [int]($c.R * 31 / 255)
    $g5 = [int]($c.G * 31 / 255)
    $b5 = [int]($c.B * 31 / 255)
    $val = $r5 -bor ($g5 -shl 5) -bor ($b5 -shl 10)
    $palBytes[$i*2]   = [byte]($val -band 0xFF)
    $palBytes[$i*2+1] = [byte](($val -shr 8) -band 0xFF)
}
[System.IO.File]::WriteAllBytes("$root\nitrofs\starfighter.pal", $palBytes)
Write-Host "Wrote starfighter.pal ($($palBytes.Length) bytes)."

$frameFiles = Get-ChildItem "$framesDir\frame_*.png" | Sort-Object Name
Write-Host "Converting $($frameFiles.Count) frames..."

$outStream = [System.IO.File]::Create("$root\nitrofs\starfighter.raw")
$missCount = 0
$frameW = 0
$frameH = 0

foreach ($file in $frameFiles) {
    $img = New-Object System.Drawing.Bitmap($file.FullName)
    $frameW = $img.Width
    $frameH = $img.Height
    $rowBytes = New-Object byte[] ($frameW * $frameH)
    for ($y = 0; $y -lt $frameH; $y++) {
        for ($x = 0; $x -lt $frameW; $x++) {
            $c = $img.GetPixel($x, $y)
            $key = "$($c.R),$($c.G),$($c.B)"
            if ($colorToIndex.ContainsKey($key)) {
                $idx = $colorToIndex[$key]
            } else {
                $missCount++
                $idx = 0
            }
            $rowBytes[$y*$frameW + $x] = [byte]$idx
        }
    }
    $outStream.Write($rowBytes, 0, $rowBytes.Length)
    $img.Dispose()
}
$outStream.Close()

Write-Host "Done. Color-lookup misses: $missCount"
$rawInfo = Get-Item "$root\nitrofs\starfighter.raw"
Write-Host "starfighter.raw size: $($rawInfo.Length) bytes ($($rawInfo.Length / ($frameW * $frameH)) frames of ${frameW}x${frameH})"
