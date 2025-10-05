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

        fs::path scenes = fs::absolute(fs::path("Resources") / "Scenes");
        if (!fs::exists(scenes, ec)) fs::create_directories(scenes, ec); // best effort

        // Turn into an IShellItem
        IShellItem* folder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(scenes.wstring().c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
            // Default folder when the dialog is first shown:
            dlg->SetDefaultFolder(folder);
            // Also set current folder (useful if the dialog would otherwise restore last-used):
            dlg->SetFolder(folder);
            folder->Release();
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
    auto& ecs = Ermine::ECS::GetInstance();
    //EnsureActiveScene().Clear();

    LoadSceneFromFile(Ermine::ECS::GetInstance(), path);

    if (auto scene = GetActiveScene()) {
        scene->EnsureSyncedWithECS();
    }

    m_CurrentScenePath = path;
    m_Dirty = false;
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

void SceneManager::SaveSceneAsDialog()
{
    auto path = ShowSaveDialog(L"untitled.scene", GetActiveWindow());
    if (path) SaveSceneTo(*path);
}

void SceneManager::SaveSceneTo(const std::string& path)
{
    SaveSceneToFile(Ermine::ECS::GetInstance(), path, true);
    m_CurrentScenePath = path;
    m_Dirty = false;
}
