# **ErmineEngine by ErmineWorks**

The Ermine Engine is a custom C++ game engine developed by third-year students of DigiPen Singapore.

---

## Overview

Ermine Engine is a data‑driven, editor‑centric 3D engine built around an Entity‑Component‑System (ECS) architecture.  
It ships with a real‑time editor, PBR renderer, integrated physics, audio, scripting, and a content pipeline for managing assets.

---

## Key Features

### Core Architecture

- **Entity‑Component‑System (ECS)** with modular components
- **Hierarchical entity parenting** (parent/child transforms & hierarchy views)
- **Scene management & serialization** (save/load scenes & prefabs)
- **Prefab system** for reusable entities
- **Job system** for multi‑threaded tasks
- **GUID & asset registry** for stable references
- **Frame controller** for timing and frame stepping
- **Logging system** for engine/editor diagnostics

### Rendering & Graphics

- **Physically‑Based Rendering (PBR)** with metal/rough workflows
- **Image‑Based Lighting (IBL)** with skybox/environment cubemaps
- **HDR rendering pipeline** with bloom and tone mapping
- **Gamma correction** and configurable exposure
- **Forward and Deferred shading** paths
- **Screen‑space Ambient Occlusion (SSAO)** toggle
- **Mesh & model management** (static & skinned meshes)
- **Camera and viewport system** (editor and in‑game cameras)
- **GPU profiler** with frame time graphs and performance metrics
- **UI rendering** for text and screen‑space elements
- **Debug drawing & gizmos** (translate/rotate/scale, pivot/local/world)

### Animation & Models

- **Skeletal animation** with Assimp FBX import
- **GPU skinning** (bone transforms via SSBO)
- **Animation clips & animator system**
- **Model import** with materials and textures bound to engine shaders
- **Animation management** for reusing and sharing clips

### Physics

- **Jolt Physics (JPH) integration**
- **Rigid body & trigger volumes**
- **Box, Sphere, Capsule, and Mesh colliders**
- **Dynamic, Static, and Kinematic motion types**
- **Physics debug renderer** integrated with the editor viewport

### Audio

- **3D positional audio** (spatialized emitters & listeners)
- **Global/ambient audio** for music and ambience
- **Centralized audio manager/system**
- **Audio debugging panel** in the editor

### Scripting

- **C# (Script‑Assembly) integration** (script engine & script system)
- **Live script reloading** without restarting the editor
- **Component‑based script attachment** to entities
- **Interop between engine components and gameplay scripts**

### AI, Navigation & Gameplay Tools

- **Finite State Machine (FSM) system** for gameplay logic
- **FSM editor** integrated into the editor UI
- **NavMesh system & agents** for navigation and pathfinding
- **NavMesh visualization & tweaking through the editor**

### Particles

- **GPU‑driven particle systems** (GPUParticles)
- **Configurable particle emitters**
- **Particle editor** with live preview inside the engine

### Editor & Tools

- **Real‑time editor** built with ImGui
- **Viewport window** with in‑editor camera controls
- **Scene Hierarchy panel** with parenting & reordering
- **Inspector window** with components for:
  - Transform, Mesh, Material, Light
  - Physics, Audio, Script, Model, Animation, Particles, etc.
- **Asset Browser**:
  - Browse models, textures, shaders, and prefabs
  - Integration with the asset database/resource pipeline
- **Material editor** with shader parameter controls
- **Console panel** for logs and debug output
- **Settings, video, and debug GUIs**
- **CPU/GPU performance graphs & FPS tracking**
- **Creation/deletion and duplication of entities**
- **Parent/child hierarchy manipulation via UI**

### Asset & Resource Pipeline

- **Asset database** (`AssetDatabase`) with GUID‑based references
- **Resource pipeline** (`ResourcePipe`) for importing & processing assets
- **Support for external resource pipeline project (Ermine‑ResourcePipeline)**
- **Centralized asset manager** for models, textures, shaders, and more

---

## Editor Controls

**Mouse & Camera**

- **Left Mouse**: Select object
- **Right Mouse + WASD**: Move editor camera forward/back & strafe
- **Right Mouse + Q/E**: Move camera down/up
- **Left Shift**: Increase camera movement speed
- **Mouse Scroll**: Zoom in/out

**Play Mode**

- **Ctrl + P**: Enter play mode
- **Ctrl + Shift + P**: Stop play mode (return to editor)

**Gizmos & Transform**

- **Q**: Toggle Local / World transform space
- **W**: Translate mode
- **E**: Rotate mode
- **R**: Scale mode
- **Y**: Toggle Pivot / Center mode (gizmo position)

**Rendering Shortcuts**

- **1 / 2**: Toggle PBR / Blinn‑Phong shading
- **3**: Toggle Deferred Rendering
- **4**: Toggle SSAO

---

## Build Information

1. Run `MakeProject.bat` to generate the Visual Studio project/solution.
2. Open the generated solution in **Visual Studio 2022**.
3. Build both **Engine** and **Script‑Assembly** projects (ensure `Script-Assembly.dll` is produced).
4. Launch the editor executable to begin creating or testing scenes.
5. Ensure required assets and the resource pipeline outputs are correctly placed (see FAQ).

---

## Project Structure (High Level)

- `src/Engine.cpp` – Engine entry & main loop
- `src/ECS/` – Entity, Component, System implementations
- `src/Graphics/` – Renderer, shaders, buffers, camera, animation, UI, skybox, GPU profiler
- `src/Physics/` – Jolt integration, physics world, colliders, debug rendering
- `src/AudioManager/` – Audio engine, audio components & systems
- `src/Scripting/` – Script engine, script system, C# integration
- `src/Particles/` – GPU particle system and tools
- `src/AssetManager/`, `src/ResourcePipe/` – Asset database and resource pipeline bridge
- `src/AssetBrowser/` – Asset browser editor integration
- `src/ImGUI/` – Editor windows (Inspector, Hierarchy, Console, Settings, etc.)
- `src/Serialisation/` – Scene, prefab, and asset serialization
- `src/NavMesh/` – Navigation mesh and agent systems
- `src/FSM/` – Finite State Machine runtime and editor
- `src/Utility/` – Input, editor camera, GUIDs, math helpers, job system, frame controller
- `src/Logger/` – Logging utilities

---

## FAQ / Common Issues

1. **Missing or mismatched `Script-Assembly.dll`**
   - Rebuild the **Script‑Assembly** project from Visual Studio 2022.
   - Verify the generated `Script-Assembly.dll` is next to the engine executable or in the configured search path.

2. **Missing assets or invalid paths**
   - Ensure that the **Ermine-ResourcePipeline** output and `.lion_rcdbase` asset database are in the same directory as the engine executable.
   - Confirm that relative paths inside the editor and config files point to the correct asset folders.

3. **Editor starts but viewport is blank**
   - Confirm a camera exists in the scene and is active.
   - Check that the correct scene is loaded and that the renderer is not disabled via debug settings.

---

## Team Roster

### Programmers

(IMGD)

- Kean (Product Manager | Scripting)
- Ko Sand (Tech Lead | Assets)
- Brian (AI)
- Si Han (Physics & Math)
- Curtis (Serialization)

(RTIS)

- Jeremy (Graphics)
- Kai Rui (Audio)
- Ridhwan (Graphics Lead)
- Edwin (Scene & UI)

### Designers

(UXGD)

- Klive (Design Lead)
- Brandon (Audio Lead | Level Design)
- Fabio (Mechanics & Level Design)

### Artists

(BFA)

- Lera (Art Lead | Texture & Environment)
- Naomi (Character & UI)
- Wei Jia (Animator)

---

## License

© 2026 DigiPen Institute of Technology.  
Reproduction or distribution of this project or its components without prior written consent is prohibited.
