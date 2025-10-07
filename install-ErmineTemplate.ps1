# Copies a template ZIP from .\.templates into the vs 2022 ItemTemplates folder.
$source = Join-Path $PSScriptRoot ".\templates\ErmineTemplate.zip"
$destination = Join-Path $env:USERPROFILE "Documents\Visual Studio 2022\Templates\ItemTemplates\Visual C#"

New-Item -ItemType Directory -Force -Path $destination | Out-Null
Copy-Item -Force $source (Join-Path $destination "Ermine Script (MonoBehaviour).zip")

Write-Host "Installed Ermine item template. Restart visual studio to see it in the Add New Item dialog."