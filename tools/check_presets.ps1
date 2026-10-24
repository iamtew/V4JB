# Checks every factory preset in presets/*.xml (run by `just check`).
# Catches what would otherwise fail silently at load time: unknown parameter ids,
# out-of-range values, missing built-in waves, duplicate names, unknown categories,
# and malformed <MORPH> blocks. Ranges mirror src/parameters/ParameterLayout.h.
$ErrorActionPreference = 'Stop'

$categories = 'Init', 'Atmospheres', 'Pads', 'Moving Pads', 'Organs', 'Bells', 'Basses', 'SFX', 'Leads',
              'Swells', 'Polys', 'Percussive', 'Waves', 'Drones', 'Keys', 'Plucks', 'Textures', 'Stressful Violin', 'Thoughtful Violins'

# id regex, min, max, integer-only. First match wins.
$rules = @(
    @('^(level|spread|sp_depth|sp_moddepth|sp_modrate|nz_(mix|width|low|high|decay)|sub[12]|w_bal|w[ab]_(start|end|speed)|dr_(amt|mix)|md_(depth|mix)|dl_mix|rv_(size|damp|mix)|mx|my)$', 0, 1, $false),
    @('^pitch$', -12, 12, $false),
    @('^octave$', -3, 3, $true),
    @('^glide$', 0, 2, $false),
    @('^poly$', 1, 8, $true),
    @('^width$', 0, 2, $false),
    @('^sp_algo$', 0, 17, $true),
    @('^sp_tilt$', 0, 3, $false),
    @('^sp_partials$', 1, 512, $true),
    @('^(economy|dr_on|md_on|dl_on|dl_sync|rv_on|lock([0-9]|10)|l[1-3]_sync)$', 0, 1, $true),
    @('^md_type$', 0, 1, $true),
    @('^md_rate$', 0.01, 10, $false),
    @('^dl_time$', 10, 2000, $false),
    @('^dl_div$', 0, 6, $true),
    @('^dl_fb$', 0, 0.95, $false),
    @('^m([0-9]|10)$', -0.5, 0.5, $false),
    @('^mmode$', 0, 3, $true),
    @('^mtime$', 0.1, 30, $false),
    @('^msnap$', 0, 4, $true),
    @('^muser$', 0, 9, $true),
    @('^e[1-4]_(a|d|r)$', 0.001, 10, $false),
    @('^e[1-4]_(s|vel)$', 0, 1, $false),
    @('^l[1-3]_rate$', 0.01, 20, $false),
    @('^l[1-3]_div$', 0, 7, $true),
    @('^l[1-3]_shape$', 0, 4, $true),
    @('^s([2-9]|1[01])_type$', 0, 27, $true),
    @('^s([2-9]|1[01])_(main|p[1-4])$', 0, 1, $false),
    @('^s([2-9]|1[01])_es$', 0, 4, $true),
    @('^s([2-9]|1[01])_ls$', 0, 3, $true),
    @('^s([2-9]|1[01])_ms$', 0, 5, $true),
    @('^s([2-9]|1[01])_(ea|la|ma)$', -1, 1, $false)
)

$root = Split-Path $PSScriptRoot -Parent
$errors = [System.Collections.Generic.List[string]]::new()
$names = @{}
$files = Get-ChildItem (Join-Path $root 'presets\*.xml')

foreach ($f in $files) {
    $x = [xml](Get-Content $f.FullName -Raw)
    $p = $x.PARAMS
    if ($null -eq $p) { $errors.Add("$($f.Name): no <PARAMS> root"); continue }

    $name = $p.GetAttribute('presetName'); $cat = $p.GetAttribute('category')
    if (-not $name) { $errors.Add("$($f.Name): no presetName") }
    elseif ($names.ContainsKey($name)) { $errors.Add("$($f.Name): name '$name' also used by $($names[$name])") }
    else { $names[$name] = $f.Name }
    if ($categories -notcontains $cat) { $errors.Add("$($f.Name): unknown category '$cat'") }

    foreach ($w in 'waveA', 'waveB') {
        $src = $p.GetAttribute($w)
        if ($src -and -not ($src.StartsWith('factory:') -and (Test-Path (Join-Path $root ('presets\waves\' + $src.Substring(8)))))) {
            $errors.Add("$($f.Name): $w '$src' is not a bundled factory: wave")
        }
    }

    $seen = @{}
    foreach ($e in $p.SelectNodes('PARAM')) {
        $id = $e.id; $v = [double]$e.value
        if ($seen.ContainsKey($id)) { $errors.Add("$($f.Name): $id set twice") }
        $seen[$id] = 1
        $rule = $rules | Where-Object { $id -match $_[0] } | Select-Object -First 1
        if ($null -eq $rule) { $errors.Add("$($f.Name): unknown id '$id'"); continue }
        if ($v -lt $rule[1] -or $v -gt $rule[2] -or ($rule[3] -and $v -ne [math]::Floor($v))) {
            $errors.Add("$($f.Name): $id=$v outside $($rule[1])..$($rule[2])$(if ($rule[3]) { ' (integer)' })")
        }
    }

    $m = $p.SelectSingleNode('MORPH')
    if ($m) {
        foreach ($c in 'c0', 'c1', 'c2', 'c3') {
            $s = $m.GetAttribute($c)
            if (-not $s) { continue }
            $vals = $s -split '\s+' | Where-Object { $_ }
            if ($vals.Count -ne 11 -or ($vals | Where-Object { [double]$_ -lt 0 -or [double]$_ -gt 1 })) {
                $errors.Add("$($f.Name): MORPH $c needs 11 values in 0..1")
            }
        }
        $nodes = $m.GetAttribute('nodes')
        if ($nodes) {
            $list = $nodes -split ';' | Where-Object { $_ }
            if ($list.Count -gt 16) { $errors.Add("$($f.Name): more than 16 morph nodes") }
            foreach ($n in $list) {
                $t = $n -split '\s+' | Where-Object { $_ }
                if ($t.Count -ne 3 -or [double]$t[0] -lt 0 -or [double]$t[0] -gt 1 -or [double]$t[1] -lt 0 -or [double]$t[1] -gt 1 -or
                    [int]$t[2] -lt 0 -or [int]$t[2] -gt 5) { $errors.Add("$($f.Name): bad morph node '$n'") }
            }
        }
    }
}

if ($errors.Count) { $errors | ForEach-Object { Write-Host $_ }; Write-Host "check_presets FAILED ($($errors.Count))"; exit 1 }
Write-Host "check_presets OK ($($files.Count) presets, $(($names.Values | Measure-Object).Count) names)"
