@echo off
setlocal
set "SOURCE=%~dp0templates\Ermine Script (MonoBehaviour).zip"
set "DESTDIR=%USERPROFILE%\Documents\Visual Studio 2022\Templates\ItemTemplates\Visual C#\Ermine"

if not exist "%DESTDIR%" mkdir "%DESTDIR%"
copy /Y "%SOURCE%" "%DESTDIR%\Ermine Script (MonoBehaviour).zip" >nul

echo Installed Ermine item template. Restart visual studio to see it in the Add New Item dialog.
endlocal