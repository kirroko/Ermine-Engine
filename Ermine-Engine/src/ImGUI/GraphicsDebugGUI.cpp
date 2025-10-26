/* Start Header ************************************************************************/
/*!
\file       GraphicsDebugGUI.cpp
\author     Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\date       29/9/2025
\brief      This file contains the implementation of the GraphicsDebugGUI class.
            A debug GUI for graphics-related parameters and controls using ImGui.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "GraphicsDebugGUI.h"
#include "imgui.h"
#include "ECS.h"
#include "Renderer.h"
#include "GPUProfiler.h"
#include "Logger.h"
#include "shadow_config.h"
#include "Material.h"
#include "FrameController.h"

using namespace Ermine::editor;
using namespace Ermine::graphics;

// Helper function for formatting numbers
namespace
{
    /**
     * @brief Formats a large integer value into a human-readable string with units (K, M, B, T).
     * @param value The value to format.
     * @return Formatted string.
     */
    std::string FormatNumber(uint64_t value)
    {
        struct Unit { uint64_t base; const char* suffix; };
        static constexpr Unit units[] = {
            {.base= 1'000'000'000'000ULL, .suffix= "T"},
            {.base= 1'000'000'000ULL, .suffix= "B"},
            {.base= 1'000'000ULL, .suffix= "M"},
            {.base= 1'000ULL, .suffix= "K"},
            {.base= 1, .suffix= ""}
        };

        for (const auto& u : units)
        {
            if (value >= u.base)
            {
                char buffer[32];
                const double scaled = static_cast<double>(value) / static_cast<double>(u.base);
                const int written = snprintf(buffer, sizeof(buffer), "%.1f%s", scaled, u.suffix);
                if (written < 0)
                {
                    EE_CORE_WARN("FormatNumber error occurred");
                    return std::to_string(value);
                }
                return std::string(buffer);
            }
        }

        char buffer[32];
        const int written = snprintf(buffer, sizeof(buffer), "%llu", value);
        if (written < 0)
        {
            EE_CORE_WARN("FormatNumber error occurred");
            return std::to_string(value);
        }
        return std::string(buffer);
    }
}

/**
 * @brief Constructs a GraphicsDebugGUI window with the given title.
 * @param title The window title.
 */
GraphicsDebugGUI::GraphicsDebugGUI(const std::string& title)
    : ImGUIWindow(title), m_title(title)
{
}

/**
 * @brief Updates the debug GUI, rendering all graphics-related controls and metrics.
 */
void GraphicsDebugGUI::Update()
{
    auto renderer = ECS::GetInstance().GetSystem<Renderer>();
    if (!renderer) {
        ImGui::Begin(m_title.c_str());
        ImGui::Text("Renderer system not available");
        ImGui::End();
    }
}

/**
 * @brief Renders the GUI window. All rendering is handled in Update().
 */
void GraphicsDebugGUI::Render()
{
    ImGui::Begin(m_title.c_str());

    // Create collapsible sections for organized UI
    DrawRenderingModeControls();
    DrawMaterialControls();
    DrawPostProcessingControls();
    DrawShadowMappingControls();
    DrawLightingControls();
    DrawPerformanceMetrics();

    ImGui::End();
}

/**
 * @brief Draws controls for rendering mode selection and SSAO toggle.
 */
void GraphicsDebugGUI::DrawRenderingModeControls()
{
    auto renderer = ECS::GetInstance().GetSystem<Renderer>();
    
    if (ImGui::CollapsingHeader("Rendering Pipeline", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(10.0f);
        
        // Shading Model Toggle
        bool isBlinnPhong = renderer->GetShadingMode();
        if (ImGui::RadioButton("PBR Shading", !isBlinnPhong)) {
            renderer->SetShadingMode(false);
            EE_CORE_INFO("Switched to PBR shading");
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Blinn-Phong", isBlinnPhong)) {
            renderer->SetShadingMode(true);
            EE_CORE_INFO("Switched to Blinn-Phong shading");
        }
        DrawTooltip("Choose between Physically Based Rendering (PBR) and classic Blinn-Phong shading");
        
        ImGui::Separator();
        
        // SSAO Toggle
        if (DrawToggleButton("Screen Space Ambient Occlusion", &renderer->m_SSAOEnabled, 
                            "Enable/disable Screen Space Ambient Occlusion for enhanced depth perception")) {
            EE_CORE_INFO("SSAO {}", renderer->m_SSAOEnabled ? "enabled" : "disabled");
        }
        
        // SSAO Parameters (shown when SSAO is enabled)
        if (renderer->m_SSAOEnabled && ImGui::TreeNode("SSAO Settings"))
        {
            if (ImGui::SliderInt("Sample Count", &renderer->m_SSAOSamples, 4, 64)) {
                EE_CORE_INFO("SSAO Samples changed to {}", renderer->m_SSAOSamples);
            }
            DrawTooltip("Number of samples for SSAO calculation (higher = better quality but slower)");
            
            DrawFloatSlider("Sampling Radius", &renderer->m_SSAORadius, 0.1f, 50.0f, 
                           "Radius of the sampling hemisphere in world space");
            
            DrawFloatSlider("Bias", &renderer->m_SSAOBias, 0.0f, 0.1f, 
                           "Bias to prevent self-shadowing artifacts");
            
            DrawFloatSlider("Intensity", &renderer->m_SSAOIntensity, 0.0f, 5.0f, 
                           "Strength of the ambient occlusion effect");
            
            DrawFloatSlider("Fadeout Distance", &renderer->m_SSAOFadeout, 0.0f, 1.0f, 
                           "Distance factor for fading out SSAO effect");
            
            DrawFloatSlider("Max Distance", &renderer->m_SSAOMaxDistance, 10.0f, 500.0f, 
                           "Maximum distance for SSAO calculation");
            
            ImGui::TreePop();
        }
        
        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Draws controls for material properties including UV scale and offset.
 */
void GraphicsDebugGUI::DrawMaterialControls()
{
    auto renderer = ECS::GetInstance().GetSystem<Renderer>();

    if (ImGui::CollapsingHeader("Material Editor"))
    {
        ImGui::Indent(10.0f);

        // Entity selection for material editing
        static EntityID selectedEntity = 0;
        auto& ecs = ECS::GetInstance();

        // Get all entities with Material component
        std::vector<EntityID> materialEntities;
        for (EntityID entity = 1; entity <= MAX_ENTITIES; ++entity)
        {
            if (ecs.IsEntityValid(entity) && ecs.HasComponent<Material>(entity))
            {
                materialEntities.push_back(entity);
            }
        }

        if (materialEntities.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No entities with Material component found");
            ImGui::Unindent(10.0f);
            return;
        }

        // Entity selector dropdown
        ImGui::Text("Select Entity:");
        if (ImGui::BeginCombo("##EntitySelector",
            selectedEntity == 0 ? "Select Entity..." :
            (ecs.HasComponent<ObjectMetaData>(selectedEntity) ?
                ecs.GetComponent<ObjectMetaData>(selectedEntity).name.c_str() :
                ("Entity " + std::to_string(selectedEntity)).c_str())))
        {
            for (EntityID entity : materialEntities)
            {
                std::string label = ecs.HasComponent<ObjectMetaData>(entity) ?
                    ecs.GetComponent<ObjectMetaData>(entity).name + " (ID: " + std::to_string(entity) + ")" :
                    "Entity " + std::to_string(entity);

                bool isSelected = (selectedEntity == entity);
                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    selectedEntity = entity;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        DrawTooltip("Select an entity to edit its material properties");

        ImGui::Separator();

        // If entity is selected and valid, show material controls
        if (selectedEntity != 0 && ecs.IsEntityValid(selectedEntity) &&
            ecs.HasComponent<Material>(selectedEntity))
        {
            auto& materialComp = ecs.GetComponent<Material>(selectedEntity);
            auto* material = materialComp.GetMaterial();

            if (!material) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Material pointer is null!");
                ImGui::Unindent(10.0f);
                return;
            }

            // Get material index for GPU updates
            uint32_t materialIndex = renderer->GetMaterialIndex(selectedEntity);
            bool materialChanged = false;

            // === ALBEDO COLOR ===
            if (ImGui::TreeNodeEx("Albedo Color", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Get current albedo value
                auto albedoParam = material->GetParameter("materialAlbedo");
                float albedoColor[3] = { 1.0f, 1.0f, 1.0f };

                if (albedoParam && albedoParam->floatValues.size() >= 3) {
                    albedoColor[0] = albedoParam->floatValues[0];
                    albedoColor[1] = albedoParam->floatValues[1];
                    albedoColor[2] = albedoParam->floatValues[2];
                }

                if (ImGui::ColorEdit3("Color", albedoColor))
                {
                    material->SetVec3("materialAlbedo", Vec3(albedoColor[0], albedoColor[1], albedoColor[2]));
                    materialChanged = true;
                }
                DrawTooltip("Base color of the material (RGB)");

                ImGui::TreePop();
            }

            // === MATERIAL PROPERTIES ===
            if (ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Roughness
                auto roughnessParam = material->GetParameter("materialRoughness");
                float roughness = roughnessParam ? roughnessParam->floatValues[0] : 0.5f;

                if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f"))
                {
                    material->SetFloat("materialRoughness", roughness);
                    materialChanged = true;
                }
                DrawTooltip("Surface roughness (0 = smooth/reflective, 1 = rough/diffuse)");

                // Metallic
                auto metallicParam = material->GetParameter("materialMetallic");
                float metallic = metallicParam ? metallicParam->floatValues[0] : 0.0f;

                if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f"))
                {
                    material->SetFloat("materialMetallic", metallic);
                    materialChanged = true;
                }
                DrawTooltip("Metallic property (0 = dielectric, 1 = metal)");

                // Ambient Occlusion
                auto aoParam = material->GetParameter("materialAo");
                float ao = aoParam ? aoParam->floatValues[0] : 1.0f;

                if (ImGui::SliderFloat("Ambient Occlusion", &ao, 0.0f, 1.0f, "%.3f"))
                {
                    material->SetFloat("materialAo", ao);
                    materialChanged = true;
                }
                DrawTooltip("Ambient occlusion factor (1 = no occlusion, 0 = fully occluded)");

                ImGui::TreePop();
            }

            // === EMISSIVE ===
            if (ImGui::TreeNode("Emissive"))
            {
                auto emissiveParam = material->GetParameter("materialEmissive");
                float emissiveColor[3] = { 0.0f, 0.0f, 0.0f };

                if (emissiveParam && emissiveParam->floatValues.size() >= 3) {
                    emissiveColor[0] = emissiveParam->floatValues[0];
                    emissiveColor[1] = emissiveParam->floatValues[1];
                    emissiveColor[2] = emissiveParam->floatValues[2];
                }

                if (ImGui::ColorEdit3("Emissive Color", emissiveColor))
                {
                    material->SetVec3("materialEmissive", Vec3(emissiveColor[0], emissiveColor[1], emissiveColor[2]));
                    materialChanged = true;
                }
                DrawTooltip("Self-illumination color for glowing effects");

                auto emissiveIntensityParam = material->GetParameter("materialEmissiveIntensity");
                float emissiveIntensity = emissiveIntensityParam ? emissiveIntensityParam->floatValues[0] : 1.0f;

                if (ImGui::SliderFloat("Intensity", &emissiveIntensity, 0.0f, 10.0f, "%.2f"))
                {
                    material->SetFloat("materialEmissiveIntensity", emissiveIntensity);
                    materialChanged = true;
                }
                DrawTooltip("Brightness multiplier for emissive color");

                ImGui::TreePop();
            }

            // === UV TRANSFORM ===
            if (ImGui::TreeNode("UV Transform"))
            {
                Vec2 currentScale = material->GetUVScale();
                Vec2 currentOffset = material->GetUVOffset();

                float uvScale[2] = { currentScale.x, currentScale.y };
                float uvOffset[2] = { currentOffset.x, currentOffset.y };

                if (ImGui::DragFloat2("UV Scale", uvScale, 0.01f, 0.01f, 10.0f, "%.2f"))
                {
                    material->SetUVScale(Vec2(uvScale[0], uvScale[1]));
                    materialChanged = true;
                }
                DrawTooltip("Scale texture coordinates (tiling)");

                if (ImGui::DragFloat2("UV Offset", uvOffset, 0.01f, -10.0f, 10.0f, "%.2f"))
                {
                    material->SetUVOffset(Vec2(uvOffset[0], uvOffset[1]));
                    materialChanged = true;
                }
                DrawTooltip("Offset texture coordinates (scrolling)");

                // Quick presets
                ImGui::Separator();
                if (ImGui::Button("Reset UV"))
                {
                    material->SetUVScale(Vec2(1.0f, 1.0f));
                    material->SetUVOffset(Vec2(0.0f, 0.0f));
                    materialChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Tile 2x2"))
                {
                    material->SetUVScale(Vec2(2.0f, 2.0f));
                    materialChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Tile 4x4"))
                {
                    material->SetUVScale(Vec2(4.0f, 4.0f));
                    materialChanged = true;
                }

                ImGui::TreePop();
            }

            // === TRANSPARENCY ===
            if (ImGui::TreeNode("Transparency"))
            {
                auto albedoParam = material->GetParameter("materialAlbedo");
                float alpha = 1.0f;

                if (albedoParam && albedoParam->type == MaterialParamType::VEC4 &&
                    albedoParam->floatValues.size() >= 4) {
                    alpha = albedoParam->floatValues[3];
                }

                if (ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f, "%.3f"))
                {
                    // Get current RGB values
                    Vec3 rgb(1.0f, 1.0f, 1.0f);
                    if (albedoParam && albedoParam->floatValues.size() >= 3) {
                        rgb.x = albedoParam->floatValues[0];
                        rgb.y = albedoParam->floatValues[1];
                        rgb.z = albedoParam->floatValues[2];
                    }

                    // Update as Vec4
                    material->SetVec4("materialAlbedo", Vec4(rgb.x, rgb.y, rgb.z, alpha));
                    materialChanged = true;
                }
                DrawTooltip("Material transparency (0 = transparent, 1 = opaque)");

                ImGui::TreePop();
            }

            // === APPLY CHANGES ===
            if (materialChanged)
            {
                // Get updated SSBO data
                auto ssboData = material->GetSSBOData();

                // Upload to GPU
                renderer->UpdateMaterialSSBO(ssboData, materialIndex);

                EE_CORE_INFO("Material updated for Entity {0}", selectedEntity);
            }

            ImGui::Separator();

            // === MATERIAL INFO ===
            ImGui::Text("Material Info:");
            ImGui::Text("  Index: %d", materialIndex);
            ImGui::Text("  UV Scale: (%.2f, %.2f)", material->GetUVScale().x, material->GetUVScale().y);
            ImGui::Text("  UV Offset: (%.2f, %.2f)", material->GetUVOffset().x, material->GetUVOffset().y);
        }
        else if (selectedEntity != 0)
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                "Selected entity no longer valid or missing Material component");
        }

        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Draws controls for post-processing effects and their parameters.
 */
void GraphicsDebugGUI::DrawPostProcessingControls()
{
    auto renderer = ECS::GetInstance().GetSystem<Renderer>();
    
    if (ImGui::CollapsingHeader("Post-Processing Effects"))
    {
        ImGui::Indent(10.0f);
        
        // Post-processing toggles
        DrawToggleButton("HDR Tone Mapping", &renderer->m_ToneMappingEnabled, 
                        "Enable High Dynamic Range tone mapping for better exposure control");
        DrawToggleButton("Gamma Correction", &renderer->m_GammaCorrectionEnabled, 
                        "Apply gamma correction for proper color space conversion");
        DrawToggleButton("FXAA Anti-Aliasing", &renderer->m_FXAAEnabled, 
                        "Fast Approximate Anti-Aliasing to reduce jagged edges");
        DrawToggleButton("Bloom Effect", &renderer->m_BloomEnabled, 
                        "Bloom effect for bright light sources");
        DrawToggleButton("Vignette Effect", &renderer->m_VignetteEnabled, 
                        "Vignette darkening at screen borders");
        
        ImGui::Separator();
        
        // Exposure and Color Controls
        if (ImGui::TreeNode("Color & Exposure"))
        {
            DrawFloatSlider("Exposure", &renderer->m_Exposure, 0.1f, 5.0f, 
                           "Controls overall scene brightness");
            DrawFloatSlider("Contrast", &renderer->m_Contrast, 0.5f, 2.0f, 
                           "Adjust contrast between light and dark areas");
            DrawFloatSlider("Saturation", &renderer->m_Saturation, 0.0f, 2.0f, 
                           "Color saturation intensity");
            DrawFloatSlider("Gamma", &renderer->m_Gamma, 1.0f, 3.0f, 
                           "Gamma curve for color correction");
            ImGui::TreePop();
        }
        
        // Bloom Controls
        if (ImGui::TreeNode("Bloom Settings"))
        {
            DrawFloatSlider("Bloom Threshold", &renderer->m_BloomThreshold, 0.1f, 3.0f, 
                           "Brightness threshold for bloom effect");
            DrawFloatSlider("Bloom Strength", &renderer->m_BloomStrength, 0.0f, 1.0f, 
                           "Intensity of bloom effect");
            DrawFloatSlider("Bloom Radius", &renderer->m_BloomRadius, 1.0f, 10.0f, 
                           "Blur radius for bloom effect");
            ImGui::TreePop();
        }
        
        // Vignette Controls
        if (ImGui::TreeNode("Vignette Settings"))
        {
            DrawFloatSlider("Vignette Intensity", &renderer->m_VignetteIntensity, 0.0f, 1.0f, 
                           "Strength of vignette darkening");
            DrawFloatSlider("Vignette Radius", &renderer->m_VignetteRadius, 0.1f, 1.0f, 
                           "Size of vignette effect");
            ImGui::TreePop();
        }
        
        // FXAA Controls
        if (ImGui::TreeNode("FXAA Settings"))
        {
            DrawFloatSlider("FXAA Span Max", &renderer->m_FXAASpanMax, 2.0f, 16.0f, 
                           "Maximum search span for edge detection");
            DrawFloatSlider("FXAA Reduce Min", &renderer->m_FXAAReduceMin, 1.0f/256.0f, 1.0f/32.0f, 
                           "Minimum luminance reduction threshold");
            DrawFloatSlider("FXAA Reduce Mul", &renderer->m_FXAAReduceMul, 1.0f/16.0f, 1.0f/4.0f, 
                           "Luminance reduction multiplier");
            ImGui::TreePop();
        }

        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Draws controls and statistics for shadow mapping configuration.
 */
void GraphicsDebugGUI::DrawShadowMappingControls()
{
    if (ImGui::CollapsingHeader("Shadow Mapping"))
    {
        ImGui::Indent(10.0f);
        
        ImGui::Text("Shadow Map Resolution: %d x %d", SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
        DrawTooltip("Current shadow map resolution - defined in shadow_config.h");
        
        ImGui::Text("Max Shadow Layers: %u", SHADOW_MAX_LAYERS);
        DrawTooltip("Maximum number of shadow map layers available");
        
        ImGui::Text("Cascade Count: %d", NUM_CASCADES);
        DrawTooltip("Number of cascades for Cascaded Shadow Maps (CSM)");
        
        ImGui::Text("CSM Lambda: %.2f", SHADOW_MAP_ARRAY_LAMBDA);
        DrawTooltip("Blend factor between logarithmic and linear cascade distribution");
        
        ImGui::Text("Refresh Interval: %d frames", SHADOW_MAP_REFRESH_INTERVAL_IN_FRAMES);
        DrawTooltip("Shadow maps are refreshed every N frames for performance");
        
        if (ImGui::Button("Force Shadow Refresh")) {
            EE_CORE_INFO("Manual shadow map refresh triggered");
        }
        DrawTooltip("Force refresh all shadow maps (normally updated every few frames)");
        
        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Draws controls and statistics for the lighting system.
 */
void GraphicsDebugGUI::DrawLightingControls()
{
    if (ImGui::CollapsingHeader("Lighting System"))
    {
        ImGui::Indent(10.0f);
        
        ImGui::Text("Maximum Lights: %d", MAX_LIGHTS);
        DrawTooltip("Maximum number of lights that can be processed simultaneously");
        
        // Count active lights
        auto lightSystem = ECS::GetInstance().GetSystem<LightSystem>();
        int activeLights = lightSystem ? static_cast<int>(lightSystem->m_Entities.size()) : 0;
        ImGui::Text("Active Lights: %d", activeLights);
        
        // Show light distribution
        if (lightSystem && activeLights > 0) {
            const auto& ecs = ECS::GetInstance();
            int directionalLights = 0;
            int pointLights = 0;
            int spotLights = 0;
            int shadowCasters = 0;
            
            for (EntityID entity : lightSystem->m_Entities) {
                if (ecs.HasComponent<Light>(entity)) {
                    auto& light = ecs.GetComponent<Light>(entity);
                    switch (light.type) {
                        case LightType::DIRECTIONAL: directionalLights++; break;
                        case LightType::POINT: pointLights++; break;
                        case LightType::SPOT: spotLights++; break;
                    }
                    if (light.castsShadows) shadowCasters++;
                }
            }
            
            ImGui::Text("  Directional: %d", directionalLights);
            ImGui::Text("  Point: %d", pointLights);
            ImGui::Text("  Spot: %d", spotLights);
            ImGui::Text("  Shadow Casters: %d", shadowCasters);
        }
        
        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Draws performance metrics including frame timing, draw calls, and memory usage.
 */
void GraphicsDebugGUI::DrawPerformanceMetrics()
{
    if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(10.0f);
        
        const auto& metrics = GPUProfiler::GetMetrics();
        
        // Frame timing
        float avgFps = metrics.averageFrameTimeMs > 0.0f ? 1000.0f / metrics.averageFrameTimeMs : 0.0f;
        ImGui::Text("FPS: %.1f (avg: %.1f)", metrics.fps, avgFps);
        ImGui::Text("Frame Time: %.2f ms", metrics.frameTimeMs);
        ImGui::Text("CPU Time: %.2f ms", metrics.cpuFrameTimeMs);
        ImGui::Text("GPU Time: %.2f ms", metrics.gpuFrameTimeMs);

		ImGui::Separator();
        
        // Render statistics
        ImGui::Text("Draw Calls: %u", metrics.drawCallCount);
        ImGui::Text("Triangles: %s", FormatNumber(metrics.triangleCount).c_str());
        ImGui::Text("Vertices: %s", FormatNumber(metrics.vertexCount).c_str());
        
        ImGui::Separator();
        
        // Memory usage
        ImGui::Text("VRAM Usage: %llu MB", metrics.totalVRAMUsageMB);
        ImGui::Text("Texture Memory: %llu MB", metrics.textureMemoryMB);
        ImGui::Text("Buffer Memory: %llu MB", metrics.bufferMemoryMB);
        
        // Frame time history graph
        const auto& history = GPUProfiler::GetFrameTimeHistory();
        if (!history.empty()) {
            std::vector<float> values(history.begin(), history.end());
            ImGui::PlotLines("Frame Times (ms)", values.data(), static_cast<int>(values.size()),
                0, nullptr, 0.0f, metrics.maxFrameTimeMs * 1.2f, ImVec2(0, 80));
        }
        
        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Shows a tooltip for the last hovered ImGui item.
 * @param description The tooltip text.
 */
void GraphicsDebugGUI::DrawTooltip(const char* description)
{
    if (ImGui::IsItemHovered() && description) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(description);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

/**
 * @brief Draws a float slider with a label and optional tooltip.
 * @param label The slider label.
 * @param value Pointer to the float value.
 * @param min Minimum slider value.
 * @param max Maximum slider value.
 * @param tooltip Optional tooltip text.
 * @return true if the value was changed.
 */
bool GraphicsDebugGUI::DrawFloatSlider(const char* label, float* value, float min, float max, const char* tooltip)
{
    bool changed = ImGui::SliderFloat(label, value, min, max, "%.3f");
    if (tooltip) DrawTooltip(tooltip);
    return changed;
}

/**
 * @brief Draws a toggle button (checkbox) with a label and optional tooltip.
 * @param label The checkbox label.
 * @param value Pointer to the boolean value.
 * @return true if the value was changed.
 * @param tooltip Optional tooltip text.
 */
bool GraphicsDebugGUI::DrawToggleButton(const char* label, bool* value, const char* tooltip)
{
    bool changed = ImGui::Checkbox(label, value);
    if (tooltip) DrawTooltip(tooltip);
    return changed;
}