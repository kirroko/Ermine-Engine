# Ensure VS Installer tools are present (vswhere is installed with VS/Build Tools)
$vswhere = Join-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer" "vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found. Install Visual Studio 2022 (or Build Tools) with the MSBuild component."
    exit 1
}

# Find the latest VS instance with MSBuild and return MSBuild.exe
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild -or -not (Test-Path $msbuild)) {
    Write-Error "MSBuild.exe not found. Ensure 'MSBuild' workload/component is installed in Visual Studio."
    exit 2
}

$msbuildDir = Split-Path -Parent $msbuild
Write-Host "Found MSBuild at: $msbuild"

# Add to User PATH if not present
$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
if ($userPath -notlike "*$msbuildDir*") {
    $newUserPath = if ([string]::IsNullOrEmpty($userPath)) { $msbuildDir } else { "$userPath;$msbuildDir" }
    [Environment]::SetEnvironmentVariable('Path', $newUserPath, 'User')
    Write-Host "Added to User PATH: $msbuildDir"
} else {
    Write-Host "MSBuild directory already in User PATH."
}

# Update current session PATH
$env:Path = "$env:Path;$msbuildDir"

# Verify
try {
    & msbuild -version
} catch {
    Write-Warning "msbuild is not available in this shell yet. Open a new terminal to use it."
}