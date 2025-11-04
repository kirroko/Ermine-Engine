/* Start Header ************************************************************************/
/*!
\file       UIRenderSystem.cpp
\author     Claude Code Assistant
\date       04/11/2025
\brief      Implementation of the UIRenderSystem for rendering HUD elements.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "UIRenderSystem.h"
#include "ECS.h"
#include "AssetManager.h"
#include "Logger.h"
#include <cmath>

#if defined(EE_EDITOR)
#include "EditorGUI.h"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Ermine
{
    void UIRenderSystem::Init(int screenWidth, int screenHeight)
    {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;

        // Load UI shader
        m_uiShader = AssetManager::GetInstance().LoadShader(
            "../Resources/Shaders/ui_vertex.glsl",
            "../Resources/Shaders/ui_fragment.glsl"
        );

        if (!m_uiShader || !m_uiShader->IsValid())
        {
            EE_CORE_ERROR("UIRenderSystem: Failed to load UI shaders");
            return;
        }

        // Create orthographic projection matrix (0,0 at bottom-left, 1,1 at top-right)
        m_orthoProjection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

        // Setup VAO and VBO for dynamic quad rendering
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        // Reserve space for vertex data (position + color)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 1024, nullptr, GL_DYNAMIC_DRAW);

        // Position attribute (x, y)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

        // Color attribute (r, g, b, a)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);

        EE_CORE_INFO("UIRenderSystem initialized successfully");
    }

    void UIRenderSystem::Update(float deltaTime)
    {
        // Iterate through all entities with UIComponent
        for (EntityID entity : m_Entities)
        {
            auto& ui = ECS::GetInstance().GetComponent<UIComponent>(entity);

            // Note: Mana regeneration disabled for life essence system
            // Life essence (health) does not regenerate automatically
            // If you want life essence regeneration, uncomment the code below and modify to use currentHealth
            /*
            if (ui.currentMana < ui.maxMana)
            {
                ui.manaRegenTimer += deltaTime;

                // Only regenerate mana after the delay
                if (ui.manaRegenTimer >= ui.manaRegenDelay)
                {
                    ui.currentMana += ui.manaRegenRate * deltaTime;
                    if (ui.currentMana > ui.maxMana)
                        ui.currentMana = ui.maxMana;
                }
            }
            */

            // Update skill cooldowns
            for (auto& skill : ui.skills)
            {
                if (skill.isOnCooldown && skill.currentCooldown > 0.0f)
                {
                    skill.currentCooldown -= deltaTime;
                    if (skill.currentCooldown <= 0.0f)
                    {
                        skill.currentCooldown = 0.0f;
                        skill.isOnCooldown = false;
                    }
                }
            }
        }
    }

    void UIRenderSystem::Render()
    {
        if (!m_uiShader || !m_uiShader->IsValid())
        {
            static bool loggedOnce = false;
            if (!loggedOnce)
            {
                EE_CORE_ERROR("UIRenderSystem::Render() - Shader is invalid!");
                loggedOnce = true;
            }
            return;
        }

#if defined(EE_EDITOR)
        // Only render UI during play mode (like Unreal Engine's PIE - Play In Editor)
        // Editor mode should have a clean view for level design
        if (!editor::EditorGUI::isPlaying)
            return;
#endif

        // Debug: Log once when UI starts rendering
        static bool firstRender = true;
        if (firstRender)
        {
            EE_CORE_INFO("UIRenderSystem: First render call! Entities count: {}", m_Entities.size());
            firstRender = false;
        }

        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Disable depth test for UI overlay
        glDisable(GL_DEPTH_TEST);

        // Use UI shader
        m_uiShader->Bind();
        m_uiShader->SetUniformMatrix4fv("projection", m_orthoProjection);

        // Render UI for all entities with UIComponent
        for (EntityID entity : m_Entities)
        {
            const auto& ui = ECS::GetInstance().GetComponent<UIComponent>(entity);

            if (ui.showHealthbar)
                RenderHealthBar(ui);

            if (ui.showManaBar)
                RenderManaBar(ui);

            if (ui.showSkills)
                RenderSkillSlots(ui);

            if (ui.showCrosshair)
                RenderCrosshair(ui);
        }

        // Re-enable depth test
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    void UIRenderSystem::OnScreenResize(int width, int height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    bool UIRenderSystem::CastSkill(EntityID entity, int skillIndex)
    {
        if (!ECS::GetInstance().HasComponent<UIComponent>(entity))
            return false;

        auto& ui = ECS::GetInstance().GetComponent<UIComponent>(entity);

        if (skillIndex < 0 || skillIndex >= static_cast<int>(ui.skills.size()))
            return false;

        auto& skill = ui.skills[skillIndex];

        // Check if skill is on cooldown
        if (skill.isOnCooldown || skill.currentCooldown > 0.0f)
            return false;

        // Check if enough life essence (health)
        if (ui.currentHealth < skill.manaCost)
            return false;

        // Cast skill - drains life essence!
        ui.currentHealth -= skill.manaCost;
        skill.currentCooldown = skill.maxCooldown;
        skill.isOnCooldown = true;

        // Note: Mana regen timer not needed for life essence system
        // ui.manaRegenTimer = 0.0f;

        return true;
    }

    void UIRenderSystem::RenderHealthBar(const UIComponent& ui)
    {
        float x = ui.healthbarPosition.x;
        float y = ui.healthbarPosition.y;
        float width = ui.healthbarWidth;
        float height = ui.healthbarHeight;

        // Outer border (black outline for contrast)
        float outerBorder = 0.003f;
        Vec3 outerBorderColor = { 0.0f, 0.0f, 0.0f };
        RenderQuad(x - outerBorder, y - outerBorder, width + outerBorder * 2.0f, height + outerBorder * 2.0f, outerBorderColor, 0.9f);

        // Inner border (bright accent)
        float innerBorder = 0.0015f;
        Vec3 innerBorderColor = { 0.8f, 0.8f, 0.8f };
        RenderQuad(x - innerBorder, y - innerBorder, width + innerBorder * 2.0f, height + innerBorder * 2.0f, innerBorderColor, 0.8f);

        // Render background
        RenderQuad(x, y, width, height, ui.healthbarBgColor, 0.9f);

        // Render health fill with gradient effect (darker at bottom, brighter at top)
        float healthPercent = ui.currentHealth / ui.maxHealth;
        if (healthPercent > 0.0f)
        {
            float fillWidth = width * healthPercent;

            // Determine health color based on percentage
            Vec3 healthColor = ui.healthbarColor;
            if (healthPercent < 0.25f)
                healthColor = { 1.0f, 0.0f, 0.0f }; // Red when critical
            else if (healthPercent < 0.5f)
                healthColor = { 1.0f, 0.5f, 0.0f }; // Orange when low

            // Main health bar
            RenderQuad(x, y, fillWidth, height, healthColor, 1.0f);

            // Shine effect on top of health bar (brighter overlay)
            float shineHeight = height * 0.4f;
            Vec3 shineColor = { 1.0f, 1.0f, 1.0f };
            RenderQuad(x, y + height - shineHeight, fillWidth, shineHeight, shineColor, 0.3f);
        }

        // Inner shadow at the bottom for depth
        float shadowHeight = height * 0.2f;
        Vec3 shadowColor = { 0.0f, 0.0f, 0.0f };
        RenderQuad(x, y, width, shadowHeight, shadowColor, 0.3f);
    }

    void UIRenderSystem::RenderManaBar(const UIComponent& ui)
    {
        float x = ui.manaBarPosition.x;
        float y = ui.manaBarPosition.y;
        float width = ui.manaBarWidth;
        float height = ui.manaBarHeight;

        // Render background
        RenderQuad(x, y, width, height, ui.manaBarBgColor, 0.8f);

        // Render mana fill
        float manaPercent = ui.currentMana / ui.maxMana;
        if (manaPercent > 0.0f)
        {
            RenderQuad(x, y, width * manaPercent, height, ui.manaBarColor, 1.0f);
        }
    }

    void UIRenderSystem::RenderSkillSlots(const UIComponent& ui)
    {
        float startX = ui.skillsPosition.x;
        float startY = ui.skillsPosition.y;
        float slotSize = ui.skillSlotSize;
        float spacing = ui.skillSlotSpacing;
        float radius = slotSize * 0.5f; // Circle radius is half the slot size

        // Calculate total width of all slots to center them
        float totalWidth = (slotSize * ui.skills.size()) + (spacing * (ui.skills.size() - 1));
        float currentX = startX - (totalWidth * 0.5f);

        // Get current time for animation (using glfwGetTime or similar)
        static float animTime = 0.0f;
        animTime += 0.016f; // Approximate 60 FPS for smooth animation

        for (size_t i = 0; i < ui.skills.size(); ++i)
        {
            const auto& skill = ui.skills[i];

            // Calculate center of circle
            float centerX = currentX + radius;
            float centerY = startY + radius;

            // Determine slot color based on cooldown state and life essence
            Vec3 slotColor = skill.slotColor;
            bool isReady = !skill.isOnCooldown && ui.currentHealth >= skill.manaCost;

            if (skill.isOnCooldown)
                slotColor = skill.cooldownColor;
            else if (isReady)
                slotColor = skill.readyColor;

            // Add pulsing glow effect when skill is ready (outer glow)
            if (isReady)
            {
                float pulse = 0.5f + 0.5f * sinf(animTime * 3.0f + i * 0.5f); // Pulse between 0.5 and 1.0
                float glowRadius = radius + 0.004f * pulse;
                Vec3 glowColor = { 0.0f, 1.0f, 0.0f }; // Bright green glow
                RenderFilledCircle(centerX, centerY, glowRadius, glowColor, 0.3f * pulse);
            }

            // Render outer border (dark shadow for depth)
            float outerBorderRadius = radius + 0.002f;
            RenderFilledCircle(centerX, centerY, outerBorderRadius, { 0.0f, 0.0f, 0.0f }, 0.8f);

            // Render slot background circle
            RenderFilledCircle(centerX, centerY, radius, slotColor, 0.9f);

            // Render cooldown overlay (radial)
            if (skill.isOnCooldown && skill.maxCooldown > 0.0f)
            {
                float progress = skill.currentCooldown / skill.maxCooldown;
                RenderRadialCooldown(centerX, centerY, radius, progress, skill.cooldownOverlayColor, 0.7f);
            }

            // Border around slot (circle outline - thicker and brighter when ready)
            float borderThickness = isReady ? 0.003f : 0.002f;
            Vec3 borderColor = isReady ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.8f, 0.8f, 0.8f};
            RenderCircle(centerX, centerY, radius, borderThickness, borderColor);

            currentX += slotSize + spacing;
        }
    }

    void UIRenderSystem::RenderCrosshair(const UIComponent& ui)
    {
        float centerX = 0.5f;
        float centerY = 0.5f;
        float size = ui.crosshairSize;
        float thickness = ui.crosshairThickness;
        float gap = ui.crosshairGap;

        switch (ui.crosshairStyle)
        {
        case 0: // Sniper scope style crosshair
        {
            // Center dot for precision
            float dotSize = thickness * 1.5f;
            RenderQuad(centerX - dotSize * 0.5f, centerY - dotSize * 0.5f, dotSize, dotSize, ui.crosshairColor, 1.0f);

            // Inner circle
            float innerRadius = size * 0.6f;
            RenderCircle(centerX, centerY, innerRadius, thickness * 0.8f, ui.crosshairColor);

            // Outer crosshair lines extending from circle
            float outerGap = innerRadius + gap * 2.0f;
            float lineLength = size * 1.2f;

            // Horizontal lines (left and right)
            RenderQuad(centerX - lineLength - outerGap, centerY - thickness * 0.5f, lineLength, thickness, ui.crosshairColor, 0.9f);  // Left
            RenderQuad(centerX + outerGap, centerY - thickness * 0.5f, lineLength, thickness, ui.crosshairColor, 0.9f);               // Right

            // Vertical lines (top and bottom)
            RenderQuad(centerX - thickness * 0.5f, centerY + outerGap, thickness, lineLength, ui.crosshairColor, 0.9f);               // Top
            RenderQuad(centerX - thickness * 0.5f, centerY - lineLength - outerGap, thickness, lineLength, ui.crosshairColor, 0.9f);  // Bottom

            // Tick marks on the lines for range estimation
            float tickSize = thickness * 2.0f;
            float tickSpacing = size * 0.4f;

            // Left tick marks
            for (int i = 1; i <= 2; ++i)
            {
                float tickX = centerX - outerGap - (tickSpacing * i);
                RenderQuad(tickX - thickness * 0.25f, centerY - tickSize * 0.5f, thickness * 0.5f, tickSize, ui.crosshairColor, 0.7f);
            }
            // Right tick marks
            for (int i = 1; i <= 2; ++i)
            {
                float tickX = centerX + outerGap + (tickSpacing * i);
                RenderQuad(tickX - thickness * 0.25f, centerY - tickSize * 0.5f, thickness * 0.5f, tickSize, ui.crosshairColor, 0.7f);
            }
            // Top tick marks
            for (int i = 1; i <= 2; ++i)
            {
                float tickY = centerY + outerGap + (tickSpacing * i);
                RenderQuad(centerX - tickSize * 0.5f, tickY - thickness * 0.25f, tickSize, thickness * 0.5f, ui.crosshairColor, 0.7f);
            }
            // Bottom tick marks
            for (int i = 1; i <= 2; ++i)
            {
                float tickY = centerY - outerGap - (tickSpacing * i);
                RenderQuad(centerX - tickSize * 0.5f, tickY - thickness * 0.25f, tickSize, thickness * 0.5f, ui.crosshairColor, 0.7f);
            }

            break;
        }
        case 1: // Precise center dot
        {
            float dotSize = thickness * 2.0f;
            RenderQuad(centerX - dotSize * 0.5f, centerY - dotSize * 0.5f, dotSize, dotSize, ui.crosshairColor, 1.0f);
            break;
        }
        case 2: // Circle outline
        {
            RenderCircle(centerX, centerY, size, thickness, ui.crosshairColor);
            break;
        }
        default:
            break;
        }
    }

    void UIRenderSystem::RenderQuad(float posX, float posY, float width, float height, const Vec3& color, float alpha)
    {
        // Define quad vertices (2 triangles)
        float vertices[] = {
            // Position (x, y)    // Color (r, g, b, a)
            posX,         posY,          color.x, color.y, color.z, alpha,  // Bottom-left
            posX + width, posY,          color.x, color.y, color.z, alpha,  // Bottom-right
            posX + width, posY + height, color.x, color.y, color.z, alpha,  // Top-right

            posX,         posY,          color.x, color.y, color.z, alpha,  // Bottom-left
            posX + width, posY + height, color.x, color.y, color.z, alpha,  // Top-right
            posX,         posY + height, color.x, color.y, color.z, alpha   // Top-left
        };

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void UIRenderSystem::RenderRadialCooldown(float centerX, float centerY, float radius, float progress, const Vec3& color, float alpha)
    {
        if (progress <= 0.0f)
            return;

        const int segments = 32;
        m_vertexData.clear();
        m_vertexData.reserve((segments + 2) * 6); // Center + perimeter vertices

        // Center vertex
        m_vertexData.push_back(centerX);
        m_vertexData.push_back(centerY);
        m_vertexData.push_back(color.x);
        m_vertexData.push_back(color.y);
        m_vertexData.push_back(color.z);
        m_vertexData.push_back(alpha);

        // Calculate how many segments to draw based on progress
        int segmentsToDraw = static_cast<int>(progress * segments);
        float angleStep = (2.0f * static_cast<float>(M_PI)) / segments;
        float startAngle = static_cast<float>(M_PI) * 0.5f; // Start from top (90 degrees)

        for (int i = 0; i <= segmentsToDraw; ++i)
        {
            float angle = startAngle - (i * angleStep); // Clockwise from top
            float x = centerX + radius * cosf(angle);
            float y = centerY + radius * sinf(angle);

            m_vertexData.push_back(x);
            m_vertexData.push_back(y);
            m_vertexData.push_back(color.x);
            m_vertexData.push_back(color.y);
            m_vertexData.push_back(color.z);
            m_vertexData.push_back(alpha);
        }

        // Render as triangle fan
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexData.size() * sizeof(float), m_vertexData.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(m_vertexData.size() / 6));
        glBindVertexArray(0);
    }

    void UIRenderSystem::RenderCircle(float centerX, float centerY, float radius, float thickness, const Vec3& color)
    {
        const int segments = 64;
        float angleStep = (2.0f * static_cast<float>(M_PI)) / segments;

        m_vertexData.clear();
        m_vertexData.reserve(segments * 2 * 6);

        for (int i = 0; i <= segments; ++i)
        {
            float angle = i * angleStep;
            float x = centerX + radius * cosf(angle);
            float y = centerY + radius * sinf(angle);

            m_vertexData.push_back(x);
            m_vertexData.push_back(y);
            m_vertexData.push_back(color.x);
            m_vertexData.push_back(color.y);
            m_vertexData.push_back(color.z);
            m_vertexData.push_back(1.0f);
        }

        glLineWidth(thickness * static_cast<float>(m_screenHeight));
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexData.size() * sizeof(float), m_vertexData.data());
        glDrawArrays(GL_LINE_LOOP, 0, segments + 1);
        glLineWidth(1.0f);
        glBindVertexArray(0);
    }

    void UIRenderSystem::RenderFilledCircle(float centerX, float centerY, float radius, const Vec3& color, float alpha)
    {
        const int segments = 64;
        m_vertexData.clear();
        m_vertexData.reserve((segments + 2) * 6); // Center + perimeter vertices

        // Center vertex
        m_vertexData.push_back(centerX);
        m_vertexData.push_back(centerY);
        m_vertexData.push_back(color.x);
        m_vertexData.push_back(color.y);
        m_vertexData.push_back(color.z);
        m_vertexData.push_back(alpha);

        // Perimeter vertices
        float angleStep = (2.0f * static_cast<float>(M_PI)) / segments;
        for (int i = 0; i <= segments; ++i)
        {
            float angle = i * angleStep;
            float x = centerX + radius * cosf(angle);
            float y = centerY + radius * sinf(angle);

            m_vertexData.push_back(x);
            m_vertexData.push_back(y);
            m_vertexData.push_back(color.x);
            m_vertexData.push_back(color.y);
            m_vertexData.push_back(color.z);
            m_vertexData.push_back(alpha);
        }

        // Render as triangle fan
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexData.size() * sizeof(float), m_vertexData.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(m_vertexData.size() / 6));
        glBindVertexArray(0);
    }

} // namespace Ermine
