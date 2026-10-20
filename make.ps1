# Rocket build driver for Windows, the counterpart of make.sh.
#
#   .\make.ps1              debug build
#   .\make.ps1 --test       debug build, then run the test suite
#   .\make.ps1 --asan       debug build with AddressSanitizer
#   .\make.ps1 --release    optimised build, tests off
#
# Flags combine: .\make.ps1 --asan --test
#
# It needs Visual Studio 2022 (or its Build Tools) with the C++ workload,
# whose Windows SDK also brings dxc, the shader compiler. It sets up the MSVC
# environment itself, so it runs from any PowerShell.

$ErrorActionPreference = 'Stop'

$Dir = $PSScriptRoot
$Build = 'Debug'
$Out = Join-Path $Dir '.build\Debug'
$Testing = 'ON'
$Tests = $false
$Asan = ''

if ($args -contains '--release') {
    $Build = 'Release'
    $Out = Join-Path $Dir '.build\Release'
    $Testing = 'OFF'
}

if ($args -contains '--asan') {
    $Asan = '/fsanitize=address'
}

if ($args -contains '--test') {
    $Testing = 'ON'
    $Tests = $true
}

# The MSVC compiler, linker and Windows SDK are only on PATH inside a developer
# shell; vcvars64.bat sets it up, and its environment is copied into this one.
if ($null -eq (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    # vcvars64.bat runs vswhere itself, so its folder goes on PATH.
    $Installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
    $env:PATH = "$Installer;$env:PATH"
    $Vs = & (Join-Path $Installer 'vswhere.exe') -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

    if ([string]::IsNullOrEmpty($Vs)) {
        throw 'Visual Studio with the C++ workload is not installed'
    }

    $VcVars = Join-Path $Vs 'VC\Auxiliary\Build\vcvars64.bat'

    cmd /c "`"$VcVars`" >nul && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2])
        }
    }
}

# The flags replace CMake's MSVC defaults, so the defaults are spelled out
# (/EHsc and /GR among them) and the sanitizer is added to them.
$Configure = @(
    '-S', $Dir, '-B', $Out, '-G', 'Ninja',
    "-DCMAKE_BUILD_TYPE=$Build",
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    "-DCMAKE_C_FLAGS=/DWIN32 /D_WINDOWS $Asan",
    "-DCMAKE_CXX_FLAGS=/DWIN32 /D_WINDOWS /EHsc /GR $Asan",
    "-DBUILD_TESTING=$Testing"
)

cmake @Configure

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build $Out --parallel 4

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($Tests) {
    # Two at a time: every test process creates a Direct3D 12 device.
    ctest --test-dir $Out --output-on-failure --parallel 2
    exit $LASTEXITCODE
}
