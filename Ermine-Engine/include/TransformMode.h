/* Start Header ************************************************************************/
/*!
\file       TransformMode.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      Declares the TransformMode class which provides functionality for switching
            between pivot and center transform modes.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "MathVector.h"
#include "ECS.h"

namespace Ermine::editor {

    /**
     * @brief Transform manipulation modes (Unity-style)
     */
    enum class TransformMode {
        Pivot,   // Manipulate around individual object pivots
        Center   // Manipulate around selection center
    };

    /**
     * @brief Utility class for transform mode functionality
     */
    class TransformModeManager {
    public:
        /**
         * @brief Get the current transform mode
         */
        static TransformMode GetMode() { return s_currentMode; }
        
        /**
         * @brief Set the transform mode
         */
        static void SetMode(TransformMode mode) { s_currentMode = mode; }
        
        /**
         * @brief Toggle between pivot and center modes
         */
        static TransformMode ToggleMode() {
            s_currentMode = (s_currentMode == TransformMode::Pivot)
                ? TransformMode::Center
                : TransformMode::Pivot;

            return s_currentMode;
        }
        
        /**
         * @brief Calculate the manipulation position for given entities
         * @param entities List of selected entities
         * @return Position to use for gizmo manipulation
         */
        static Vec3 GetManipulationPosition(const std::vector<EntityID>& entities);
        
        /**
         * @brief Get the manipulation position for a single entity
         * @param entity The entity to get position for
         * @return Position to use for gizmo manipulation
         */
        static Vec3 GetManipulationPosition(EntityID entity);

    private:
        static TransformMode s_currentMode;
    };

} // namespace Ermine::editor