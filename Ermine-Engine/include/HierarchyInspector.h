/* Start Header ************************************************************************/
/*!
\file       HierarchyInspector.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       27/03/2025
\brief      Inspector panel for viewing and editing entity properties

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "Scene.h"
#include "HierarchyPanel.h"

namespace Ermine::editor {

class HierarchyInspector {
public:
    HierarchyInspector() = default;
    void OnImGuiRender();

    void SetScene(Scene* scene) { m_ActiveScene = scene; }
    Scene* GetScene() const { return m_ActiveScene; }

    void SetVisible(bool visible) { m_IsVisible = visible; }
    bool IsVisible() const { return m_IsVisible; }

private:
    void DrawEntityHeader(EntityID entity);
    void DrawTransformComponent(EntityID entity);
    void DrawMeshComponent(EntityID entity);
    void DrawMaterialComponent(EntityID entity);
    void DrawLightComponent(EntityID entity);
    void DrawHierarchyComponent(EntityID entity);
    void DrawAddComponentMenu(EntityID entity);

    Scene* m_ActiveScene = nullptr;
    bool m_IsVisible = true;
};

} // namespace Ermine::editor