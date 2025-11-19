/* Start Header ************************************************************************/
/*!
\file       UIButtonSystem.h
\author     Claude Code
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

namespace Ermine
{
    class UIButtonSystem : public System
    {
    public:
        UIButtonSystem() = default;
        ~UIButtonSystem() = default;

        void Init();
        void Update(float deltaTime);

    private:
        // Check if mouse position is within button bounds
        bool IsMouseOverButton(const UIButtonComponent& button, float mouseX, float mouseY);

        // Execute button action
        void ExecuteButtonAction(const UIButtonComponent& button);

        // Get normalized mouse position (0-1 range)
        void GetNormalizedMousePosition(float& outX, float& outY);

        // Track mouse state
        bool m_wasMousePressed = false;
    };
}
