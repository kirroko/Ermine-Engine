/* Start Header ************************************************************************/
/*!
\file       MainMenu.cs
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       11/2025
\brief      This file contains the MainMenu script that handles the main menu scene
            logic including play and quit button interactions.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using ErmineEngine;

public class MainMenu : MonoBehaviour
{
    private bool isMenuActive = true;

    void Start()
    {
        // Ensure cursor is visible when menu starts
        Debug.Log("Main Menu started");
    }

    void Update()
    {
        // Handle keyboard input to close menu (ESC key)
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            isMenuActive = false;
        }
    }

    // Note: OnGUI is not supported in this engine.
    // Menu rendering should be handled through the engine's UI system or ImGui.
    // This is a placeholder to prevent compilation errors.
    // The actual menu rendering would need to be implemented using the engine's
    // native UI rendering system (UIComponent, UIRenderSystem, etc.)
}
