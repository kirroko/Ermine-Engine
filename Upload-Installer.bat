@echo off
setlocal

echo ========================================================
echo Uploading installer to GitHub release
echo ========================================================

set "ROOT=%~dp0"
set "INSTALLER_DIR=%ROOT%Installer"
set "OUTPUT_DIR=%INSTALLER_DIR%\INSTALLER"

REM Get the installer filename (find the .exe in OUTPUT_DIR)
for %%f in ("%OUTPUT_DIR%\*.exe") do (
    set "INSTALLER_FILE=%%f"
)

if not defined INSTALLER_FILE (
    echo [ERROR] No installer .exe found in "%OUTPUT_DIR%"
    exit /b 1
)

echo Found installer: %INSTALLER_FILE%

REM Get release tag from command line argument
if "%~1"=="" (
    echo [ERROR] Release tag not provided.
    echo Usage: %~nx0 ^<release-tag^>
    echo Example: %~nx0 v1.0.0
    exit /b 1
)

set "RELEASE_TAG=%~1"

echo.
echo Uploading "%INSTALLER_FILE%" to release %RELEASE_TAG%...
gh release upload "%RELEASE_TAG%" "%INSTALLER_FILE%" --clobber

if errorlevel 1 (
    echo [ERROR] Failed to upload to GitHub release.
    exit /b 1
)

echo.
echo ========================================================
echo Installer uploaded successfully to %RELEASE_TAG%
echo ========================================================
exit /b 0
