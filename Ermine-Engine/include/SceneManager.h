/* Start Header ************************************************************************/
/*!
\file       SceneManager.h
\author     WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu
\date       Sep 10, 2025
\brief      Scene management including new, open, save, and file dialogs

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include <string>
#include <optional>

class SceneManager
{
public:
    static SceneManager& GetInstance();

    // File operations
    void NewScene();
    void OpenSceneDialog();
    void SaveScene();
    void SaveSceneAsDialog();

    // Direct path-based API
    void OpenScene(const std::string& path);
    void SaveSceneTo(const std::string& path);

    // Accessors
    std::optional<std::string> GetCurrentScenePath() const { return m_CurrentScenePath; }
    bool IsDirty() const { return m_Dirty; }
    void MarkDirty(bool dirty = true) { m_Dirty = dirty; }

    static std::optional<std::string> ShowSaveDialog(
        const wchar_t* defaultFileName = L"untitled.scene",
        HWND owner = nullptr);

    static std::optional<std::string> ShowOpenDialog(HWND owner = nullptr);

private:
    SceneManager() = default;

    std::optional<std::string> m_CurrentScenePath; // full file path
    bool m_Dirty = false; // set true when scene modified
};
