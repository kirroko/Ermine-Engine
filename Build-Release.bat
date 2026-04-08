@echo off
echo ========================================================
echo Starting CD Pipeline for Ermine Engine
echo ========================================================

REM Set variables
set BUILD_CONFIG_GAME_RELEASE=Game-Release
set DIST_DIR=dist\Internal
set VERSION=%date:~-4,4%%date:~-10,2%%date:~-7,2%
set CONFIGS=%BUILD_CONFIG_GAME_RELEASE%

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

REM Clean only this configuration's build output (not entire Build)
if exist "Build\bin\%BUILD_CONFIG%-windows-x86_64" rmdir /S /Q "Build\bin\%BUILD_CONFIG%-windows-x86_64"
if exist "Build\obj\%BUILD_CONFIG%-windows-x86_64" rmdir /S /Q "Build\obj\%BUILD_CONFIG%-windows-x86_64"

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
@REM xcopy /Y /E /I /Q Build\bin\%BUILD_CONFIG%-windows-x86_64\Ermine-Engine\Ermine-Engine.dll %DIST_SUBDIR%\

REM Copy validation script
@REM if exist "validation.sh" (
@REM 	copy /Y validation.sh %DIST_SUBDIR%\validation.sh >nul
@REM ) else (
@REM 	echo WARNING: validation.sh not found.
@REM )

REM --- Run Validation via WSL ---
@REM if exist "%DIST_SUBDIR%\validation.sh" (
@REM 	setlocal EnableDelayedExpansion

@REM     echo Running WSL validation for %BUILD_CONFIG%...
	
@REM 	REM Build absolute Windows path for DIST_SUBDIR
@REM 	set "DIST_SUBDIR_WIN=%CD%\%DIST_SUBDIR%"
    
@REM     REM Convert the Windows path to a WSL path
@REM 	set "DIST_SUBDIR_WSL="
@REM     for /f "delims=" %%I in ('wsl.exe wslpath -u "!DIST_SUBDIR_WIN!"') do set "DIST_SUBDIR_WSL=%%I"

@REM 	if not defined DIST_SUBDIR_WSL (
@REM 		echo [ERROR] Failed to convert DIST_SUBDIR to WSL path.
@REM 		endlocal
@REM 		exit /b 1
@REM 	)

@REM 	REM Execute the script inside WSL
@REM     wsl.exe bash -lc "cd '!DIST_SUBDIR_WSL!' && sed -i 's/\r$//' ./validation.sh && chmod +x ./validation.sh && ./validation.sh"
    
@REM     if errorlevel 1 (
@REM         echo [ERROR] Validation failed in WSL.
@REM 		endlocal
@REM         exit /b 1
@REM     )
@REM )

REM Create version file
@REM echo Version: %VERSION% > %DIST_SUBDIR%\VERSION.txt
@REM echo Config: %BUILD_CONFIG% >> %DIST_SUBDIR%\VERSION.txt
@REM echo Build Date: %date% %time% >> %DIST_SUBDIR%\VERSION.txt

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
echo.
echo ========================================
echo ALL PACKAGES CREATED SUCCESSFULLY
echo ========================================
exit /b 0

:error
echo.
echo ========================================================
echo [FATAL ERROR] CD PIPELINE FAILED!
echo ========================================================
exit /b 1