# Repository Guidelines

## Project Structure & Module Organization
- `Ermine-Engine/include` and `Ermine-Engine/src` hold the core C++ engine code.
- `Ermine-Editor/src` contains editor-only UI and tooling; `Ermine-Game/src` is the runtime game entry.
- `Ermine-ScriptAssembly` and `Ermine-ScriptSandbox` contain C# gameplay scripts and examples.
- `Ermine-ResourcePipeline` and `Resources` store asset pipeline data and shipped content.
- `ThirdParty` vendors external dependencies; avoid modifying these unless explicitly required.
- Build outputs land in `Build/bin/<config>-<system>-<arch>/`.

## Build, Test, and Development Commands
- `MakeProject.bat` generates the Visual Studio 2022 solution via Premake.
- Open `Ermine.sln` in VS2022, choose a config like `Editor-Debug`, and build `Ermine-Engine`, `Ermine-Editor`, `Ermine-Game`, and `Ermine-ScriptAssembly`.
- Run the editor from VS or `Build/bin/<config>/Ermine-Editor/Ermine-Editor.exe`.

## Architecture Overview
- `Ermine-Engine` is the shared runtime (ECS, rendering, physics, audio, serialization, asset system).
- `Ermine-Editor` hosts editor-only UI panels, tooling, and play/stop orchestration on top of the engine.
- `Ermine-Game` is a lightweight game launcher that uses the engine without editor tooling.
- C# scripts in `Ermine-ScriptAssembly` are loaded by the scripting layer and can be hot-reloaded during editor sessions.

## Coding Style & Naming Conventions
- C++ targets C++20; include `PreCompile.h` first in engine/editor sources.
- Indentation is 4 spaces, braces are on the next line, and files typically begin with the standard header banner.
- Use PascalCase for file names and public types; follow existing naming in nearby code.
- No repo-wide formatter is configured; keep edits consistent with surrounding files.

## Testing Guidelines
- No dedicated test suite is wired up for the engine; most validation is manual.
- Verify editor startup, play/stop mode, and any affected tools or scripts after changes.
- Third-party tests under `ThirdParty/` are not run as part of normal development.

## Commit & Pull Request Guidelines
- Recent history uses short, direct summaries like "Update GPUProfiler.cpp" or "Motion Blur fix, Shader optimizations"; keep messages concise and descriptive.
- PRs should include a clear description, testing notes (what you ran), and screenshots or clips for editor/UI changes.
- If assets or pipeline data change, call out the affected paths under `Resources/` or `Ermine-ResourcePipeline/`.

## Configuration Tips
- The asset database files (`Ermine-Game.lion_rcdbase` / `.lion_project`) must sit next to the built executable for the editor to load assets correctly.
