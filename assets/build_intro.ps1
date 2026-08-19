Add-Type -AssemblyName System.Drawing -ErrorAction Stop

$root = "C:\Users\ZenoxArt\Documents\!CODING\.claude\worktrees\nintendo-ds-homebrew-game-e07d4e\NDSGame"
$framesDir = "$root\assets\intro_frames"
$paletteImg = New-Object System.Drawing.Bitmap("$root\assets\intro_work\palette.png")

# Build the 256-entry palette list (row-major, index = y*16+x) and an
# RGB -> index lookup for exact matching (paletteuse already snapped every
# frame's pixels onto this exact palette, so exact-match lookup is enough).
$paletteColors = New-Object 'System.Drawing.Color[]' 256
$colorToIndex = @{}
for ($y = 0; $y -lt 16; $y++) {
    for ($x = 0; $x -lt 16; $x++) {
        $idx = $y * 16 + $x
        $c = $paletteImg.GetPixel($x, $y)
        $paletteColors[$idx] = $c
        $key = "$($c.R),$($c.G),$($c.B)"
        if (-not $colorToIndex.ContainsKey($key)) {
            $colorToIndex[$key] = $idx
        }
    }
}
$paletteImg.Dispose()
Write-Host "Loaded 256-color palette."

# Write the palette as RGB555 (matches BG_PALETTE's native format).
$palBytes = New-Object byte[] (256 * 2)
for ($i = 0; $i -lt 256; $i++) {
    $c = $paletteColors[$i]
    $r5 = [int]($c.R * 31 / 255)
    $g5 = [int]($c.G * 31 / 255)
    $b5 = [int]($c.B * 31 / 255)
    $val = $r5 -bor ($g5 -shl 5) -bor ($b5 -shl 10)
    $palBytes[$i*2]   = [byte]($val -band 0xFF)
    $palBytes[$i*2+1] = [byte](($val -shr 8) -band 0xFF)
}
[System.IO.File]::WriteAllBytes("$root\nitrofs\intro.pal", $palBytes)
Write-Host "Wrote intro.pal (512 bytes)."

$frameFiles = Get-ChildItem "$framesDir\frame_*.png" | Sort-Object Name
Write-Host "Converting $($frameFiles.Count) frames..."

$outStream = [System.IO.File]::Create("$root\nitrofs\intro.raw")
$missCount = 0

foreach ($file in $frameFiles) {
    $img = New-Object System.Drawing.Bitmap($file.FullName)
    $rowBytes = New-Object byte[] (256 * 192)
    for ($y = 0; $y -lt 192; $y++) {
        for ($x = 0; $x -lt 256; $x++) {
            $c = $img.GetPixel($x, $y)
            $key = "$($c.R),$($c.G),$($c.B)"
            if ($colorToIndex.ContainsKey($key)) {
                $idx = $colorToIndex[$key]
            } else {
                $missCount++
                $idx = 0
            }
            $rowBytes[$y*256 + $x] = [byte]$idx
        }
    }
    $outStream.Write($rowBytes, 0, $rowBytes.Length)
    $img.Dispose()
}
$outStream.Close()

Write-Host "Done. Color-lookup misses: $missCount"
$rawInfo = Get-Item "$root\nitrofs\intro.raw"
Write-Host "intro.raw size: $($rawInfo.Length) bytes ($($rawInfo.Length / (256*192)) frames)"
