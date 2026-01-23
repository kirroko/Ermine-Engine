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
#include "AssetManager.h"

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
    DrawPostProcessingControls();
    DrawShadowMappingControls();
    DrawLightingControls();
    DrawLightProbeTools();
    DrawPerformanceMetrics();
    DrawShaderControls();

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

        ImGui::Separator();

        // Fog Toggle
        if (DrawToggleButton("Distance-Based Fog", &renderer->m_FogEnabled,
                            "Enable/disable atmospheric fog based on distance from camera")) {
            EE_CORE_INFO("Fog {}", renderer->m_FogEnabled ? "enabled" : "disabled");
        }

        // Fog Parameters (shown when Fog is enabled)
        if (renderer->m_FogEnabled && ImGui::TreeNode("Fog Settings"))
        {
            const char* fogModes[] = { "Linear", "Exponential", "Exponential Squared" };
            if (ImGui::Combo("Fog Mode", &renderer->m_FogMode, fogModes, 3)) {
                EE_CORE_INFO("Fog mode changed to {}", fogModes[renderer->m_FogMode]);
            }
            DrawTooltip("Linear = smooth fade between start/end\nExponential = natural fog falloff\nExp² = most realistic atmospheric fog");

            // Fog color picker
            if (ImGui::ColorEdit3("Fog Color", &renderer->m_FogColor.r)) {
                EE_CORE_INFO("Fog color changed to ({:.2f}, {:.2f}, {:.2f})",
                           renderer->m_FogColor.r, renderer->m_FogColor.g, renderer->m_FogColor.b);
            }
            DrawTooltip("RGB color of the fog");

            // Linear fog parameters
            if (renderer->m_FogMode == 0) {
                DrawFloatSlider("Fog Start", &renderer->m_FogStart, 0.0f, 500.0f,
                               "Distance where fog begins to appear (linear mode)");
                DrawFloatSlider("Fog End", &renderer->m_FogEnd, 1.0f, 1000.0f,
                               "Distance where fog is fully opaque (linear mode)");
            }
            // Exponential fog parameters
            else {
                DrawFloatSlider("Fog Density", &renderer->m_FogDensity, 0.0f, 0.1f,
                               "Fog density for exponential modes (lower = less dense)");
            }

            ImGui::Separator();

            // Height-based fog parameters
            ImGui::Text("Height-Based Fog");
            DrawFloatSlider("Height Influence", &renderer->m_FogHeightCoefficient, 0.0f, 1.0f,
                "How much height affects fog density (0=disabled, 1=maximum effect)");
            DrawFloatSlider("Height Falloff", &renderer->m_FogHeightFalloff, 1.0f, 100.0f,
                "Height at which fog starts to thin out (lower=fog stays near ground)");

            ImGui::TreePop();
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

        ImGui::Separator();

        // Motion Blur Toggle
        if (DrawToggleButton("Motion Blur", &renderer->m_MotionBlurEnabled,
                            "Enable motion blur based on camera and object movement")) {
            EE_CORE_INFO("Motion blur {}", renderer->m_MotionBlurEnabled ? "enabled" : "disabled");
        }

        // Motion Blur Controls
        if (renderer->m_MotionBlurEnabled && ImGui::TreeNode("Motion Blur Settings"))
        {
            DrawFloatSlider("Blur Strength", &renderer->m_MotionBlurStrength, 0.0f, 3.0f,
                           "Intensity of motion blur effect");

            if (ImGui::SliderInt("Sample Count", &renderer->m_MotionBlurSamples, 2, 32)) {
                EE_CORE_INFO("Motion blur samples changed to {}", renderer->m_MotionBlurSamples);
            }
            DrawTooltip("Number of samples for motion blur (higher = smoother blur but slower)");

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

        ImGui::Separator();

        // Volumetric Spotlight Rays
        auto renderer = ECS::GetInstance().GetSystem<Renderer>();
        if (renderer) {
            if (DrawToggleButton("Volumetric Spotlight Rays", &renderer->m_SpotlightRaysEnabled,
                                "Enable volumetric god rays for spotlights")) {
                EE_CORE_INFO("Spotlight rays {}", renderer->m_SpotlightRaysEnabled ? "enabled" : "disabled");
            }

            // Spotlight ray parameters (shown when enabled)
            if (renderer->m_SpotlightRaysEnabled && ImGui::TreeNode("Spotlight Ray Settings"))
            {
                DrawFloatSlider("Ray Intensity", &renderer->m_SpotlightRayIntensity, 0.0f, 2.0f,
                               "Brightness of volumetric god rays from spotlights");
                DrawFloatSlider("Ray Falloff", &renderer->m_SpotlightRayFalloff, 0.5f, 5.0f,
                               "How quickly rays fade with distance");

                ImGui::TreePop();
            }

            ImGui::Separator();

            // Ambient Lighting Controls
            if (DrawToggleButton("Global Ambient Light", &renderer->m_AmbientLightEnabled,
                                "Enable/disable global ambient lighting for the scene")) {
                EE_CORE_INFO("Ambient lighting {}", renderer->m_AmbientLightEnabled ? "enabled" : "disabled");
            }

            // Ambient light parameters (shown when enabled)
            if (renderer->m_AmbientLightEnabled && ImGui::TreeNode("Ambient Light Settings"))
            {
                // Ambient color picker
                if (ImGui::ColorEdit3("Ambient Color", &renderer->m_AmbientColor.r)) {
                    EE_CORE_INFO("Ambient color changed to ({:.2f}, {:.2f}, {:.2f})",
                               renderer->m_AmbientColor.r, renderer->m_AmbientColor.g, renderer->m_AmbientColor.b);
                }
                DrawTooltip("RGB color of ambient light (use warm/cool tints for atmosphere)");

                DrawFloatSlider("Ambient Intensity", &renderer->m_AmbientIntensity, 0.0f, 2.0f,
                               "Strength of ambient lighting (0=none, 1=normal, 2=very bright)");

                DrawFloatSlider("AO Influence", &renderer->m_AmbientOcclusionStrength, 0.0f, 1.0f,
                               "How much ambient occlusion affects ambient light (0=no effect, 1=full darkening)");

                ImGui::TreePop();
            }

            ImGui::Separator();

            // Light Probe System Controls
            if (DrawToggleButton("Use Light Probes", &renderer->m_UseLightProbes,
                                "Enable/disable Spherical Harmonics light probe system for dynamic ambient")) {
                EE_CORE_INFO("Light probe system {}", renderer->m_UseLightProbes ? "enabled" : "disabled");
                renderer->m_LightProbesDirty = true;
            }

            // Light probe parameters (shown when enabled)
            if (renderer->m_UseLightProbes && ImGui::TreeNode("Light Probe Settings"))
            {
                int maxProbes = renderer->m_MaxLightProbes;
                if (ImGui::SliderInt("Max Blend Probes", &maxProbes, 1, 8)) {
                    renderer->m_MaxLightProbes = maxProbes;
                }
                DrawTooltip("Maximum number of nearby probes to blend (higher = smoother but slower)");

                // Show number of active probes in scene
                size_t probeCount = renderer->GetLightProbeCount();
                ImGui::Text("Active Probes in Scene: %zu", probeCount);
                DrawTooltip("Number of AmbientLightProbe components found in the scene");

                // Button to force probe refresh
                if (ImGui::Button("Refresh Probes")) {
                    renderer->m_LightProbesDirty = true;
                    EE_CORE_INFO("Forced light probe refresh");
                }
                DrawTooltip("Force update of cached light probe data");

                ImGui::Separator();
                ImGui::Text("Global Fallback SH Coefficients:");
                DrawTooltip("Used when no probes are nearby");

                // Show first 3 SH coefficients (most important)
                if (ImGui::TreeNode("SH L0 & L1 Bands"))
                {
                    ImGui::ColorEdit3("L0 (DC)", &renderer->m_LightProbeSH[0].r);
                    DrawTooltip("L0: Average ambient color (constant term)");
                    
                    ImGui::ColorEdit3("L1,-1 (Y)", &renderer->m_LightProbeSH[1].r);
                    ImGui::ColorEdit3("L1,0 (Z)", &renderer->m_LightProbeSH[2].r);
                    ImGui::ColorEdit3("L1,1 (X)", &renderer->m_LightProbeSH[3].r);
                    DrawTooltip("L1: Linear gradients (directional ambient)");
                    
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }

        ImGui::Unindent(10.0f);
    }
}

/**
 * @brief Draws performance metrics including frame timing, draw calls, and memory usage.
 */
void GraphicsDebugGUI::DrawPerformanceMetrics()
{
    auto renderer = ECS::GetInstance().GetSystem<Renderer>();

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
        ImGui::Text("Meshes Culled: %u", metrics.culledMeshes);

        ImGui::Separator();

        // Debug Visualization Controls
        if (renderer) {
            ImGui::Text("Debug Visualization:");

            if (DrawToggleButton("Show AABBs", &renderer->m_DebugDrawAABBs,
                                "Draw bounding boxes for all meshes (Green = visible, Red = culled)")) {
                EE_CORE_INFO("AABB visualization {}", renderer->m_DebugDrawAABBs ? "enabled" : "disabled");
            }

            if (DrawToggleButton("Show Frustum", &renderer->m_DebugDrawFrustum,
                                "Draw camera frustum planes (Cyan)")) {
                EE_CORE_INFO("Frustum visualization {}", renderer->m_DebugDrawFrustum ? "enabled" : "disabled");
            }

            if (DrawToggleButton("Show Light Probe Gizmos", &renderer->m_DebugDrawProbes,
                                "Draw light probe spheres showing influence radius")) {
                EE_CORE_INFO("Probe gizmos {}", renderer->m_DebugDrawProbes ? "enabled" : "disabled");
            }

            ImGui::Separator();

            // Draw Data Rebuild Control
            if (ImGui::Button("Force Draw Data Rebuild")) {
                renderer->ForceDrawDataRebuild();
                EE_CORE_INFO("Draw data rebuild triggered manually");
            }
            DrawTooltip("Force a full rebuild of draw commands and shadow buffers on the next frame");
        }

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

void GraphicsDebugGUI::DrawShaderControls()
{
    if (!ImGui::CollapsingHeader("Shader Tools", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Indent(10.0f);
    if (ImGui::Button("Recompile Shaders"))
    {
        AssetManager::GetInstance().ReloadCachedShaders();
    }
    DrawTooltip("Recompile all cached shaders from disk");
    ImGui::Unindent(10.0f);
}

/**
 * @brief Draws comprehensive light probe testing and management tools
 */
void GraphicsDebugGUI::DrawLightProbeTools()
{
    auto renderer = ECS::GetInstance().GetSystem<Renderer>();
    if (!renderer) return;

    if (ImGui::CollapsingHeader("Light Probe Tools & Testing", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(10.0f);

        // === PROBE CREATION TOOLS ===
        if (ImGui::TreeNode("Probe Creation"))
        {
            static float probeRadius = 10.0f;
            static float probeSpacing = 5.0f;
            static char probeName[128] = "TestProbe";
            static float probePos[3] = { 0.0f, 0.0f, 0.0f };

            ImGui::InputText("Probe Name", probeName, sizeof(probeName));
            DrawTooltip("Name for the new probe entity");

            ImGui::DragFloat3("Position", probePos, 0.1f);
            DrawTooltip("World-space position for the new probe");

            DrawFloatSlider("Influence Radius", &probeRadius, 1.0f, 50.0f,
                           "How far this probe affects surrounding areas");

            if (ImGui::Button("Create Single Probe", ImVec2(200, 0)))
            {
                Vec3 position = Vec3{ probePos[0], probePos[1], probePos[2] };
                EntityID newProbe = renderer->CreateLightProbeEntity(position, probeRadius, probeName);
                EE_CORE_INFO("Created probe entity {} at ({}, {}, {})", newProbe, position.x, position.y, position.z);
            }
            DrawTooltip("Create a new light probe at the specified position");

            ImGui::Separator();

            // Grid generation
            ImGui::Text("Grid Generation");
            static int gridX = 3, gridY = 1, gridZ = 3;
            ImGui::DragInt("Grid X", &gridX, 0.1f, 1, 20);
            ImGui::DragInt("Grid Y", &gridY, 0.1f, 1, 10);
            ImGui::DragInt("Grid Z", &gridZ, 0.1f, 1, 20);
            DrawFloatSlider("Probe Spacing", &probeSpacing, 2.0f, 20.0f,
                           "Distance between probes in the grid");

            if (ImGui::Button("Generate Probe Grid", ImVec2(200, 0)))
            {
                Vec3 startPos = Vec3{ probePos[0], probePos[1], probePos[2] };
                int count = 0;
                for (int x = 0; x < gridX; ++x) {
                    for (int y = 0; y < gridY; ++y) {
                        for (int z = 0; z < gridZ; ++z) {
                            Vec3 gridPos = Vec3{
                                startPos.x + x * probeSpacing,
                                startPos.y + y * probeSpacing,
                                startPos.z + z * probeSpacing
                            };
                            std::string name = std::string(probeName) + "_" + std::to_string(count++);
                            renderer->CreateLightProbeEntity(gridPos, probeRadius, name);
                        }
                    }
                }
                EE_CORE_INFO("Generated {} probes in a {}x{}x{} grid", count, gridX, gridY, gridZ);
            }
            DrawTooltip("Create a grid of probes for volumetric coverage");

            ImGui::TreePop();
        }

        ImGui::Separator();

        // === PROBE MANAGEMENT ===
        if (ImGui::TreeNode("Probe Management"))
        {
            auto& ecs = ECS::GetInstance();
            std::vector<EntityID> probeEntities;

            // Find all probe entities
            for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
                if (!ecs.IsEntityValid(entity)) continue;
                if (ecs.HasComponent<AmbientLightProbe>(entity)) {
                    probeEntities.push_back(entity);
                }
            }

            ImGui::Text("Total Probes in Scene: %zu", probeEntities.size());

            if (!probeEntities.empty())
            {
                static int selectedProbeIdx = 0;
                selectedProbeIdx = std::clamp(selectedProbeIdx, 0, (int)probeEntities.size() - 1);

                // Probe selector
                if (ImGui::BeginCombo("Select Probe", std::to_string(probeEntities[selectedProbeIdx]).c_str()))
                {
                    for (size_t i = 0; i < probeEntities.size(); ++i) {
                        EntityID entity = probeEntities[i];
                        bool isSelected = (selectedProbeIdx == i);
                        std::string label = "Entity " + std::to_string(entity);
                        
                        if (ecs.HasComponent<ObjectMetaData>(entity)) {
                            auto& meta = ecs.GetComponent<ObjectMetaData>(entity);
                            label = meta.name + " (ID:" + std::to_string(entity) + ")";
                        }

                        if (ImGui::Selectable(label.c_str(), isSelected)) {
                            selectedProbeIdx = i;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                EntityID selectedEntity = probeEntities[selectedProbeIdx];
                auto& probe = ecs.GetComponent<AmbientLightProbe>(selectedEntity);

                // Probe properties editor
                ImGui::Separator();
                ImGui::Text("Probe Properties:");

                ImGui::Checkbox("Active##probe", &probe.isActive);
                ImGui::Checkbox("Use Spherical Harmonics", &probe.useSphericalHarmonics);
                ImGui::Checkbox("Show Gizmo", &probe.showGizmo);

                DrawFloatSlider("Influence Radius##edit", &probe.influenceRadius, 1.0f, 100.0f,
                               "Radius of influence for this probe");
                DrawFloatSlider("Blend Weight##edit", &probe.blendWeight, 0.0f, 2.0f,
                               "Weight for blending (1.0 = normal)");

                if (ImGui::ColorEdit3("Gizmo Color", &probe.gizmoColor.x)) {
                    // Color updated
                }

                // Position info
                if (ecs.HasComponent<Transform>(selectedEntity)) {
                    auto& trans = ecs.GetComponent<Transform>(selectedEntity);
                    ImGui::Text("Position: (%.2f, %.2f, %.2f)", trans.position.x, trans.position.y, trans.position.z);
                }

                ImGui::Separator();

                // Capture button
                if (ImGui::Button("Capture Lighting##selected", ImVec2(200, 0)))
                {
                    Vec3 probePos = Vec3{ 0, 0, 0 };
                    if (ecs.HasComponent<Transform>(selectedEntity)) {
                        probePos = ecs.GetComponent<Transform>(selectedEntity).position;
                    }
                    if (renderer->CaptureLightProbe(probePos, selectedEntity)) {
                        EE_CORE_INFO("Captured lighting for probe {}", selectedEntity);
                        renderer->m_LightProbesDirty = true;
                    }
                }
                DrawTooltip("Capture ambient lighting from the scene at this probe's position");

                if (ImGui::Button("Delete Probe##selected", ImVec2(200, 0)))
                {
                    ecs.DestroyEntity(selectedEntity);
                    EE_CORE_INFO("Deleted probe entity {}", selectedEntity);
                    renderer->m_LightProbesDirty = true;
                    selectedProbeIdx = 0;
                }
                DrawTooltip("Delete this probe entity");

                ImGui::Separator();

                // SH Coefficient Viewer
                if (ImGui::TreeNode("SH Coefficients (Advanced)"))
                {
                    ImGui::Text("Spherical Harmonics L2 (9 RGB coefficients):");
                    for (int i = 0; i < 9; ++i) {
                        std::string label = "SH[" + std::to_string(i) + "]";
                        ImGui::ColorEdit3(label.c_str(), &probe.shCoefficients[i].x);
                    }
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "No probes in scene. Create some above!");
            }

            ImGui::TreePop();
        }

        ImGui::Separator();

        // === BATCH OPERATIONS ===
        if (ImGui::TreeNode("Batch Operations"))
        {
            auto& ecs = ECS::GetInstance();
            
            if (ImGui::Button("Capture All Probes", ImVec2(200, 0)))
            {
                int capturedCount = 0;
                for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
                    if (!ecs.IsEntityValid(entity)) continue;
                    if (!ecs.HasComponent<AmbientLightProbe>(entity)) continue;
                    
                    Vec3 probePos = Vec3{ 0, 0, 0 };
                    if (ecs.HasComponent<Transform>(entity)) {
                        probePos = ecs.GetComponent<Transform>(entity).position;
                    }
                    
                    if (renderer->CaptureLightProbe(probePos, entity)) {
                        capturedCount++;
                    }
                }
                EE_CORE_INFO("Captured lighting for {} probes", capturedCount);
                renderer->m_LightProbesDirty = true;
            }
            DrawTooltip("Capture ambient lighting for ALL probes in the scene");

            if (ImGui::Button("Delete All Probes", ImVec2(200, 0)))
            {
                std::vector<EntityID> toDelete;
                for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
                    if (!ecs.IsEntityValid(entity)) continue;
                    if (ecs.HasComponent<AmbientLightProbe>(entity)) {
                        toDelete.push_back(entity);
                    }
                }
                for (EntityID entity : toDelete) {
                    ecs.DestroyEntity(entity);
                }
                EE_CORE_INFO("Deleted {} probe entities", toDelete.size());
                renderer->m_LightProbesDirty = true;
            }
            DrawTooltip("WARNING: Deletes ALL probe entities in the scene");

            ImGui::TreePop();
        }

        ImGui::Separator();

        // === TESTING & VISUALIZATION ===
        if (ImGui::TreeNode("Testing & Visualization"))
        {
            ImGui::Text("Probe System Status:");
            ImGui::Text("  System Enabled: %s", renderer->m_UseLightProbes ? "YES" : "NO");
            ImGui::Text("  Cached Probes: %zu", renderer->GetLightProbeCount());
            ImGui::Text("  Cache Dirty: %s", renderer->m_LightProbesDirty ? "YES" : "NO");
            ImGui::Text("  Max Blend Probes: %d", renderer->m_MaxLightProbes);
            ImGui::Text("  Gizmo Drawing: %s", renderer->m_DebugDrawProbes ? "ENABLED" : "DISABLED");

            if (ImGui::Button("Print Probe Debug Info", ImVec2(200, 0)))
            {
                auto& ecs = ECS::GetInstance();
                int totalProbes = 0;
                int visibleProbes = 0;
                
                for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
                    if (!ecs.IsEntityValid(entity)) continue;
                    if (!ecs.HasComponent<AmbientLightProbe>(entity)) continue;
                    
                    totalProbes++;
                    auto& probe = ecs.GetComponent<AmbientLightProbe>(entity);
                    
                    if (probe.showGizmo) visibleProbes++;
                    
                    Vec3 pos = Vec3{0, 0, 0};
                    if (ecs.HasComponent<Transform>(entity)) {
                        pos = ecs.GetComponent<Transform>(entity).position;
                    }
                    
                    EE_CORE_INFO("Probe Entity {}: pos=({:.1f},{:.1f},{:.1f}) active={} showGizmo={} radius={:.1f}",
                        entity, pos.x, pos.y, pos.z, probe.isActive, probe.showGizmo, probe.influenceRadius);
                }
                
                EE_CORE_INFO("Total probes: {} | Visible gizmos: {} | m_DebugDrawProbes: {}",
                    totalProbes, visibleProbes, renderer->m_DebugDrawProbes);
            }
            DrawTooltip("Print detailed info about all probes to console");

            ImGui::Separator();

            // Test different probe setups
            ImGui::Text("Quick Test Setups:");
            
            if (ImGui::Button("Setup: Outdoor Day", ImVec2(200, 0)))
            {
                // Create a single probe with outdoor day lighting
                Vec3 pos = Vec3{ 0.0f, 0.0f, 0.0f };
                EntityID probe = renderer->CreateLightProbeEntity(pos, 50.0f, "OutdoorDay");
                
                Vec3 skyColor = Vec3{ 0.5f, 0.7f, 1.0f };  // Blue sky
                Vec3 groundColor = Vec3{ 0.3f, 0.25f, 0.2f }; // Brown ground
                Vec3 lightDir = Vec3{ 0.5f, -1.0f, 0.3f };
                Vec3 lightColor = Vec3{ 1.0f, 0.95f, 0.85f }; // Warm sunlight
                
                renderer->SetProbeSHFromDirectionalLight(probe, skyColor, groundColor, lightDir, lightColor, 1.2f);
                renderer->m_LightProbesDirty = true;
                EE_CORE_INFO("Created outdoor day probe setup");
            }

            if (ImGui::Button("Setup: Indoor Warm", ImVec2(200, 0)))
            {
                Vec3 pos = Vec3{ 0.0f, 0.0f, 0.0f };
                EntityID probe = renderer->CreateLightProbeEntity(pos, 20.0f, "IndoorWarm");
                
                Vec3 skyColor = Vec3{ 0.8f, 0.7f, 0.6f };  // Warm ceiling
                Vec3 groundColor = Vec3{ 0.4f, 0.3f, 0.25f };
                Vec3 lightDir = Vec3{ 0.0f, -1.0f, 0.0f };
                Vec3 lightColor = Vec3{ 1.0f, 0.9f, 0.7f };
                
                renderer->SetProbeSHFromDirectionalLight(probe, skyColor, groundColor, lightDir, lightColor, 0.8f);
                renderer->m_LightProbesDirty = true;
                EE_CORE_INFO("Created indoor warm probe setup");
            }

            if (ImGui::Button("Setup: Night/Cool", ImVec2(200, 0)))
            {
                Vec3 pos = Vec3{ 0.0f, 0.0f, 0.0f };
                EntityID probe = renderer->CreateLightProbeEntity(pos, 30.0f, "NightCool");
                
                Vec3 skyColor = Vec3{ 0.1f, 0.15f, 0.3f };  // Dark blue night
                Vec3 groundColor = Vec3{ 0.05f, 0.05f, 0.1f };
                Vec3 lightDir = Vec3{ 0.2f, -0.8f, 0.5f };
                Vec3 lightColor = Vec3{ 0.6f, 0.7f, 0.9f }; // Cool moonlight
                
                renderer->SetProbeSHFromDirectionalLight(probe, skyColor, groundColor, lightDir, lightColor, 0.3f);
                renderer->m_LightProbesDirty = true;
                EE_CORE_INFO("Created night/cool probe setup");
            }

            ImGui::TreePop();
        }

        ImGui::Unindent(10.0f);
    }
}

