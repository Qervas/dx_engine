# DX12 Engine Build Script
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",

    [switch]$Clean,
    [switch]$Run,
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"
$BuildDir = "build"

Write-Host "=== DX12 Engine Build ===" -ForegroundColor Cyan
Write-Host "Configuration: $Config" -ForegroundColor Yellow

# Clean build directory if requested
if ($Clean -or $Rebuild) {
    if (Test-Path $BuildDir) {
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $BuildDir
    }
}

# Create build directory if it doesn't exist
if (-not (Test-Path $BuildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure with CMake if needed
$CacheFile = Join-Path $BuildDir "CMakeCache.txt"
if (-not (Test-Path $CacheFile)) {
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    cmake -B $BuildDir -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
}

# Build
Write-Host "Building ($Config)..." -ForegroundColor Yellow
cmake --build $BuildDir --config $Config

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green

# Run if requested
if ($Run) {
    $ExePath = Join-Path $BuildDir "$Config\dx_engine.exe"
    if (Test-Path $ExePath) {
        Write-Host "Running $ExePath..." -ForegroundColor Yellow
        & $ExePath
    } else {
        Write-Host "Executable not found: $ExePath" -ForegroundColor Red
        exit 1
    }
}
