/* Start Header ************************************************************************/
/*!
\file       MaterialEditorGUI.cpp
\author     GitHub Copilot
\date       Feb 02, 2026
\brief      Implementation of Material Editor GUI

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "MaterialEditorGUI.h"
#include "Serialisation.h"
#include "Logger.h"
#include "Components.h"
#include "ECS.h"
#include "HierarchySystem.h"
#include <imgui.h>
#include <filesystem>

namespace Ermine::editor
{
    MaterialEditorGUI::MaterialEditorGUI(const std::string& title)
        : ImGUIWindow(title)
    {
        // Create a default material
        m_material = std::make_shared<Ermine::graphics::Material>();
        m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_WHITE());
    }

    void MaterialEditorGUI::SetMaterial(std::shared_ptr<Ermine::graphics::Material> material)
    {
        if (material) {
            m_material = material;
        }
    }

    void MaterialEditorGUI::Render()
    {
        if (!ImGui::Begin(Name().c_str(), nullptr, ImGuiWindowFlags_MenuBar))
        {
            ImGui::End();
            return;
        }

        // Auto-load material from selected entity
        auto& ecs = ECS::GetInstance();
        EntityID selectedEntity = Selection::Primary();
        
        // Check if selection changed to a valid entity with Material component
        if (selectedEntity != m_lastSelectedEntity && selectedEntity != 0 && ecs.IsEntityValid(selectedEntity))
        {
            if (ecs.HasComponent<Material>(selectedEntity))
            {
                auto& matComponent = ecs.GetComponent<Material>(selectedEntity);
                if (matComponent.m_material)
                {
                    // Automatically load the selected entity's material
                    m_material = matComponent.m_material;
                    m_editingEntityMaterial = true;
                    m_lastError.clear();
                }
            }
            else
            {
                // Entity has no material, switch to standalone editing mode
                m_editingEntityMaterial = false;
            }
            m_lastSelectedEntity = selectedEntity;
        }
        // If selection was cleared (selectedEntity == 0)
        else if (selectedEntity == 0 && m_lastSelectedEntity != 0)
        {
            m_editingEntityMaterial = false;
            m_lastSelectedEntity = 0;
        }

        if (!m_material)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No material loaded");
            ImGui::End();
            return;
        }

        // Draw entity material selection section
        DrawEntityMaterialSection();
        ImGui::Separator();

        // Draw sections
        DrawFileOperations();
        ImGui::Separator();
        DrawTemplateSelection();
        ImGui::Separator();
        DrawMaterialProperties();
        ImGui::Separator();
        DrawTextureSettings();

        // Display errors if any
        if (!m_lastError.empty())
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", m_lastError.c_str());
            if (ImGui::Button("Clear Error"))
            {
                m_lastError.clear();
            }
        }

        ImGui::End();
    }

    void MaterialEditorGUI::DrawEntityMaterialSection()
    {
        if (ImGui::CollapsingHeader("Entity Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& ecs = ECS::GetInstance();
            EntityID selectedEntity = Selection::Primary();

            if (selectedEntity != 0 && ecs.IsEntityValid(selectedEntity))
            {
                ImGui::Text("Selected Entity: %u", selectedEntity);

                if (ecs.HasComponent<Material>(selectedEntity))
                {
                    if (m_editingEntityMaterial)
                    {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Auto-loaded (editing in real-time)");
                    }
                    
                    ImGui::Spacing();
                    
                    if (ImGui::Button("Refresh from Entity"))
                    {
                        LoadEntityMaterial();
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(?)");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Reload the material from the entity if it changed externally");
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Selected entity has no Material component");
                    
                    if (ImGui::Button("Apply Material to Entity"))
                    {
                        // Add Material component with current material
                        ApplyToEntity();
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("No entity selected");
                ImGui::Spacing();
                ImGui::TextWrapped("Select an entity with a Material component to edit it in real-time");
                
                if (m_editingEntityMaterial)
                {
                    ImGui::Spacing();
                    if (ImGui::Button("Create Standalone Material"))
                    {
                        // Create new standalone material
                        m_material = std::make_shared<Ermine::graphics::Material>();
                        m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_WHITE());
                        m_editingEntityMaterial = false;
                    }
                }
            }
        }
    }

    void MaterialEditorGUI::DrawFileOperations()
    {
        if (ImGui::CollapsingHeader("File Operations", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Save Material
            ImGui::InputText("Save Path", m_savePathBuffer, sizeof(m_savePathBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Save Material"))
            {
                SaveMaterial();
            }

            ImGui::Spacing();

            // Load Material
            ImGui::InputText("Load Path", m_loadPathBuffer, sizeof(m_loadPathBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Load Material"))
            {
                LoadMaterial();
            }

            ImGui::Spacing();

            // New Material
            if (ImGui::Button("New Material (White PBR)"))
            {
                CreateNewMaterial("PBR_WHITE");
            }
        }
    }

    void MaterialEditorGUI::DrawTemplateSelection()
    {
        if (ImGui::CollapsingHeader("Material Templates"))
        {
            if (ImGui::Button("PBR White"))
            {
                m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_WHITE());
            }
            ImGui::SameLine();
            if (ImGui::Button("PBR Red"))
            {
                m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_RED());
            }
            ImGui::SameLine();
            if (ImGui::Button("PBR Metal"))
            {
                m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_METAL());
            }

            if (ImGui::Button("Glass"))
            {
                m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_GLASS());
            }
            ImGui::SameLine();
            if (ImGui::Button("Water"))
            {
                m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_WATER());
            }
            ImGui::SameLine();
            if (ImGui::Button("Emissive (Green)"))
            {
                m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::EMISSIVE(Vec3(0.0f, 1.0f, 0.0f), 5.0f));
            }
        }
    }

    void MaterialEditorGUI::DrawMaterialProperties()
    {
        if (ImGui::CollapsingHeader("Material Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Albedo Color
            if (auto param = m_material->GetParameter("materialAlbedo"))
            {
                if (param->floatValues.size() >= 4)
                {
                    float color[4] = { param->floatValues[0], param->floatValues[1], 
                                      param->floatValues[2], param->floatValues[3] };
                    if (ImGui::ColorEdit4("Albedo", color))
                    {
                        m_material->SetVec4("materialAlbedo", Vec4(color[0], color[1], color[2], color[3]));
                    }
                }
            }

            // Metallic
            if (auto param = m_material->GetParameter("materialMetallic"))
            {
                float value = param->floatValues.empty() ? 0.0f : param->floatValues[0];
                if (ImGui::SliderFloat("Metallic", &value, 0.0f, 1.0f))
                {
                    m_material->SetFloat("materialMetallic", value);
                }
            }

            // Roughness
            if (auto param = m_material->GetParameter("materialRoughness"))
            {
                float value = param->floatValues.empty() ? 0.5f : param->floatValues[0];
                if (ImGui::SliderFloat("Roughness", &value, 0.0f, 1.0f))
                {
                    m_material->SetFloat("materialRoughness", value);
                }
            }

            // Ambient Occlusion
            if (auto param = m_material->GetParameter("materialAo"))
            {
                float value = param->floatValues.empty() ? 1.0f : param->floatValues[0];
                if (ImGui::SliderFloat("AO", &value, 0.0f, 1.0f))
                {
                    m_material->SetFloat("materialAo", value);
                }
            }

            ImGui::Spacing();
            ImGui::Text("Emissive");
            ImGui::Indent();

            // Emissive Color
            if (auto param = m_material->GetParameter("materialEmissive"))
            {
                if (param->floatValues.size() >= 3)
                {
                    float color[3] = { param->floatValues[0], param->floatValues[1], param->floatValues[2] };
                    if (ImGui::ColorEdit3("Emissive Color", color))
                    {
                        m_material->SetVec3("materialEmissive", Vec3(color[0], color[1], color[2]));
                    }
                }
            }

            // Emissive Intensity
            if (auto param = m_material->GetParameter("materialEmissiveIntensity"))
            {
                float value = param->floatValues.empty() ? 0.0f : param->floatValues[0];
                if (ImGui::SliderFloat("Emissive Intensity", &value, 0.0f, 10.0f))
                {
                    m_material->SetFloat("materialEmissiveIntensity", value);
                }
            }

            ImGui::Unindent();
            ImGui::Spacing();

            // Normal Strength
            if (auto param = m_material->GetParameter("materialNormalStrength"))
            {
                float value = param->floatValues.empty() ? 1.0f : param->floatValues[0];
                if (ImGui::SliderFloat("Normal Strength", &value, 0.0f, 2.0f))
                {
                    m_material->SetFloat("materialNormalStrength", value);
                }
            }

            // Shading Model
            if (auto param = m_material->GetParameter("materialShadingModel"))
            {
                int value = param->intValue;
                if (ImGui::SliderInt("Shading Model", &value, 0, 3))
                {
                    m_material->SetInt("materialShadingModel", value);
                }
            }

            // Casts Shadows
            if (auto param = m_material->GetParameter("materialCastsShadows"))
            {
                bool value = param->boolValue;
                if (ImGui::Checkbox("Casts Shadows", &value))
                {
                    m_material->SetBool("materialCastsShadows", value);
                }
            }
        }
    }

    void MaterialEditorGUI::DrawTextureSettings()
    {
        if (ImGui::CollapsingHeader("Texture Settings"))
        {
            // UV Transform
            Vec2 uvScale = m_material->GetUVScale();
            if (ImGui::DragFloat2("UV Scale", &uvScale.x, 0.1f, 0.01f, 100.0f))
            {
                m_material->SetUVScale(uvScale);
            }

            Vec2 uvOffset = m_material->GetUVOffset();
            if (ImGui::DragFloat2("UV Offset", &uvOffset.x, 0.01f, -10.0f, 10.0f))
            {
                m_material->SetUVOffset(uvOffset);
            }

            ImGui::Spacing();
            ImGui::Text("Texture Maps");
            ImGui::Indent();

            // Display texture flags
            auto displayTextureFlag = [&](const char* label, const char* paramName) {
                if (auto param = m_material->GetParameter(paramName))
                {
                    bool value = param->boolValue;
                    if (ImGui::Checkbox(label, &value))
                    {
                        m_material->SetBool(paramName, value);
                    }
                }
            };

            displayTextureFlag("Albedo Map", "materialHasAlbedoMap");
            displayTextureFlag("Normal Map", "materialHasNormalMap");
            displayTextureFlag("Roughness Map", "materialHasRoughnessMap");
            displayTextureFlag("Metallic Map", "materialHasMetallicMap");
            displayTextureFlag("AO Map", "materialHasAoMap");
            displayTextureFlag("Emissive Map", "materialHasEmissiveMap");

            ImGui::Unindent();
            
            ImGui::Spacing();
            ImGui::TextWrapped("Note: Texture loading requires AssetManager integration. Use the Asset Browser to assign textures.");
        }
    }

    void MaterialEditorGUI::SaveMaterial()
    {
        try
        {
            std::filesystem::path savePath(m_savePathBuffer);
            
            // Ensure .mat extension
            if (savePath.extension() != ".mat")
            {
                savePath.replace_extension(".mat");
            }

            SaveMaterialToFile(*m_material, savePath);
            
            EE_CORE_INFO("Material saved to: {}", savePath.string());
            m_lastError.clear();
        }
        catch (const std::exception& e)
        {
            m_lastError = e.what();
            EE_CORE_ERROR("Failed to save material: {}", e.what());
        }
    }

    void MaterialEditorGUI::LoadMaterial()
    {
        try
        {
            std::filesystem::path loadPath(m_loadPathBuffer);
            
            // Ensure .mat extension
            if (loadPath.extension() != ".mat")
            {
                loadPath.replace_extension(".mat");
            }

            if (!std::filesystem::exists(loadPath))
            {
                throw std::runtime_error("File does not exist: " + loadPath.string());
            }

            auto loadedMaterial = LoadMaterialFromFile(loadPath);
            
            // Copy loaded material to current material
            *m_material = loadedMaterial;
            
            EE_CORE_INFO("Material loaded from: {}", loadPath.string());
            m_lastError.clear();
        }
        catch (const std::exception& e)
        {
            m_lastError = e.what();
            EE_CORE_ERROR("Failed to load material: {}", e.what());
        }
    }

    void MaterialEditorGUI::CreateNewMaterial(const std::string& templateName)
    {
        if (templateName == "PBR_WHITE")
        {
            m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_WHITE());
        }
        else if (templateName == "PBR_RED")
        {
            m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_RED());
        }
        else if (templateName == "PBR_METAL")
        {
            m_material->LoadTemplate(Ermine::graphics::MaterialTemplates::PBR_METAL());
        }
        
        EE_CORE_INFO("Created new material from template: {}", templateName);
        m_lastError.clear();
        m_editingEntityMaterial = false; // No longer editing entity material
    }

    void MaterialEditorGUI::LoadEntityMaterial()
    {
        auto& ecs = ECS::GetInstance();
        EntityID selectedEntity = Selection::Primary();

        if (selectedEntity == 0 || !ecs.IsEntityValid(selectedEntity))
        {
            m_lastError = "No valid entity selected";
            return;
        }

        if (!ecs.HasComponent<Material>(selectedEntity))
        {
            m_lastError = "Selected entity has no Material component";
            return;
        }

        auto& matComponent = ecs.GetComponent<Material>(selectedEntity);
        if (matComponent.m_material)
        {
            // Edit the entity's material directly (shared_ptr)
            m_material = matComponent.m_material;
            m_editingEntityMaterial = true;
            EE_CORE_INFO("Loaded material from entity {} (real-time editing)", selectedEntity);
            m_lastError.clear();
        }
        else
        {
            m_lastError = "Entity's material is null";
        }
    }

    void MaterialEditorGUI::ApplyToEntity()
    {
        auto& ecs = ECS::GetInstance();
        EntityID selectedEntity = Selection::Primary();

        if (selectedEntity == 0 || !ecs.IsEntityValid(selectedEntity))
        {
            m_lastError = "No valid entity selected";
            return;
        }

        if (!ecs.HasComponent<Material>(selectedEntity))
        {
            m_lastError = "Selected entity has no Material component";
            return;
        }

        auto& matComponent = ecs.GetComponent<Material>(selectedEntity);
        matComponent.m_material = m_material; // Share the material
        m_editingEntityMaterial = true;
        
        EE_CORE_INFO("Applied material to entity {}", selectedEntity);
        m_lastError.clear();
    }
}
