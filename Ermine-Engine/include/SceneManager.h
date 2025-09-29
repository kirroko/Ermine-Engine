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
