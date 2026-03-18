@echo off
echo ========================================================
echo Starting CI Pipeline for Ermine Engine
echo ========================================================

REM Set variables
set BUILD_CONFIG_EDITOR_DEBUG=Editor-Debug
set BUILD_CONFIG_EDITOR_RELEASE=Editor-Release
set BUILD_CONFIG_GAME_DEBUG=Game-Debug
set BUILD_CONFIG_GAME_RELEASE=Game-Release
set DIST_DIR=dist\Internal
set VERSION=%date:~-4,4%%date:~-10,2%%date:~-7,2%
set CONFIGS=%BUILD_CONFIG_EDITOR_DEBUG% %BUILD_CONFIG_EDITOR_RELEASE% %BUILD_CONFIG_GAME_DEBUG% %BUILD_CONFIG_GAME_RELEASE%

REM Stage 1 would be Linting, Code Formatting and Static Analysis
REM NOTE; Check if eslint and other tools available.

REM Stage 2: Build

REM Find MSBuild from Visual Studio
set "MSBUILD_EXE="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD_EXE=%%I"
        goto :msbuild_found
    )
)

@REM for %%I in (
@REM     "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
@REM     "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
@REM ) do (
@REM     if exist %%~I (
@REM         set "MSBUILD_EXE=%%~I"
@REM         goto :msbuild_found
@REM     )
@REM )

echo MSBuild executable not found. Exiting script.
goto :error

:msbuild_found
echo Starting build with MSBuild...
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

for %%C in (%CONFIGS%) do (
	call :build_and_copy "%%~C"
	if errorlevel 1 goto:error
)
goto:success

:build_and_copy
set BUILD_CONFIG=%~1
set DIST_SUBDIR=%DIST_DIR%\%BUILD_CONFIG%

echo.
echo --------------------------------------------------------
echo Building configuration: %BUILD_CONFIG%
echo --------------------------------------------------------

REM Clean previous build
echo Cleaning previous build artifacts for configuration: %BUILD_CONFIG%
if exist "%DIST_SUBDIR%" rmdir /S /Q "%DIST_SUBDIR%"
mkdir "%DIST_SUBDIR%"
if exist "Build" rmdir /S /Q "Build"

REM Build the solution
echo Building solution with MSBuild for configuration: %BUILD_CONFIG%
call "%MSBUILD_EXE%" Ermine.sln /p:Configuration=%BUILD_CONFIG% /p:Platform=x64 /v:quiet /nologo >nul
if errorlevel 1 (
	echo Build failed for configuration: %BUILD_CONFIG%
	exit /b 1
)

REM Copy project folders
echo Copying build artifacts for configuration: %BUILD_CONFIG%
if /I not "%BUILD_CONFIG:Editor=%"=="%BUILD_CONFIG%" (
	xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\Ermine-Editor %DIST_SUBDIR%\Ermine-Editor\ >nul
	
) else if /I not "%BUILD_CONFIG:Game=%"=="%BUILD_CONFIG%" (
	xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\Ermine-Game %DIST_SUBDIR%\Ermine-Game\ >nul
) else (
	echo WARNING: Unrecognized build configuration: %BUILD_CONFIG%
)
xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\Ermine-Game.lion_rcdbase %DIST_SUBDIR%\Ermine-Game.lion_rcdbase\
xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\recastnavigation %DIST_SUBDIR%\recastnavigation\
xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\Resources %DIST_SUBDIR%\Resources\

REM Copy DLLs
xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\Ermine-Engine\Ermine-Engine.dll %DIST_SUBDIR%\

REM Copy validation script
if exist "validation.sh" (
	copy /Y validation.sh %DIST_SUBDIR%\validation.sh >nul
) else (
	echo WARNING: validation.sh not found.
)

REM --- Run Validation via WSL ---
if exist "%DIST_SUBDIR%\validation.sh" (
	setlocal EnableDelayedExpansion

    echo Running WSL validation for %BUILD_CONFIG%...
	
	REM Build absolute Windows path for DIST_SUBDIR
	set "DIST_SUBDIR_WIN=%CD%\%DIST_SUBDIR%"
    
    REM Convert the Windows path to a WSL path
	set "DIST_SUBDIR_WSL="
    for /f "delims=" %%I in ('wsl.exe wslpath -u "!DIST_SUBDIR_WIN!"') do set "DIST_SUBDIR_WSL=%%I"

	if not defined DIST_SUBDIR_WSL (
		echo [ERROR] Failed to convert DIST_SUBDIR to WSL path.
		endlocal
		exit /b 1
	)

	REM Execute the script inside WSL
    wsl.exe bash -lc "cd '!DIST_SUBDIR_WSL!' && chmod +x ./validation.sh && ./validation.sh"
    
    if errorlevel 1 (
        echo [ERROR] Validation failed in WSL.
		endlocal
        exit /b 1
    )
)

REM Create version file
echo Version: %VERSION% > %DIST_SUBDIR%\VERSION.txt
echo Config: %BUILD_CONFIG% >> %DIST_SUBDIR%\VERSION.txt
echo Build Date: %date% %time% >> %DIST_SUBDIR%\VERSION.txt

REM Create ZIP archive
@REM echo Creating archive...
@REM call powershell Compress-Archive -Path %DIST_SUBDIR% -DestinationPath ErmineEngine-%BUILD_CONFIG%-%VERSION%.zip -Force
@REM if errorlevel 1 (
@REM 	echo Failed to create archive for configuration: %BUILD_CONFIG%
@REM 	exit /b 1
@REM )

exit /b 0
REM End of build_and_copy

:success
rmdir /S /Q "%DIST_DIR%"
echo.
echo ========================================
echo ALL PACKAGES CREATED SUCCESSFULLY
echo ========================================
exit /b 0

:error
echo.
echo ========================================================
echo [FATAL ERROR] CI PIPELINE FAILED!
echo ========================================================
exit /b 1