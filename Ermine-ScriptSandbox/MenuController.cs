/* Start Header ************************************************************************/
/*!
\file       MenuController.cs
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       11/2025
\brief      Scene-based main menu controller that handles menu input and transitions.
            This replaces the ImGui-based MainMenuGUI with a proper scene approach.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using ErmineEngine;
using System;

public class MenuController : MonoBehaviour
{
    // Configuration
    private string cutsceneScenePath = "../Resources/Scenes/cutscene_intro.scene";
    private string levelScenePath = "../Resources/Scenes/level.scene";

    // State
    private bool menuActive = true;

    void Start()
    {
        Debug.Log("Main Menu Controller initialized - Scene-based approach");
        // Cursor should be visible in menu
        // Input.SetCursorVisible(true); // If you have this function
    }

    void Update()
    {
        if (!menuActive)
            return;

        // PLAY button - Space or Enter key
        if (Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.Enter))
        {
            OnPlayClicked();
        }

        // QUIT button - Q key or Escape
        if (Input.GetKeyDown(KeyCode.Q) || Input.GetKeyDown(KeyCode.Escape))
        {
            OnQuitClicked();
        }

        // Direct to level (for testing) - L key
        if (Input.GetKeyDown(KeyCode.L))
        {
            Debug.Log("Loading level directly (skip cutscene)");
            SceneManager.LoadScene(levelScenePath);
            menuActive = false;
        }
    }

    void OnPlayClicked()
    {
        Debug.Log("PLAY clicked - Loading cutscene intro");
        menuActive = false;

        // Load the intro cutscene scene
        SceneManager.LoadScene(cutsceneScenePath);
    }

    void OnQuitClicked()
    {
        Debug.Log("QUIT clicked - Exiting application");

        // Quit the application
        // Note: This won't work in editor, only in build
        Application.Quit();
    }

    // Public methods that can be called by UI button entities if you implement button system
    public void PlayButton()
    {
        OnPlayClicked();
    }

    public void QuitButton()
    {
        OnQuitClicked();
    }
}
