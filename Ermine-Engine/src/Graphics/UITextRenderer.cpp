/* Start Header ************************************************************************/
/*!
\file       UITextRenderer.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       11/07/2025
\brief      Implementation of simple text rendering system for UI using stb_truetype.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "UITextRenderer.h"
#include "AssetManager.h"
#include "Logger.h"
#include <fstream>

// stb_truetype implementation
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../../ThirdParty/recastnavigation/RecastDemo/Contrib/stb_truetype.h"

namespace Ermine
{
    bool UITextRenderer::Initialize(const std::string& fontPath, int fontSize)
    {
        m_fontSize = fontSize;
        m_lineHeight = static_cast<float>(fontSize) / 720.0f; // Normalize to screen space

        if (!fontPath.empty())
        {
            if (GenerateFontAtlas(fontPath, fontSize))
            {
                EE_CORE_INFO("UITextRenderer initialized with font: {}", fontPath);
                return true;
            }
            EE_CORE_WARN("Failed to load font '{}', using fallback", fontPath);
        }

        // Use fallback font if no path provided or loading failed
        CreateFallbackFont();
        return true;
    }

    bool UITextRenderer::GenerateFontAtlas(const std::string& fontPath, int fontSize)
    {
        // Read font file
        std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            EE_CORE_ERROR("Failed to open font file: {}", fontPath);
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> fontBuffer(size);
        if (!file.read(reinterpret_cast<char*>(fontBuffer.data()), size))
        {
            EE_CORE_ERROR("Failed to read font file: {}", fontPath);
            return false;
        }

        // Initialize stb_truetype
        stbtt_fontinfo fontInfo;
        if (!stbtt_InitFont(&fontInfo, fontBuffer.data(), 0))
        {
            EE_CORE_ERROR("Failed to initialize font: {}", fontPath);
            return false;
        }

        // Atlas size (power of 2 for GPU)
        m_atlasWidth = 512;
        m_atlasHeight = 512;

        // Allocate bitmap for atlas
        std::vector<unsigned char> atlasData(m_atlasWidth * m_atlasHeight);

        // Bake font to atlas (ASCII printable characters: 32-126)
        const int firstChar = 32;
        const int numChars = 95; // 32 to 126 inclusive
        std::vector<stbtt_bakedchar> charData(numChars);

        int result = stbtt_BakeFontBitmap(
            fontBuffer.data(), 0,
            static_cast<float>(fontSize),
            atlasData.data(), m_atlasWidth, m_atlasHeight,
            firstChar, numChars,
            charData.data()
        );

        if (result <= 0)
        {
            EE_CORE_ERROR("stbtt_BakeFontBitmap failed");
            return false;
        }

        // Convert grayscale to RGBA for OpenGL
        std::vector<unsigned char> rgbaData(m_atlasWidth * m_atlasHeight * 4);
        for (int i = 0; i < m_atlasWidth * m_atlasHeight; ++i)
        {
            rgbaData[i * 4 + 0] = 255;              // R
            rgbaData[i * 4 + 1] = 255;              // G
            rgbaData[i * 4 + 2] = 255;              // B
            rgbaData[i * 4 + 3] = atlasData[i];     // A (font alpha)
        }

        // Create OpenGL texture
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_atlasWidth, m_atlasHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, rgbaData.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Store texture ID for later use
        // Note: We manage the OpenGL texture manually here since we're creating it from scratch
        // The Texture class expects to load from files, so we'll store just the ID
        m_fontTextureID = textureID;

        // Store glyph data
        m_glyphs.clear();
        float invWidth = 1.0f / m_atlasWidth;
        float invHeight = 1.0f / m_atlasHeight;

        for (int i = 0; i < numChars; ++i)
        {
            char c = static_cast<char>(firstChar + i);
            const auto& baked = charData[i];

            CharGlyph glyph;
            glyph.u0 = baked.x0 * invWidth;
            glyph.v0 = baked.y0 * invHeight;
            glyph.u1 = baked.x1 * invWidth;
            glyph.v1 = baked.y1 * invHeight;

            // Normalize to screen space (assuming 1280x720 reference)
            glyph.width = (baked.x1 - baked.x0) / 1280.0f;
            glyph.height = (baked.y1 - baked.y0) / 720.0f;
            glyph.xOffset = baked.xoff / 1280.0f;
            glyph.yOffset = baked.yoff / 720.0f;
            glyph.xAdvance = baked.xadvance / 1280.0f;

            m_glyphs[c] = glyph;
        }

        EE_CORE_INFO("Font atlas generated: {}x{}, {} characters", m_atlasWidth, m_atlasHeight, numChars);
        return true;
    }

    void UITextRenderer::CreateFallbackFont()
    {
        // Simple 8x8 bitmap font atlas (8x12 grid for 96 characters)
        m_atlasWidth = 128;
        m_atlasHeight = 128;

        std::vector<unsigned char> atlasData(m_atlasWidth * m_atlasHeight * 4, 0);

        // Fill with white checkerboard pattern as fallback
        for (int y = 0; y < m_atlasHeight; ++y)
        {
            for (int x = 0; x < m_atlasWidth; ++x)
            {
                int idx = (y * m_atlasWidth + x) * 4;
                bool isWhite = ((x / 8) + (y / 8)) % 2 == 0;
                unsigned char val = isWhite ? 255 : 0;
                atlasData[idx + 0] = 255;   // R
                atlasData[idx + 1] = 255;   // G
                atlasData[idx + 2] = 255;   // B
                atlasData[idx + 3] = val;   // A
            }
        }

        // Create texture
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_atlasWidth, m_atlasHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, atlasData.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Store texture ID for fallback font
        m_fontTextureID = textureID;

        // Create simple glyph mapping (8x8 characters)
        for (char c = 32; c < 127; ++c)
        {
            int idx = c - 32;
            int col = idx % 16;
            int row = idx / 16;

            CharGlyph glyph;
            glyph.u0 = (col * 8) / static_cast<float>(m_atlasWidth);
            glyph.v0 = (row * 8) / static_cast<float>(m_atlasHeight);
            glyph.u1 = ((col + 1) * 8) / static_cast<float>(m_atlasWidth);
            glyph.v1 = ((row + 1) * 8) / static_cast<float>(m_atlasHeight);
            glyph.width = 8.0f / 1280.0f;
            glyph.height = 8.0f / 720.0f;
            glyph.xOffset = 0.0f;
            glyph.yOffset = 0.0f;
            glyph.xAdvance = 8.0f / 1280.0f;

            m_glyphs[c] = glyph;
        }

        EE_CORE_WARN("Using fallback font (checkerboard pattern)");
    }

    void UITextRenderer::RenderText(std::shared_ptr<graphics::Shader> shader,
        const std::string& text,
        float x, float y,
        float scale,
        const Vec3& color,
        float alpha)
    {
        if (m_fontTextureID == 0 || m_glyphs.empty())
        {
            EE_CORE_ERROR("UITextRenderer::RenderText - Font not initialized!");
            return;
        }

        if (!shader || !shader->IsValid())
        {
            EE_CORE_ERROR("UITextRenderer::RenderText - Invalid shader!");
            return;
        }

        // Enable texture mode in shader
        shader->SetUniform1i("uUseTexture", 1);

        // Bind font texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_fontTextureID);
        shader->SetUniform1i("uTexture", 0);

        float cursorX = x;
        float cursorY = y;

        for (char c : text)
        {
            if (c == '\n')
            {
                cursorX = x;
                cursorY -= m_lineHeight * scale;
                continue;
            }

            auto it = m_glyphs.find(c);
            if (it == m_glyphs.end())
                continue; // Skip unknown characters

            const CharGlyph& glyph = it->second;

            // Calculate quad position
            float x0 = cursorX + glyph.xOffset * scale;
            float y0 = cursorY + glyph.yOffset * scale;
            float x1 = x0 + glyph.width * scale;
            float y1 = y0 + glyph.height * scale;

            // Build vertex data (2D position, RGBA color, UV)
            float vertices[] = {
                // Position       // Color                        // UV
                x0, y0,          color.x, color.y, color.z, alpha,  glyph.u0, glyph.v0,
                x1, y0,          color.x, color.y, color.z, alpha,  glyph.u1, glyph.v0,
                x1, y1,          color.x, color.y, color.z, alpha,  glyph.u1, glyph.v1,

                x0, y0,          color.x, color.y, color.z, alpha,  glyph.u0, glyph.v0,
                x1, y1,          color.x, color.y, color.z, alpha,  glyph.u1, glyph.v1,
                x0, y1,          color.x, color.y, color.z, alpha,  glyph.u0, glyph.v1
            };

            // Submit to GPU (assume VBO is bound by UIRenderSystem)
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Advance cursor
            cursorX += glyph.xAdvance * scale;
        }

        // Cleanup: unbind texture and disable texture mode
        glBindTexture(GL_TEXTURE_2D, 0);
        shader->SetUniform1i("uUseTexture", 0);
    }

    float UITextRenderer::GetTextWidth(const std::string& text, float scale) const
    {
        float width = 0.0f;
        for (char c : text)
        {
            auto it = m_glyphs.find(c);
            if (it != m_glyphs.end())
            {
                width += it->second.xAdvance * scale;
            }
        }
        return width;
    }

    const CharGlyph* UITextRenderer::GetGlyph(char c) const
    {
        auto it = m_glyphs.find(c);
        if (it != m_glyphs.end())
            return &it->second;
        return nullptr;
    }

} // namespace Ermine
