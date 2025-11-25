/* Start Header ************************************************************************/
/*!
\file       UIButtonSystem.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       11/2025
\brief      System for handling UI button interactions (hover, click, actions)
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "Systems.h"
#include "Components.h"

#ifdef EE_EDITOR
#include "imgui.h"
#endif

namespace Ermine
{
    class UIButtonSystem : public System
    {
    public:
        UIButtonSystem() = default;
        ~UIButtonSystem() = default;

        void Init();
        void Update(float deltaTime);

#ifdef EE_EDITOR
        // Set viewport info for editor mode (called from EditorGUI)
        void SetViewportInfo(const ImVec2& min, const ImVec2& size)
        {
            m_viewportMin = min;
            m_viewportSize = size;
        }
#endif

    private:
        // Check if mouse position is within button bounds
        bool IsMouseOverButton(const UIButtonComponent& button, float mouseX, float mouseY);

        // Execute button action
        void ExecuteButtonAction(const UIButtonComponent& button);

        // Get normalized mouse position (0-1 range)
        void GetNormalizedMousePosition(float& outX, float& outY);

        // Track mouse state
        bool m_wasMousePressed = false;

#ifdef EE_EDITOR
        // Viewport tracking for editor mode
        ImVec2 m_viewportMin = { 0, 0 };      // Top-left corner of viewport in screen space
        ImVec2 m_viewportSize = { 1920, 1080 }; // Size of viewport
#endif
    };
}