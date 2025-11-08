/* Start Header ************************************************************************/
/*!
\file       UIRenderSystem.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       04/11/2025
\brief      This file contains declarations for the UIRenderSystem which renders
            HUD elements including health bars, mana bars, skill slots, and crosshairs.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "Systems.h"
#include "Shader.h"
#include "Texture.h"
#include "Components.h"
#include "UITextRenderer.h"
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ermine
{
    /*!***********************************************************************
    \brief
        UI Rendering System for HUD elements (health bars, mana bars,
        skill slots, crosshairs). Renders in screen-space after 3D scene.
    *************************************************************************/
    class UIRenderSystem : public System
    {
    public:
        /*!***********************************************************************
        \brief
            Initializes the UI render system with screen dimensions.
        \param[in] screenWidth
            Width of the screen in pixels.
        \param[in] screenHeight
            Height of the screen in pixels.
        *************************************************************************/
        void Init(int screenWidth, int screenHeight);

        /*!***********************************************************************
        \brief
            Updates mana regeneration and skill cooldowns for all entities with UIComponent.
        \param[in] deltaTime
            Time elapsed since last frame in seconds.
        *************************************************************************/
        void Update(float deltaTime);

        /*!***********************************************************************
        \brief
            Renders all UI elements for entities with UIComponent.
            Should be called after 3D rendering but before ImGui.
        *************************************************************************/
        void Render();

        /*!***********************************************************************
        \brief
            Updates screen dimensions and recalculates orthographic projection.
        \param[in] width
            New screen width in pixels.
        \param[in] height
            New screen height in pixels.
        *************************************************************************/
        void OnScreenResize(int width, int height);

        /*!***********************************************************************
        \brief
            Helper function to cast a skill, handling mana cost and cooldown.
        \param[in] entity
            Entity ID with UIComponent.
        \param[in] skillIndex
            Index of the skill slot (0-3).
        \return
            True if skill was successfully cast, false if on cooldown or insufficient mana.
        *************************************************************************/
        bool CastSkill(EntityID entity, int skillIndex);

    private:
        /*!***********************************************************************
        \brief
            Renders a health bar for the given UI component.
        \param[in] ui
            Reference to UIComponent with health bar settings.
        *************************************************************************/
        void RenderHealthBar(const UIComponent& ui);

        /*!***********************************************************************
        \brief
            Renders a mana bar for the given UI component.
        \param[in] ui
            Reference to UIComponent with mana bar settings.
        *************************************************************************/
        void RenderManaBar(const UIComponent& ui);

        /*!***********************************************************************
        \brief
            Renders skill slots with cooldown overlays for the given UI component.
        \param[in] ui
            Reference to UIComponent with skill slot settings.
        *************************************************************************/
        void RenderSkillSlots(const UIComponent& ui);

        /*!***********************************************************************
        \brief
            Renders a crosshair at the center of the screen.
        \param[in] ui
            Reference to UIComponent with crosshair settings.
        *************************************************************************/
        void RenderCrosshair(const UIComponent& ui);

        /*!***********************************************************************
        \brief
            Renders the book counter (e.g., "0/4", "1/4", etc.) at top-left.
        \param[in] ui
            Reference to UIComponent with book counter settings.
        *************************************************************************/
        void RenderBookCounter(const UIComponent& ui);

        /*!***********************************************************************
        \brief
            Renders a filled quad at the specified position with color.
        \param[in] posX
            X position in normalized screen coordinates (0-1).
        \param[in] posY
            Y position in normalized screen coordinates (0-1).
        \param[in] width
            Width in normalized screen coordinates (0-1).
        \param[in] height
            Height in normalized screen coordinates (0-1).
        \param[in] color
            RGB color values (0-1).
        \param[in] alpha
            Alpha transparency (0-1, default 1.0).
        *************************************************************************/
        void RenderQuad(float posX, float posY, float width, float height, const Vec3& color, float alpha = 1.0f);

        /*!***********************************************************************
        \brief
            Renders a radial cooldown overlay (pie chart style).
        \param[in] centerX
            Center X position in normalized screen coordinates (0-1).
        \param[in] centerY
            Center Y position in normalized screen coordinates (0-1).
        \param[in] radius
            Radius in normalized screen coordinates (0-1).
        \param[in] progress
            Cooldown progress from 0 (complete) to 1 (just started).
        \param[in] color
            RGB color values (0-1).
        \param[in] alpha
            Alpha transparency (0-1, default 0.7).
        *************************************************************************/
        void RenderRadialCooldown(float centerX, float centerY, float radius, float progress, const Vec3& color, float alpha = 0.7f);

        /*!***********************************************************************
        \brief
            Renders a circle outline (for crosshair and borders).
        \param[in] centerX
            Center X position in normalized screen coordinates (0-1).
        \param[in] centerY
            Center Y position in normalized screen coordinates (0-1).
        \param[in] radius
            Radius in normalized screen coordinates (0-1).
        \param[in] thickness
            Line thickness in normalized screen coordinates.
        \param[in] color
            RGB color values (0-1).
        *************************************************************************/
        void RenderCircle(float centerX, float centerY, float radius, float thickness, const Vec3& color);

        /*!***********************************************************************
        \brief
            Renders a filled circle (for skill slots and UI elements).
        \param[in] centerX
            Center X position in normalized screen coordinates (0-1).
        \param[in] centerY
            Center Y position in normalized screen coordinates (0-1).
        \param[in] radius
            Radius in normalized screen coordinates (0-1).
        \param[in] color
            RGB color values (0-1).
        \param[in] alpha
            Alpha transparency (0-1, default 1.0).
        *************************************************************************/
        void RenderFilledCircle(float centerX, float centerY, float radius, const Vec3& color, float alpha = 1.0f);

        /*!***********************************************************************
        \brief
            Renders a textured filled circle (for skill icons).
        \param[in] centerX
            Center X position in normalized screen coordinates (0-1).
        \param[in] centerY
            Center Y position in normalized screen coordinates (0-1).
        \param[in] radius
            Radius in normalized screen coordinates (0-1).
        \param[in] texture
            Shared pointer to texture to render.
        \param[in] color
            Tint color RGB values (0-1), default white.
        \param[in] alpha
            Alpha transparency (0-1, default 1.0).
        *************************************************************************/
        void RenderTexturedCircle(float centerX, float centerY, float radius,
                                  std::shared_ptr<graphics::Texture> texture,
                                  const Vec3& color = {1.0f, 1.0f, 1.0f},
                                  float alpha = 1.0f);

        // OpenGL resources
        std::shared_ptr<graphics::Shader> m_uiShader;
        GLuint m_VAO;
        GLuint m_VBO;

        // Screen dimensions
        int m_screenWidth;
        int m_screenHeight;

        // Orthographic projection matrix for screen-space rendering
        glm::mat4 m_orthoProjection;

        // Vertex data for dynamic rendering
        std::vector<float> m_vertexData;

        // Texture cache for skill icons (path -> texture)
        std::unordered_map<std::string, std::shared_ptr<graphics::Texture>> m_textureCache;

        // Text renderer for keybind labels
        std::shared_ptr<UITextRenderer> m_textRenderer;
    };

} // namespace Ermine
