/* Start Header ************************************************************************/
/*!
\file       InspectorGUI.cpp
\author     WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu
\date       Sep 01, 2025
\brief      This file contains the implementation of the Inspector GUI window.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "SettingsGUI.h"
#include <imgui.h>
#include <fstream> // for file existence check
#include <imgui_impl_opengl3.h>

namespace Ermine
{
    // Static variable to hold the open state for all SettingsGUI instances
    static bool s_settingsIsOpen = false;
	static float s_fontSize = 16.0f;
	static float s_baseFontSize = 1.0f;
	static int s_mode = -1; // 0: Light, 1: Dark, 2: Pink, 3: Cyberpunk, 4: Overwatch(Dark), 5: Overwatch(Light)

    SettingsGUI::SettingsGUI()
        : ImGUIWindow("Settings"), settingsIsOpen(s_settingsIsOpen)
    {
    }

    SettingsGUI::SettingsGUI(std::string name)
        : ImGUIWindow(name), settingsIsOpen(s_settingsIsOpen)
    {
    }

    void SettingsGUI::SetSettingsOpen(bool isOpen)
    {
        s_settingsIsOpen = isOpen;
    }

    bool SettingsGUI::GetSettingsOpen()
    {
        return s_settingsIsOpen;
    }

    void SettingsGUI::SetFontSize(float fontSize, float baseFontSize)
    {
        s_fontSize = fontSize;
        s_baseFontSize = baseFontSize;
    }

    float SettingsGUI::GetFontSizeS()
    {
		return s_fontSize;
    }

    float SettingsGUI::GetBaseFontSize()
    {
        return s_baseFontSize;
    }

    void SettingsGUI::SetMode(int mode)
    {
        s_mode = mode;
    }

    int SettingsGUI::GetMode()
    {
        return s_mode;
    }

    void SettingsGUI::Render()
    {
        // Use the static as authoritative state so ImGui can toggle it via the window close button.
        if (s_settingsIsOpen)
        {
            // Pass the address of the static open flag so ImGui will update it if the user closes the window.
            ImGui::Begin("Editor Settings", &s_settingsIsOpen);

            // Base font size to compute a safe global scale without rebuilding textures.
            static const float kBaseFontSize = 16.0f;
            static float fontSize = s_fontSize;
            static char fontPath[260] = "path/to/your/font.ttf"; // optional input field for font path

            ImGui::SliderFloat("Font Size", &fontSize, 10.0f, 32.0f);
            ImGui::InputText("Font Path", fontPath, sizeof(fontPath));

            if (ImGui::Button("Apply Font Size"))
            {
                ImGuiIO& io = ImGui::GetIO();

                // Safer approach: adjust global scale rather than clearing/reloading fonts.
                // This avoids invalidating the GPU font texture which would crash the renderer
                // if the backend texture isn't rebuilt immediately.
                const float scale = (fontSize > 0.0f) ? (fontSize / kBaseFontSize) : 1.0f;
                io.FontGlobalScale = scale;

                // If a custom font path is provided and exists, optionally add it.
                // NOTE: Adding a font with io.Fonts->AddFontFromFileTTF() requires the renderer/backend
                // to rebuild the font atlas texture (e.g. ImGui_ImplOpenGL3_CreateFontsTexture()).
                // Do NOT call backend-specific texture rebuild here unless you also call the matching
                // destroy/create functions from the same backend. Doing so without the renderer
                // coordination is a common cause of crashes.
                std::ifstream fontFile(fontPath);
                if (fontFile.good())
                {
                    // Attempt to add the font to ImGui's font atlas. This is safe on the CPU side,
                    // but the texture on the GPU is not rebuilt here to avoid backend coupling.
                    // Consumers of this engine should ensure the renderer calls the appropriate
                    // backend font texture creation routine after fonts are modified.
                    io.Fonts->AddFontFromFileTTF(fontPath, fontSize);

                    // Example commented reminders for renderer integration (do not call blindly):
                    ImGui_ImplOpenGL3_DestroyFontsTexture();
                    ImGui_ImplOpenGL3_CreateFontsTexture();
                }
                else
                {
                    // If font not found, keep using default font and just apply the scale.
                    // No further action required.
                }
            }

            ImGui::End();

            // Sync instance member with static state
            settingsIsOpen = s_settingsIsOpen;
            s_fontSize = fontSize;
            s_baseFontSize = ImGui::GetIO().FontGlobalScale;
        }
    }
}
