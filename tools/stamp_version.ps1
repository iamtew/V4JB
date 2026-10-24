# Stamp V4JB.jucer version into the Projucer export (run by `just resave`).
# Projucer can finish without rewriting these files. The title, About dialog,
# and update check all read the compiled JucePlugin_VersionString. Windows
# file version comes from resources.rc. A missing stamp aborts the build.
$ErrorActionPreference = 'Stop'

$ver = ([xml](Get-Content 'V4JB.jucer' -Raw)).JUCERPROJECT.version
if (-not $ver -or $ver -notmatch '^\d+(\.\d+){2,3}$') {
    throw "jucer version '$ver' is not major.minor.patch"
}

# JUCE: (major<<16)+(minor<<8)+patch, then another <<8 when a fourth segment exists.
$seg = @($ver.Split('.') | ForEach-Object { [int]$_ })
$hex = ($seg[0] -shl 16) + ($seg[1] -shl 8) + $seg[2]
if ($seg.Count -gt 3) { $hex = ($hex -shl 8) + $seg[3] }
$hexStr = '0x{0:x}' -f $hex
# FILEVERSION wants four comma-separated integers. 0.0.3 becomes 0,0,3,0.
$comma = ((@($seg) + 0)[0..3] -join ',')

$rcPath = 'Builds\VisualStudio2022\resources.rc'
if (-not (Test-Path $rcPath)) { throw "missing $rcPath" }
$rc = [IO.File]::ReadAllText($rcPath)
$rc2 = $rc -replace 'FILEVERSION\s+[0-9,]+', "FILEVERSION  $comma"
$rc2 = $rc2 -replace 'PRODUCTVERSION\s+[0-9,]+', "PRODUCTVERSION  $comma"
$rc2 = $rc2 -replace 'VALUE "FileVersion",\s+"[0-9.]+\\0"', ('VALUE "FileVersion",  "' + $ver + '\0"')
$rc2 = $rc2 -replace 'VALUE "ProductVersion",\s+"[0-9.]+\\0"', ('VALUE "ProductVersion",  "' + $ver + '\0"')
if ($rc2 -notmatch ('FILEVERSION\s+' + [regex]::Escape($comma))) { throw "resources.rc FILEVERSION is not $comma" }
if ($rc2 -notmatch ('VALUE "FileVersion",\s+"' + [regex]::Escape($ver) + '\\0"')) { throw "resources.rc FileVersion is not $ver" }

# No BOM. rc.exe treats a BOM as junk at the top of the file.
$utf8 = New-Object System.Text.UTF8Encoding $false
[IO.File]::WriteAllText($rcPath, $rc2, $utf8)

$projects = @(Get-ChildItem 'Builds\VisualStudio2022\*.vcxproj')
if (-not $projects) { throw 'no vcxproj to stamp' }
foreach ($f in $projects) {
    $t = [IO.File]::ReadAllText($f.FullName)
    if ($t -notmatch 'JucePlugin_Version=') { throw "$($f.Name) has no JucePlugin_Version" }
    # Longer names first, or Version= eats VersionString= and VersionCode=.
    $n = $t -replace 'JucePlugin_VersionString=&quot;[0-9.]+&quot;', "JucePlugin_VersionString=&quot;$ver&quot;"
    $n = $n -replace 'JucePlugin_VersionString=\\&quot;[0-9.]+\\&quot;', "JucePlugin_VersionString=\&quot;$ver\&quot;"
    $n = $n -replace 'JucePlugin_VersionCode=0x[0-9a-fA-F]+', "JucePlugin_VersionCode=$hexStr"
    $n = $n -replace 'JucePlugin_Version=[0-9.]+', "JucePlugin_Version=$ver"

    $strings = [regex]::Matches($n, 'JucePlugin_VersionString=(?:\\&quot;|&quot;)([0-9.]+)')
    $codes = [regex]::Matches($n, 'JucePlugin_VersionCode=(0x[0-9a-fA-F]+)')
    $vers = [regex]::Matches($n, 'JucePlugin_Version=([0-9.]+)')
    if ($strings.Count -eq 0 -or $codes.Count -eq 0 -or $vers.Count -eq 0) {
        throw "$($f.Name) is missing a version macro"
    }
    foreach ($m in $strings) { if ($m.Groups[1].Value -ne $ver) { throw "$($f.Name) VersionString is $($m.Groups[1].Value)" } }
    foreach ($m in $codes) { if ($m.Groups[1].Value -ne $hexStr) { throw "$($f.Name) VersionCode is $($m.Groups[1].Value)" } }
    foreach ($m in $vers) { if ($m.Groups[1].Value -ne $ver) { throw "$($f.Name) Version is $($m.Groups[1].Value)" } }
    if ($n -ne $t) { [IO.File]::WriteAllText($f.FullName, $n, $utf8) }
}

# Drop the compiled resource so the next link picks up the stamped .rc.
Get-ChildItem Builds\VisualStudio2022 -Recurse -Filter resources.res -ErrorAction SilentlyContinue | Remove-Item -Force
