/* Start Header ************************************************************************/
/*!
\file       UIRenderSystem.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
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
#include "Texture.h"
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
        m_aspectRatio = (screenHeight > 0) ? static_cast<float>(screenWidth) / static_cast<float>(screenHeight) : 1.0f;

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

        // Reserve space for vertex data (position + color + texCoord)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2048, nullptr, GL_DYNAMIC_DRAW);

        // Position attribute (x, y)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        // Color attribute (r, g, b, a)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));

        // Texture coordinate attribute (u, v)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        glBindVertexArray(0);

        // Initialize text renderer for keybind labels
        m_textRenderer = std::make_shared<UITextRenderer>();
        // Try to load a TTF font, fallback to simple bitmap if not found
        if (!m_textRenderer->Initialize("../Resources/Fonts/times.ttf", 32))
        {
            EE_CORE_WARN("UITextRenderer: Failed to load font, using fallback");
        }

        EE_CORE_INFO("UIRenderSystem initialized successfully");
    }

    void UIRenderSystem::Update(float deltaTime)
    {
        // Iterate through all entities with UIComponent
        for (EntityID entity : m_Entities)
        {
            auto& ui = ECS::GetInstance().GetComponent<UIComponent>(entity);

            // Update skill cooldowns and activation animations
            bool anySkillOnCooldown = false;
            for (auto& skill : ui.skills)
            {
                // Update cooldown
                if (skill.isOnCooldown && skill.currentCooldown > 0.0f)
                {
                    skill.currentCooldown -= deltaTime;
                    if (skill.currentCooldown <= 0.0f)
                    {
                        skill.currentCooldown = 0.0f;
                        skill.isOnCooldown = false;
                    }
                    else
                    {
                        anySkillOnCooldown = true;
                    }
                }

                // Update activation flash timer
                if (skill.activationFlashTimer > 0.0f)
                {
                    skill.activationFlashTimer -= deltaTime;
                    if (skill.activationFlashTimer < 0.0f)
                        skill.activationFlashTimer = 0.0f;
                }
            }

            // Life Essence (Health) Regeneration System
            // Only regenerate when no skills are on cooldown (player is idle)
            if (!anySkillOnCooldown && ui.currentHealth < ui.maxHealth)
            {
                ui.healthRegenTimer += deltaTime;

                // Only regenerate health after the delay
                if (ui.healthRegenTimer >= ui.healthRegenDelay)
                {
                    ui.currentHealth += ui.healthRegenRate * deltaTime;
                    if (ui.currentHealth > ui.maxHealth)
                        ui.currentHealth = ui.maxHealth;
                }
            }
            else
            {
                // Reset timer when skills are being used
                ui.healthRegenTimer = 0.0f;
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
        m_uiShader->SetUniform1i("uUseTexture", 0); // Default: don't use textures

        // Render UI for all entities with UIComponent
        for (EntityID entity : m_Entities)
        {
            const auto& ui = ECS::GetInstance().GetComponent<UIComponent>(entity);

            if (ui.showHealthbar)
                RenderHealthBar(ui);

            if (ui.showBookCounter)
                RenderBookCounter(ui);

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
        m_aspectRatio = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
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

        // Trigger activation flash effect (0.2 seconds)
        skill.activationFlashTimer = 0.2f;

        // Reset health regeneration timer when skill is cast
        ui.healthRegenTimer = 0.0f;

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

            // Determine health color based on percentage - STEAMPUNK THEME
            Vec3 healthColor = ui.healthbarColor;
            if (healthPercent < 0.25f)
                healthColor = { 0.75f, 0.20f, 0.10f }; // Dark rusty copper when critical
            else if (healthPercent < 0.5f)
                healthColor = { 0.85f, 0.45f, 0.15f }; // Dimmer brass/copper when low

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

    void UIRenderSystem::RenderBookCounter(const UIComponent& ui)
    {
        if (!m_textRenderer || !m_uiShader)
            return;

        // Create counter text (e.g., "0/4", "2/4")
        std::string counterText = std::to_string(ui.booksCollected) + "/" + std::to_string(ui.totalBooks);

        // Position at top-left
        float x = ui.bookCounterPosition.x;
        float y = ui.bookCounterPosition.y;

        // Text styling - professional and readable
        float textScale = 1.0f;  // Larger text for visibility
        Vec3 textColor = { 0.95f, 0.85f, 0.55f };  // Bright brass/gold color

        // Render the counter text
        m_textRenderer->RenderText(m_uiShader, counterText, x, y, textScale, textColor, 1.0f);
    }

    void UIRenderSystem::RenderSkillSlots(const UIComponent& ui)
    {
        float startX = ui.skillsPosition.x;
        float startY = ui.skillsPosition.y;
        float slotSize = ui.skillSlotSize;
        float spacing = ui.skillSlotSpacing;
        float radius = slotSize * 0.5f;

        // Only render 3 visual slots (indices 0, 2, 3)
        // Skills[0] = LMB (Shoot Orb / Teleport) - combined visual
        // Skills[1] = (skipped, part of LMB sequence)
        // Skills[2] = RMB (Blind Burst)
        // Skills[3] = R (Recall Orb)
        const int visualSlotIndices[] = {0, 2, 3};
        const int numVisualSlots = 3;

        // Calculate total width of visual slots to center them
        float totalWidth = (slotSize * numVisualSlots) + (spacing * (numVisualSlots - 1));
        float currentX = startX - (totalWidth * 0.5f);

        for (int slotIdx = 0; slotIdx < numVisualSlots; ++slotIdx)
        {
            size_t i = visualSlotIndices[slotIdx];
            const auto& skill = ui.skills[i];

            // Calculate center position
            float centerX = currentX + radius;
            float centerY = startY + radius;

            // Check if skill has a texture icon
            std::shared_ptr<graphics::Texture> skillTexture = nullptr;
            if (!skill.iconTexturePath.empty())
            {
                // Check if texture is already cached
                auto it = m_textureCache.find(skill.iconTexturePath);
                if (it != m_textureCache.end())
                {
                    skillTexture = it->second;
                }
                else
                {
                    // Load texture via AssetManager
                    skillTexture = AssetManager::GetInstance().LoadTexture(skill.iconTexturePath);
                    if (skillTexture && skillTexture->IsValid())
                    {
                        m_textureCache[skill.iconTexturePath] = skillTexture; // Cache it
                    }
                }
            }

            // ========================================================================
            // RENDER SKILL ICON (Square with correct aspect ratio)
            // ========================================================================
            if (skillTexture && skillTexture->IsValid())
            {
                // Render clean icon texture with full brightness
                Vec3 tintColor = { 1.0f, 1.0f, 1.0f }; // No tinting - show texture as-is
                float alpha = 1.0f;

                // Slightly dim when on cooldown
                if (skill.isOnCooldown)
                {
                    tintColor = { 0.5f, 0.5f, 0.5f }; // Darken when on cooldown
                    alpha = 0.6f;
                }
                // Slightly dim when insufficient health
                else if (ui.currentHealth < skill.manaCost)
                {
                    tintColor = { 0.7f, 0.7f, 0.7f };
                    alpha = 0.7f;
                }

                // Use square rendering to maintain aspect ratio (size = diameter of old circle)
                RenderTexturedSquare(centerX, centerY, slotSize, skillTexture, tintColor, alpha);
            }
            else
            {
                // Fallback: render simple square if no texture
                Vec3 fallbackColor = { 0.3f, 0.3f, 0.3f };
                float halfSize = slotSize * 0.5f;
                float adjustedHalfWidth = halfSize / m_aspectRatio;
                RenderQuad(centerX - adjustedHalfWidth, centerY - halfSize,
                          adjustedHalfWidth * 2.0f, slotSize, fallbackColor, 0.5f);
            }

            // ========================================================================
            // RENDER COOLDOWN OVERLAY (Radial pie chart over icon)
            // ========================================================================
            if (skill.isOnCooldown && skill.maxCooldown > 0.0f)
            {
                float progress = skill.currentCooldown / skill.maxCooldown;
                Vec3 cooldownColor = { 0.0f, 0.0f, 0.0f }; // Black overlay
                // Use radius for cooldown overlay (centered on square icon)
                RenderRadialCooldown(centerX, centerY, slotSize * 0.5f, progress, cooldownColor, 0.7f);
            }

            // ========================================================================
            // ACTIVATION FLASH EFFECT (Flash when skill is activated)
            // ========================================================================
            if (skill.activationFlashTimer > 0.0f)
            {
                // Calculate flash intensity (fades from 1.0 to 0.0 over 0.2 seconds)
                float flashIntensity = skill.activationFlashTimer / 0.2f;

                // Bright white/yellow flash
                float glowSize = slotSize + 0.02f; // Slightly larger than icon
                Vec3 flashColor = { 1.0f, 1.0f, 0.8f }; // Bright white-yellow

                // Render flash as a square border
                float halfSize = glowSize * 0.5f;
                float adjustedHalfWidth = halfSize / m_aspectRatio;
                RenderQuad(centerX - adjustedHalfWidth, centerY - halfSize,
                          adjustedHalfWidth * 2.0f, glowSize, flashColor, flashIntensity * 0.8f);
            }

            // ========================================================================
            // RENDER KEYBIND LABEL BELOW SKILL SLOT
            // ========================================================================
            if (m_textRenderer && m_uiShader && !skill.keyBinding.empty())
            {
                // Calculate label position (centered below the skill slot)
                float textScale = 0.6f; // Slightly larger text for better readability
                float textWidth = m_textRenderer->GetTextWidth(skill.keyBinding, textScale);
                float labelX = centerX - (textWidth * 0.5f); // Center horizontally
                float labelY = centerY - (slotSize * 0.5f) - 0.02f; // Position below the square slot

                // Check if skill is ready to use
                bool isReady = !skill.isOnCooldown && ui.currentHealth >= skill.manaCost;

                // Professional white text with slight transparency
                Vec3 labelColor = { 1.0f, 1.0f, 1.0f };
                float labelAlpha = isReady ? 1.0f : 0.6f;

                // Render the keybind text (e.g., "LMB", "RMB", "R")
                m_textRenderer->RenderText(m_uiShader, skill.keyBinding, labelX, labelY, textScale, labelColor, labelAlpha);
            }

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
        case 3: // Steampunk ornate crosshair
        {
            // Center dot
            float dotSize = thickness * 1.5f;
            RenderQuad(centerX - dotSize * 0.5f, centerY - dotSize * 0.5f, dotSize, dotSize, ui.crosshairColor, 1.0f);

            // Inner circle (brass ring)
            float innerRadius = size * 0.4f;
            RenderCircle(centerX, centerY, innerRadius, thickness * 1.2f, ui.crosshairColor);

            // Outer decorative ring
            float outerRadius = size * 0.8f;
            RenderCircle(centerX, centerY, outerRadius, thickness * 0.6f, ui.crosshairColor);

            // Ornate corner brackets (4 corners)
            float bracketLength = size * 0.6f;
            float bracketThickness = thickness * 1.5f;
            float cornerOffset = outerRadius + gap;

            // Top-right bracket
            RenderQuad(centerX + cornerOffset, centerY + cornerOffset, bracketLength, bracketThickness, ui.crosshairColor, 0.8f);
            RenderQuad(centerX + cornerOffset, centerY + cornerOffset, bracketThickness, bracketLength, ui.crosshairColor, 0.8f);

            // Top-left bracket
            RenderQuad(centerX - cornerOffset - bracketLength, centerY + cornerOffset, bracketLength, bracketThickness, ui.crosshairColor, 0.8f);
            RenderQuad(centerX - cornerOffset, centerY + cornerOffset, bracketThickness, bracketLength, ui.crosshairColor, 0.8f);

            // Bottom-right bracket
            RenderQuad(centerX + cornerOffset, centerY - cornerOffset, bracketLength, bracketThickness, ui.crosshairColor, 0.8f);
            RenderQuad(centerX + cornerOffset, centerY - cornerOffset - bracketLength, bracketThickness, bracketLength, ui.crosshairColor, 0.8f);

            // Bottom-left bracket
            RenderQuad(centerX - cornerOffset - bracketLength, centerY - cornerOffset, bracketLength, bracketThickness, ui.crosshairColor, 0.8f);
            RenderQuad(centerX - cornerOffset, centerY - cornerOffset - bracketLength, bracketThickness, bracketLength, ui.crosshairColor, 0.8f);

            // Small decorative gears (circles at cardinal points)
            float gearRadius = thickness * 2.0f;
            float gearDistance = outerRadius + gap + size * 0.2f;
            Vec3 dimColor = { ui.crosshairColor.x * 0.7f, ui.crosshairColor.y * 0.7f, ui.crosshairColor.z * 0.7f };

            RenderFilledCircle(centerX + gearDistance, centerY, gearRadius, dimColor, 0.6f);
            RenderFilledCircle(centerX - gearDistance, centerY, gearRadius, dimColor, 0.6f);
            RenderFilledCircle(centerX, centerY + gearDistance, gearRadius, dimColor, 0.6f);
            RenderFilledCircle(centerX, centerY - gearDistance, gearRadius, dimColor, 0.6f);

            break;
        }
        default:
            break;
        }
    }

    void UIRenderSystem::RenderQuad(float posX, float posY, float width, float height, const Vec3& color, float alpha)
    {
        // Define quad vertices (2 triangles) with dummy texture coordinates
        float vertices[] = {
            // Position (x, y)    // Color (r, g, b, a)        // TexCoord (u, v)
            posX,         posY,          color.x, color.y, color.z, alpha,  0.0f, 0.0f,  // Bottom-left
            posX + width, posY,          color.x, color.y, color.z, alpha,  0.0f, 0.0f,  // Bottom-right
            posX + width, posY + height, color.x, color.y, color.z, alpha,  0.0f, 0.0f,  // Top-right

            posX,         posY,          color.x, color.y, color.z, alpha,  0.0f, 0.0f,  // Bottom-left
            posX + width, posY + height, color.x, color.y, color.z, alpha,  0.0f, 0.0f,  // Top-right
            posX,         posY + height, color.x, color.y, color.z, alpha,  0.0f, 0.0f   // Top-left
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
        m_vertexData.reserve((segments + 2) * 8); // Center + perimeter vertices (8 floats per vertex)

        // Center vertex
        m_vertexData.push_back(centerX);
        m_vertexData.push_back(centerY);
        m_vertexData.push_back(color.x);
        m_vertexData.push_back(color.y);
        m_vertexData.push_back(color.z);
        m_vertexData.push_back(alpha);
        m_vertexData.push_back(0.0f); // texCoord u
        m_vertexData.push_back(0.0f); // texCoord v

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
            m_vertexData.push_back(0.0f); // texCoord u
            m_vertexData.push_back(0.0f); // texCoord v
        }

        // Render as triangle fan
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexData.size() * sizeof(float), m_vertexData.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(m_vertexData.size() / 8));
        glBindVertexArray(0);
    }

    void UIRenderSystem::RenderCircle(float centerX, float centerY, float radius, float thickness, const Vec3& color)
    {
        const int segments = 64;
        float angleStep = (2.0f * static_cast<float>(M_PI)) / segments;

        m_vertexData.clear();
        m_vertexData.reserve(segments * 2 * 8); // 8 floats per vertex

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
            m_vertexData.push_back(0.0f); // texCoord u
            m_vertexData.push_back(0.0f); // texCoord v
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
        m_vertexData.reserve((segments + 2) * 8); // Center + perimeter vertices (8 floats per vertex)

        // Center vertex
        m_vertexData.push_back(centerX);
        m_vertexData.push_back(centerY);
        m_vertexData.push_back(color.x);
        m_vertexData.push_back(color.y);
        m_vertexData.push_back(color.z);
        m_vertexData.push_back(alpha);
        m_vertexData.push_back(0.5f); // texCoord u (center of texture)
        m_vertexData.push_back(0.5f); // texCoord v (center of texture)

        // Perimeter vertices
        float angleStep = (2.0f * static_cast<float>(M_PI)) / segments;
        for (int i = 0; i <= segments; ++i)
        {
            float angle = i * angleStep;
            float x = centerX + radius * cosf(angle);
            float y = centerY + radius * sinf(angle);

            // Calculate texture coordinates (circular mapping)
            float u = 0.5f + 0.5f * cosf(angle);
            float v = 0.5f + 0.5f * sinf(angle);

            m_vertexData.push_back(x);
            m_vertexData.push_back(y);
            m_vertexData.push_back(color.x);
            m_vertexData.push_back(color.y);
            m_vertexData.push_back(color.z);
            m_vertexData.push_back(alpha);
            m_vertexData.push_back(u);
            m_vertexData.push_back(v);
        }

        // Render as triangle fan
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexData.size() * sizeof(float), m_vertexData.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(m_vertexData.size() / 8));
        glBindVertexArray(0);
    }

    void UIRenderSystem::RenderTexturedCircle(float centerX, float centerY, float radius,
                                              std::shared_ptr<graphics::Texture> texture,
                                              const Vec3& color, float alpha)
    {
        if (!texture || !texture->IsValid())
            return;

        const int segments = 64;
        m_vertexData.clear();
        m_vertexData.reserve((segments + 2) * 8); // Center + perimeter vertices (8 floats per vertex)

        // Center vertex
        m_vertexData.push_back(centerX);
        m_vertexData.push_back(centerY);
        m_vertexData.push_back(color.x);
        m_vertexData.push_back(color.y);
        m_vertexData.push_back(color.z);
        m_vertexData.push_back(alpha);
        m_vertexData.push_back(0.5f); // texCoord u (center of texture)
        m_vertexData.push_back(0.5f); // texCoord v (center of texture)

        // Perimeter vertices
        float angleStep = (2.0f * static_cast<float>(M_PI)) / segments;
        for (int i = 0; i <= segments; ++i)
        {
            float angle = i * angleStep;
            float x = centerX + radius * cosf(angle);
            float y = centerY + radius * sinf(angle);

            // Calculate texture coordinates (circular mapping)
            float u = 0.5f + 0.5f * cosf(angle);
            float v = 0.5f + 0.5f * sinf(angle);

            m_vertexData.push_back(x);
            m_vertexData.push_back(y);
            m_vertexData.push_back(color.x);
            m_vertexData.push_back(color.y);
            m_vertexData.push_back(color.z);
            m_vertexData.push_back(alpha);
            m_vertexData.push_back(u);
            m_vertexData.push_back(v);
        }

        // Enable texture mode in shader
        m_uiShader->SetUniform1i("uUseTexture", 1);
        texture->Bind(0); // Bind to texture unit 0
        m_uiShader->SetUniform1i("uTexture", 0);

        // Render as triangle fan
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexData.size() * sizeof(float), m_vertexData.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(m_vertexData.size() / 8));
        glBindVertexArray(0);

        // Disable texture mode
        texture->Unbind();
        m_uiShader->SetUniform1i("uUseTexture", 0);
    }

    void UIRenderSystem::RenderTexturedSquare(float centerX, float centerY, float size,
                                              std::shared_ptr<graphics::Texture> texture,
                                              const Vec3& color, float alpha)
    {
        if (!texture || !texture->IsValid())
            return;

        // Get texture dimensions to calculate its aspect ratio
        int texWidth = texture->GetWidth();
        int texHeight = texture->GetHeight();

        // Calculate texture aspect ratio (width / height)
        float textureAspectRatio = (texHeight > 0) ? static_cast<float>(texWidth) / static_cast<float>(texHeight) : 1.0f;

        // Calculate base dimensions accounting for screen aspect ratio
        float halfSize = size * 0.5f;

        // Adjust dimensions to maintain BOTH screen aspect ratio and texture aspect ratio
        // This prevents stretching of non-square textures
        float adjustedHalfWidth = (halfSize * textureAspectRatio) / m_aspectRatio;
        float adjustedHalfHeight = halfSize;

        // Calculate corner positions
        float left = centerX - adjustedHalfWidth;
        float right = centerX + adjustedHalfWidth;
        float bottom = centerY - adjustedHalfHeight;
        float top = centerY + adjustedHalfHeight;

        // Define quad vertices (2 triangles) with texture coordinates
        float vertices[] = {
            // Position (x, y)    // Color (r, g, b, a)                  // TexCoord (u, v)
            left,  bottom,        color.x, color.y, color.z, alpha,     0.0f, 0.0f,  // Bottom-left
            right, bottom,        color.x, color.y, color.z, alpha,     1.0f, 0.0f,  // Bottom-right
            right, top,           color.x, color.y, color.z, alpha,     1.0f, 1.0f,  // Top-right

            left,  bottom,        color.x, color.y, color.z, alpha,     0.0f, 0.0f,  // Bottom-left
            right, top,           color.x, color.y, color.z, alpha,     1.0f, 1.0f,  // Top-right
            left,  top,           color.x, color.y, color.z, alpha,     0.0f, 1.0f   // Top-left
        };

        // Enable texture mode in shader
        m_uiShader->SetUniform1i("uUseTexture", 1);
        texture->Bind(0); // Bind to texture unit 0
        m_uiShader->SetUniform1i("uTexture", 0);

        // Render the square
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Disable texture mode
        texture->Unbind();
        m_uiShader->SetUniform1i("uUseTexture", 0);
    }

} // namespace Ermine
