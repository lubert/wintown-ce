$bytes = [System.IO.File]::ReadAllBytes('wintown.res')
$rawStr = [System.Text.Encoding]::Unicode.GetString($bytes)
$idx = $rawStr.IndexOf("New Game - WiNTown")
if ($idx -lt 0) {
    Write-Error "Could not find dialog title in wintown.res"
    exit 1
}
$titlePos = $idx * 2
# The dialog header starts 22 bytes before the title (style:4, exStyle:4, cdit:2, x:2, y:2, cx:2, cy:2, menu:2, class:2)
$p = $titlePos - 22
$style = [BitConverter]::ToUInt32($bytes, $p)
$exStyle = [BitConverter]::ToUInt32($bytes, $p + 4)
$cdit = [BitConverter]::ToUInt16($bytes, $p + 8)
$p += 18 # past x, y, cx, cy

# menu
if ([BitConverter]::ToUInt16($bytes, $p) -eq 0xFFFF) { $p += 4 }
elseif ([BitConverter]::ToUInt16($bytes, $p) -eq 0) { $p += 2 }
else { while ([BitConverter]::ToUInt16($bytes, $p) -ne 0) { $p += 2 }; $p += 2 }

# class
if ([BitConverter]::ToUInt16($bytes, $p) -eq 0xFFFF) { $p += 4 }
elseif ([BitConverter]::ToUInt16($bytes, $p) -eq 0) { $p += 2 }
else { while ([BitConverter]::ToUInt16($bytes, $p) -ne 0) { $p += 2 }; $p += 2 }

# title
$title = ""
while ([BitConverter]::ToUInt16($bytes, $p) -ne 0) {
    $title += [char][BitConverter]::ToUInt16($bytes, $p)
    $p += 2
}
$p += 2

# font
$fontSize = [BitConverter]::ToUInt16($bytes, $p)
$p += 2
$fontName = ""
while ([BitConverter]::ToUInt16($bytes, $p) -ne 0) {
    $fontName += [char][BitConverter]::ToUInt16($bytes, $p)
    $p += 2
}
$p += 2

Write-Output "Dialog title='$title', font='$fontName' ($fontSize pt), cdit=$cdit"

# Controls
for ($i = 0; $i -lt $cdit; $i++) {
    $p = ($p + 3) -band -bnot 3 # DWORD align
    $cStyle = [BitConverter]::ToUInt32($bytes, $p)
    $cExStyle = [BitConverter]::ToUInt32($bytes, $p + 4)
    $cx = [BitConverter]::ToInt16($bytes, $p + 8)
    $cy = [BitConverter]::ToInt16($bytes, $p + 10)
    $ccx = [BitConverter]::ToInt16($bytes, $p + 12)
    $ccy = [BitConverter]::ToInt16($bytes, $p + 14)
    $id = [BitConverter]::ToUInt16($bytes, $p + 16)
    $p += 18
    
    # class
    $cls = ""
    if ([BitConverter]::ToUInt16($bytes, $p) -eq 0xFFFF) {
        $atom = [BitConverter]::ToUInt16($bytes, $p + 2)
        $cls = switch ($atom) { 0x80 {"Button"} 0x81 {"Edit"} 0x82 {"Static"} 0x83 {"ListBox"} 0x84 {"ScrollBar"} 0x85 {"ComboBox"} default {"Atom_0x" + $atom.ToString("X")} }
        $p += 4
    } else {
        while ([BitConverter]::ToUInt16($bytes, $p) -ne 0) {
            $cls += [char][BitConverter]::ToUInt16($bytes, $p)
            $p += 2
        }
        $p += 2
    }
    
    # text
    $txt = ""
    if ([BitConverter]::ToUInt16($bytes, $p) -eq 0xFFFF) {
        $txt = "Ordinal_" + [BitConverter]::ToUInt16($bytes, $p + 2)
        $p += 4
    } else {
        while ([BitConverter]::ToUInt16($bytes, $p) -ne 0) {
            $txt += [char][BitConverter]::ToUInt16($bytes, $p)
            $p += 2
        }
        $p += 2
    }
    # extra data
    $extra = [BitConverter]::ToUInt16($bytes, $p)
    $p += 2 + $extra
    
    Write-Output "[$i] ID: $id, Class: $cls, Text: '$txt', Rect: ($cx, $cy, $ccx, $ccy), Style: 0x$($cStyle.ToString('X8'))"
}
