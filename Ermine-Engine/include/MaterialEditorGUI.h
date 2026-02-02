/* Start Header ************************************************************************/
/*!
\file       MaterialEditorGUI.h
\author     GitHub Copilot
\date       Feb 02, 2026
\brief      Material editor GUI for creating, editing, saving and loading materials

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "ImGuiUIWindow.h"
#include "Material.h"
#include "Selection.h"
#include <memory>
#include <string>

namespace Ermine::editor
{
    /**
     * @brief Material Editor GUI for creating, editing, and managing materials
     * Provides interface for saving/loading .mat files and editing selected entity materials
     */
    class MaterialEditorGUI : public Ermine::ImGUIWindow
    {
    public:
        /**
         * @brief Constructor
         * @param title Window title
         */
        explicit MaterialEditorGUI(const std::string& title = "Material Editor");

        /**
         * @brief Render the material editor window
         */
        void Render() override;

        /**
         * @brief Set the material to edit
         * @param material Shared pointer to material
         */
        void SetMaterial(std::shared_ptr<Ermine::graphics::Material> material);

        /**
         * @brief Get the current material being edited
         * @return Shared pointer to material
         */
        std::shared_ptr<Ermine::graphics::Material> GetMaterial() const { return m_material; }

    private:
        std::shared_ptr<Ermine::graphics::Material> m_material;
        
        // UI state
        char m_savePathBuffer[256] = "Resources/Materials/NewMaterial.mat";
        char m_loadPathBuffer[256] = "Resources/Materials/";
        bool m_showSaveDialog = false;
        bool m_showLoadDialog = false;
        std::string m_lastError;
        bool m_editingEntityMaterial = false; // Track if we're editing an entity's material
        EntityID m_lastSelectedEntity = 0; // Track last selected entity for auto-loading

        // UI sections
        void DrawEntityMaterialSection();
        void DrawMaterialProperties();
        void DrawFileOperations();
        void DrawTemplateSelection();
        void DrawTextureSettings();
        
        // Helper methods
        void SaveMaterial();
        void LoadMaterial();
        void CreateNewMaterial(const std::string& templateName);
        void LoadEntityMaterial(); // Load material from selected entity
        void ApplyToEntity(); // Apply current material to selected entity
    };
}
