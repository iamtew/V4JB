# V4JB – Justfile (Projucer + MSBuild)

set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

JUCE_PATH := env_var_or_default("JUCE_PATH", env_var("USERPROFILE") + "\\JUCE")
PROJUCER  := JUCE_PATH + "\\Projucer.exe"
PROJECT   := "V4JB.jucer"
SOLUTION  := "Builds\\VisualStudio2022\\V4JB.sln"
# Bare `msbuild` is not on PATH with Build Tools alone.
# CI sets MSBUILD to the runner's Visual Studio; this default stays the local Build Tools.
MSBUILD   := env_var_or_default("MSBUILD", "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe")

default:
    @just --list

projucer:
    {{PROJUCER}} {{PROJECT}}

resave:
    $p = Start-Process -FilePath '{{PROJUCER}}' -ArgumentList '--resave','{{PROJECT}}' -Wait -PassThru; if ($null -eq $p.ExitCode) { throw 'Projucer --resave returned no exit code' }; if ($p.ExitCode -ne 0) { exit $p.ExitCode }
    # OneDrive can lag the sln behind "Finished saving".
    $n=0; while (-not (Test-Path '{{SOLUTION}}') -and $n -lt 20) { Start-Sleep -Milliseconds 250; $n++ }
    just stamp-version

# Projucer can exit without rewriting the version. tools/stamp_version.ps1
# writes it into resources.rc and the vcxproj macros, and throws if it missed.
stamp-version:
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\stamp_version.ps1

# PNG → Builds/.../icon.ico, then wipe compiled .res so the next build relinks it.
# `just resave` does not refresh icon.ico when the PNG changes.
icon:
    magick assets/logo.png -define icon:auto-resize=256,128,64,48,32,16 Builds\VisualStudio2022\icon.ico
    Get-ChildItem Builds\VisualStudio2022 -Recurse -Filter resources.res | Remove-Item -Force

# `just icon` after resave: Projucer rewrites icon.ico; magick must win, then drop .res.
build-debug: resave
    just icon
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Debug /p:Platform=x64 /m; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    ie4uinit.exe -show

build-release: resave
    just icon
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Release /p:Platform=x64 /m; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    ie4uinit.exe -show

build-dist: build-release
    if (Test-Path dist) { Remove-Item dist -Recurse -Force }
    New-Item dist -ItemType Directory | Out-Null
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; Compress-Archive -Path 'Builds\VisualStudio2022\x64\Release\VST3\V4JB.vst3' -DestinationPath "dist\V4JB-$ver-win-x64-vst3.zip" -Force; Compress-Archive -Path 'Builds\VisualStudio2022\x64\Release\Standalone Plugin\V4JB.exe' -DestinationPath "dist\V4JB-$ver-win-x64-standalone.zip" -Force
    just build-msi

# Release binaries must already exist. First build downloads WiX.
# `dotnet` on PATH when it has an SDK, otherwise %USERPROFILE%\.dotnet.
build-msi:
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; $root = (Get-Location).Path; $dotnet = 'dotnet'; if (-not ((& dotnet --list-sdks) 2>$null)) { $dotnet = Join-Path $env:USERPROFILE '.dotnet\dotnet.exe'; if (-not (Test-Path $dotnet)) { throw 'build-msi needs the .NET 8 SDK on PATH, or dotnet.exe in %USERPROFILE%\.dotnet' } }; New-Item -ItemType Directory -Force -Path dist | Out-Null; & $dotnet build installer\V4JB.wixproj -c Release -p:ProductVersion=$ver -p:Vst3Dir="$root\Builds\VisualStudio2022\x64\Release\VST3\V4JB.vst3" -p:StandaloneDir="$root\Builds\VisualStudio2022\x64\Release\Standalone Plugin" -p:IconDir="$root\Builds\VisualStudio2022"; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; $msi = Get-ChildItem installer\bin -Recurse -Filter V4JB.msi | Select-Object -First 1; Copy-Item $msi.FullName "dist\V4JB-$ver-win-x64.msi" -Force

clean:
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Debug /p:Platform=x64
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Release /p:Platform=x64

rebuild: clean build-debug

vs:
    start {{SOLUTION}}

# Regenerate the built-in Wave sources in presets/waves/ (self-synthesised, committed).
waves:
    New-Item -ItemType Directory -Force -Path presets\waves | Out-Null
    cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" >nul && cl /nologo /EHsc /O2 /W4 /std:c++17 /D_CRT_SECURE_NO_WARNINGS tools\make_waves.cpp /Fo:tools\make_waves.obj /Fe:tools\make_waves.exe && tools\make_waves.exe"

# Header-only checks, no JUCE needed. Exit code != 0 on the first failed assert.
check:
    cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" >nul && cl /nologo /EHsc /std:c++17 /I src tests\version_check.cpp /Fo:tests\version_check.obj /Fe:tests\version_check.exe && tests\version_check.exe && cl /nologo /EHsc /O2 /W4 /std:c++17 /I src tests\engine_check.cpp /Fo:tests\engine_check.obj /Fe:tests\engine_check.exe && tests\engine_check.exe"
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_presets.ps1
