/* Start Header ************************************************************************/
/*!
\file       SceneManager.cpp
\author     WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu
\date       Sep 10, 2025
\brief      Scene management including new, open, save, and file dialogs

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "SceneManager.h"
#include "Serialisation.h"
#include "ECS.h"
#include "Physics.h"
#include "Renderer.h"
#include "Components.h"
#include "Matrix4x4.h"
#include "EditorGUI.h"

namespace
{
    static std::string WideToUTF8(const std::wstring& w)
    {
        if (w.empty()) return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
            nullptr, 0, nullptr, nullptr);
        std::string s(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
            s.data(), size, nullptr, nullptr);
        return s;
    }

    static void SetInitialScenesFolder(IFileDialog* dlg) {
        namespace fs = std::filesystem;
        std::error_code ec;

        // Get the absolute path of the solution (this file's directory)
        fs::path exePath = fs::absolute(fs::current_path());
        fs::path rootPath = exePath;

        // Move up until we find the project root (with "Resources" or "premake5.lua")
        while (!rootPath.empty() &&
            !fs::exists(rootPath / "Resources") &&
            !fs::exists(rootPath / "premake5.lua") &&
            !fs::exists(rootPath / "Ermine.sln")) {
            rootPath = rootPath.parent_path();
        }

        // Target folder
        fs::path scenes = rootPath / "Resources" / "Scenes";

        // Create it if it doesn't exist
        if (!fs::exists(scenes, ec))
            fs::create_directories(scenes, ec);

        // Turn into an IShellItem for the file dialog
        IShellItem* folder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(scenes.wstring().c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
            dlg->SetDefaultFolder(folder);
            dlg->SetFolder(folder);
            folder->Release();
        }
    }

    static void SyncHierarchyGuidsFromRuntime(Ermine::ECS& ecs)
    {
        for (Ermine::EntityID e = 0; e < Ermine::MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<Ermine::HierarchyComponent>(e)) continue;

            auto& hc = ecs.GetComponent<Ermine::HierarchyComponent>(e);

            // --- parentGuid ---
            if (hc.parent != Ermine::HierarchyComponent::INVALID_PARENT
                && ecs.IsEntityValid(hc.parent)
                && ecs.HasComponent<Ermine::IDComponent>(hc.parent))
            {
                const auto& parentID = ecs.GetComponent<Ermine::IDComponent>(hc.parent);
                hc.parentGuid = parentID.guid; // <-- CRITICAL LINE
            }
            else
            {
                // no parent, root object
                hc.parentGuid = Ermine::Guid{}; // zero GUID
            }

            // --- childrenGuids ---
            hc.childrenGuids.clear();
            hc.childrenGuids.reserve(hc.children.size());

            for (Ermine::EntityID childEid : hc.children)
            {
                if (!ecs.IsEntityValid(childEid)) continue;
                if (!ecs.HasComponent<Ermine::IDComponent>(childEid)) continue;

                const auto& childID = ecs.GetComponent<Ermine::IDComponent>(childEid);
                hc.childrenGuids.push_back(childID.guid);
            }
        }
    }

    static void RebuildRuntimeHierarchyFromGuids(Ermine::ECS& ecs)
    {
        using namespace Ermine;

        // 1. Rebuild parent / children EntityIDs based on stored GUIDs
        for (EntityID e = 0; e < MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<HierarchyComponent>(e)) continue;

            auto& hc = ecs.GetComponent<HierarchyComponent>(e);

            // ----- parent -----
            if (hc.parentGuid.IsValid()) // non-zero guid
            {
                EntityID parentEid = ecs.GetGuidRegistry().FindEntity(hc.parentGuid);

                if (ecs.IsEntityValid(parentEid))
                {
                    hc.parent = parentEid;
                }
                else
                {
                    hc.parent = HierarchyComponent::INVALID_PARENT;
                }
            }
            else
            {
                // root entity, no parent
                hc.parent = HierarchyComponent::INVALID_PARENT;
            }

            // ----- children -----
            hc.children.clear();
            hc.children.reserve(hc.childrenGuids.size());

            for (const Guid& cg : hc.childrenGuids)
            {
                if (!cg.IsValid()) continue;

                EntityID childEid = ecs.GetGuidRegistry().FindEntity(cg);
                if (ecs.IsEntityValid(childEid))
                {
                    hc.children.push_back(childEid);
                }
            }

            // We'll recompute depth in a second pass
        }

        // 2. Recompute depth (optional but nice, and prevents stale depths)
        for (EntityID e = 0; e < MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<HierarchyComponent>(e)) continue;

            auto& hc = ecs.GetComponent<HierarchyComponent>(e);

            int d = 0;
            EntityID walk = hc.parent;
            while (walk != HierarchyComponent::INVALID_PARENT &&
                ecs.IsEntityValid(walk) &&
                ecs.HasComponent<HierarchyComponent>(walk))
            {
                ++d;
                walk = ecs.GetComponent<HierarchyComponent>(walk).parent;
            }
            hc.depth = d;

            // force transforms to update next frame
            hc.isDirty = true;
            hc.worldTransformDirty = true;
        }
    }


}

std::optional<std::string> SceneManager::ShowSaveDialog(const wchar_t* defaultFileName, HWND owner) {
    IFileSaveDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dlg))))
        return std::nullopt;

    COMDLG_FILTERSPEC filters[] = {
        { L"Scene files", L"*.scene" },
        { L"All files",   L"*.*" }
    };
    dlg->SetFileTypes(2, filters);
    dlg->SetFileTypeIndex(1);
    dlg->SetDefaultExtension(L"scene");
    if (defaultFileName) dlg->SetFileName(defaultFileName);

    DWORD opts = 0; dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);

    SetInitialScenesFolder(dlg);

    std::optional<std::string> result;
    if (SUCCEEDED(dlg->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                result = WideToUTF8(path);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dlg->Release();
    return result;
}

std::optional<std::string> SceneManager::ShowOpenDialog(HWND owner) {
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dlg))))
        return std::nullopt;

    COMDLG_FILTERSPEC filters[] = {
        { L"Scene files", L"*.scene" },
        { L"All files",   L"*.*" }
    };
    dlg->SetFileTypes(2, filters);
    dlg->SetFileTypeIndex(1);
    dlg->SetDefaultExtension(L"scene");

    DWORD opts = 0; dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);

    SetInitialScenesFolder(dlg);

    std::optional<std::string> result;
    if (SUCCEEDED(dlg->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                result = WideToUTF8(path);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dlg->Release();
    return result;
}

SceneManager& SceneManager::GetInstance()
{
    static SceneManager instance;
    return instance;
}

void SceneManager::NewScene()
{
    // Clear ECS
    Ermine::ECS::GetInstance().ClearAllEntities();

    auto mainLight = Ermine::ECS::GetInstance().CreateEntity();

    // Tilted down and slightly to the side, similar to Unity's default
    Ermine::ECS::GetInstance().AddComponent(
        mainLight,
        Ermine::Transform(
            Ermine::Vec3(0, 5, 0),
            Ermine::FromEulerDegrees(50.0f, -30.0f, 0.0f),
            Ermine::Vec3(1, 1, 1)));

    Ermine::ECS::GetInstance().AddComponent(mainLight, Ermine::ObjectMetaData("Main Light", "Light", true));
    Ermine::ECS::GetInstance().AddComponent(mainLight, Ermine::Light(Ermine::Vec3(1, 1, 1), 1.0f, Ermine::LightType::DIRECTIONAL, true));
    Ermine::ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(mainLight, Ermine::HierarchyComponent{});
    // Mark materials dirty to trigger recompilation
    auto renderer = Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>();
    if (renderer) {
        renderer->MarkMaterialsDirty();
    }
    Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>()->InitializeShadowMapResources();
    Ermine::ECS::GetInstance().GetSystem<Ermine::Physics>()->UpdatePhysicList();

    // Create a new Scene object and sync with ECS
    auto newScene = std::make_shared<Ermine::Scene>("Untitled Scene");
    newScene->EnsureSyncedWithECS(/*force=*/true);

    // Set as active scene in SceneManager
    SetActiveScene(newScene);

    // Notify EditorGUI to update hierarchy panel and inspector
    Ermine::editor::EditorGUI::SetActiveScene(newScene);

    m_CurrentScenePath.reset();
    m_Dirty = false;
}

void SceneManager::ClearScene()
{
    // Clear ECS
    Ermine::ECS::GetInstance().ClearAllEntities();

    //Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>()->UpdateShadowMap();

    // Mark materials dirty to trigger recompilation
    auto renderer = Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>();
    if (renderer) {
        renderer->MarkMaterialsDirty();
    }

    // Create an empty Scene object and sync with ECS
    auto emptyScene = std::make_shared<Ermine::Scene>("Empty Scene");
    emptyScene->EnsureSyncedWithECS(/*force=*/true);

    // Set as active scene in SceneManager
    SetActiveScene(emptyScene);

    // Notify EditorGUI to update hierarchy panel and inspector
    Ermine::editor::EditorGUI::SetActiveScene(emptyScene);

    Ermine::ECS::GetInstance().GetSystem<Ermine::Physics>()->UpdatePhysicList();
    m_CurrentScenePath.reset();
    m_Dirty = false;
}

void SceneManager::OpenSceneDialog()
{
    auto path = ShowOpenDialog(GetActiveWindow());
    if (path) OpenScene(*path);
}

void SceneManager::OpenScene(const std::string& path)
{
    //auto& ecs = Ermine::ECS::GetInstance();
    //EnsureActiveScene().Clear();

    LoadSceneFromFile(Ermine::ECS::GetInstance(), path);

    Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>()->InitializeShadowMapResources();

    // Mark materials dirty to trigger recompilation after scene load
    auto renderer = Ermine::ECS::GetInstance().GetSystem<Ermine::graphics::Renderer>();
    if (renderer) {
        renderer->MarkMaterialsDirty();
    }

    //RebuildRuntimeHierarchyFromGuids(Ermine::ECS::GetInstance());

    // Create Scene object from loaded entities
    std::filesystem::path scenePath(path);
    std::string sceneName = scenePath.stem().string(); // Get filename without extension
    auto newScene = std::make_shared<Ermine::Scene>(sceneName);

    // Sync the Scene object with the loaded ECS entities
    newScene->EnsureSyncedWithECS(/*force=*/true);

    // Set as active scene in SceneManager
    SetActiveScene(newScene);

    // Notify EditorGUI to update hierarchy panel and inspector
    Ermine::editor::EditorGUI::SetActiveScene(newScene);

    m_CurrentScenePath = path;
    m_Dirty = false;
    Ermine::ECS::GetInstance().GetSystem<Ermine::Physics>()->UpdatePhysicList();
}

void SceneManager::SaveScene()
{
    if (!m_CurrentScenePath)
    {
        SaveSceneAsDialog(); // fallback if never saved
        return;
    }
    SaveSceneTo(*m_CurrentScenePath);
}

void SceneManager::SaveTemp()
{
    SaveSceneTo("../Temp/Temp.scene");
}

void SceneManager::LoadTemp()
{
    ClearScene();
    OpenScene("../Temp/Temp.scene");
    RemoveTemp();
}

void SceneManager::RemoveTemp()
{
    // Attempt to delete the file
    int status = remove("../Temp/Temp.scene");
    std::filesystem::remove("Temp");

    EE_CORE_INFO("Removed: {}", status);
}

void SceneManager::SaveSceneAsDialog()
{
    auto path = ShowSaveDialog(L"untitled.scene", GetActiveWindow());
    if (path) SaveSceneTo(*path);
}

void SceneManager::SaveSceneTo(const std::string& path)
{
    //SyncHierarchyGuidsFromRuntime(Ermine::ECS::GetInstance());
    SaveSceneToFile(Ermine::ECS::GetInstance(), path, true);
    m_CurrentScenePath = path;
    m_Dirty = false;
}

void SceneManager::CreateHUDEntity()
{
#if defined(EE_EDITOR)
    // Create HUD entity with UIComponent for rendering UI elements
    auto scene = GetActiveScene();
    if (!scene) {
        EE_CORE_ERROR("SceneManager::CreateHUDEntity() - No active scene!");
        return;
    }

    Ermine::EntityID uiEntity = scene->CreateEntity("HUD", false, false);  // No transform or hierarchy needed
    Ermine::UIComponent uiComp;  // Default values are already set in the struct

    // ============================================================================
    // HUD LAYOUT CONFIGURATION
    // ============================================================================
    uiComp.skillSlotSize = 0.08f;       // Make slots bigger (default: 0.06, now: 0.08 = 33% larger)
    uiComp.skillSlotSpacing = 0.015f;   // Increase spacing between slots

    // ============================================================================
    // CONFIGURE SKILL ICONS (Auto-configured for all scenes)
    // 3 VISUAL SLOTS: LMB (Shoot/Teleport), RMB (Blind Burst), R (Recall)
    // ============================================================================

    // Skill 0: Shoot Orb / Teleport (LMB - Sequential)
    // This slot represents BOTH LMB actions used in sequence
    uiComp.skills[0].iconTexturePath = "../Resources/Textures/UI/Skills/shoot_orb_icon.png";
    uiComp.skills[0].manaCost = 15.0f;
    uiComp.skills[0].maxCooldown = 1.5f;
    uiComp.skills[0].skillName = "Orb Control";
    uiComp.skills[0].keyBinding = "LMB";
    uiComp.skills[0].description = "Shoot orb, then teleport to it";

    // Skill 1: Teleport to Orb (Internal state - not shown as separate slot)
    uiComp.skills[1].iconTexturePath = "../Resources/Textures/UI/Skills/teleport_icon.png";
    uiComp.skills[1].manaCost = 20.0f;
    uiComp.skills[1].maxCooldown = 3.0f;
    uiComp.skills[1].skillName = "Teleport";
    uiComp.skills[1].keyBinding = "LMB";
    uiComp.skills[1].description = "Teleport to the orb's location";

    // Skill 2: Blind Burst
    uiComp.skills[2].iconTexturePath = "../Resources/Textures/UI/Skills/disable_light_icon.png";
    uiComp.skills[2].manaCost = 25.0f;
    uiComp.skills[2].maxCooldown = 5.0f;
    uiComp.skills[2].skillName = "Blind Burst";
    uiComp.skills[2].keyBinding = "RMB";
    uiComp.skills[2].description = "Disable lights and blind enemies";

    // Skill 3: Recall Orb
    uiComp.skills[3].iconTexturePath = "../Resources/Textures/UI/Skills/recall_icon.png";
    uiComp.skills[3].manaCost = 10.0f;
    uiComp.skills[3].maxCooldown = 2.0f;
    uiComp.skills[3].skillName = "Recall";
    uiComp.skills[3].keyBinding = "R";
    uiComp.skills[3].description = "Recall the orb back to you";

    Ermine::ECS::GetInstance().AddComponent<Ermine::UIComponent>(uiEntity, uiComp);
    EE_CORE_INFO("Created HUD entity with UIComponent and skill icons configured");
#endif
}

