param(
    [switch]$WithoutOled,
    [string]$BuildDirectory,
    [string[]]$CmakeArgs = @()
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    if (-not $BuildDirectory) { $BuildDirectory = if ($WithoutOled) { 'build-no-oled' } else { 'build' } }
    $localTools = Join-Path $projectRoot '.build-tools'
    $cmake = Join-Path $localTools 'python/cmake/data/bin/cmake.exe'
    if (-not (Test-Path -LiteralPath $cmake)) { $cmake = (Get-Command cmake -ErrorAction Stop).Source }
    $arguments = @('-S', '.', '-B', $BuildDirectory, '-G', 'Ninja', '-DPICO_BOARD=pico2_w', '-DCMAKE_BUILD_TYPE=Release')
    if (Test-Path -LiteralPath "$localTools/pico-sdk") { $env:PICO_SDK_PATH = "$localTools/pico-sdk" }
    if (Test-Path -LiteralPath "$localTools/bin/arm-none-eabi-gcc.exe") {
        $env:PICO_TOOLCHAIN_PATH = $localTools
        $env:PATH = "$localTools/bin;" + $env:PATH
    }
    if (Test-Path -LiteralPath "$localTools/libxsm3") { $env:LIBXSM3_ROOT = "$localTools/libxsm3" }
    if (Test-Path -LiteralPath "$localTools/python/bin/ninja.exe") {
        $arguments += "-DCMAKE_MAKE_PROGRAM=$localTools/python/bin/ninja.exe"
    }
    if (Test-Path -LiteralPath "$localTools/picotool-install/picotool/picotoolConfig.cmake") {
        $arguments += @("-Dpicotool_DIR=$localTools/picotool-install/picotool", '-DPICO_NO_PICOTOOL=0')
    }
    $env:PATH = (Split-Path $cmake -Parent) + ';' + $env:PATH
    if (Test-Path -LiteralPath 'C:/msys64/mingw64/bin/g++.exe') {
        $env:PATH = 'C:/msys64/mingw64/bin;' + $env:PATH
    }
    $arguments += if ($WithoutOled) { '-DBB_ENABLE_OLED=OFF' } else { '-DBB_ENABLE_OLED=ON' }
    & $cmake @arguments @CmakeArgs
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $cmake --build $BuildDirectory --parallel 8
    if ($LASTEXITCODE -ne 0) { throw 'Firmware build failed.' }
    & python tools/check_stack.py $BuildDirectory
    if ($LASTEXITCODE -ne 0) { throw 'Static stack audit failed.' }
} finally { Pop-Location }
