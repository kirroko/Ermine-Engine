@echo off
setlocal

echo ========================================================
echo Preparing installer package and running Inno Setup
echo ========================================================

set "ROOT=%~dp0"
set "INSTALLER_DIR=%ROOT%Installer"
set "ISS_FILE=%INSTALLER_DIR%\InstallScript.iss"
set "STAGING_DIR=%INSTALLER_DIR%\GAMEDIRECTORY"
set "OUTPUT_DIR=%INSTALLER_DIR%\INSTALLER"
set "GAME_BUILD_DIR=%ROOT%dist\Internal\Game-Release"

if not exist "%ISS_FILE%" (
	echo [ERROR] Inno Setup script not found: "%ISS_FILE%"
	exit /b 1
)

if not exist "%GAME_BUILD_DIR%" (
	echo [ERROR] Game build output not found: "%GAME_BUILD_DIR%"
	echo Build Game-Release then run this script again.
	exit /b 1
)

if not exist "%STAGING_DIR%" mkdir "%STAGING_DIR%"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo.
echo Staging files into "%STAGING_DIR%"...
xcopy "%GAME_BUILD_DIR%" "%STAGING_DIR%" /E /I /Y >nul
echo Xcopy exit code: %ERRORLEVEL%
if errorlevel 2 (
    echo [ERROR] Failed to stage files xcopy exit code: %ERRORLEVEL%.
    exit /b 1
)
ren "%STAGING_DIR%\Ermine-Game\Ermine-Game.exe" "Machina.exe"

REM --- Locate Inno Setup Compiler (ISCC.exe) ---
echo Searching for Inno Setup Compiler ISCC.exe in standard locations...
set "ISCC_EXE="
set "PROG86=%ProgramFiles(x86)%"
set "PROG=%ProgramFiles%"

if exist "%PROG86%\Inno Setup 6\ISCC.exe" (
    set "ISCC_EXE=%PROG86%\Inno Setup 6\ISCC.exe"
    goto :iscc_found
)
if exist "%PROG%\Inno Setup 6\ISCC.exe" (
    set "ISCC_EXE=%PROG%\Inno Setup 6\ISCC.exe"
    goto :iscc_found
)
if exist "%PROG86%\Inno Setup 5\ISCC.exe" (
    set "ISCC_EXE=%PROG86%\Inno Setup 5\ISCC.exe"
    goto :iscc_found
)
if exist "%PROG%\Inno Setup 5\ISCC.exe" (
    set "ISCC_EXE=%PROG%\Inno Setup 5\ISCC.exe"
    goto :iscc_found
)

if defined ISCC_EXE goto :iscc_found

echo [ERROR] ISCC.exe not found. Install Inno Setup 6 (or 5) from https://jrsoftware.org/isdl.php/ and try again.
exit /b 1

:iscc_found
echo.
echo Using Inno Setup Compiler: "%ISCC_EXE%"
echo Compiling installer script: "%ISS_FILE%"

pushd "%INSTALLER_DIR%"
call "%ISCC_EXE%" "%ISS_FILE%"
set "ISCC_EXIT=%ERRORLEVEL%"
popd

if not "%ISCC_EXIT%"=="0" (
	echo [ERROR] Inno Setup compilation failed with code %ISCC_EXIT%.
	exit /b %ISCC_EXIT%
)

echo.
echo ========================================================
echo Installer created successfully in "%OUTPUT_DIR%"
echo ========================================================
exit /b 0
