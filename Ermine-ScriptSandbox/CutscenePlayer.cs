/* Start Header ************************************************************************/
/*!
\file       CutscenePlayer.cs
\author     Claude Code
\date       11/2025
\brief      Scene-based cutscene player that manages slideshow playback and transitions.
            This replaces the ImGui-based CutsceneGUI with a proper scene approach.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using ErmineEngine;
using System;

public class CutscenePlayer : MonoBehaviour
{
    // Configuration (can be set from scene or inspector)
    private float slideDuration = 5.0f;  // Seconds per slide
    private string nextScenePath = "../Resources/Scenes/level.scene";

    // Slide data - corresponds to entities in the scene
    private string[] slideEntityNames = { "Slide1", "Slide2", "Slide3" };
    private string[] captions = {
        "Creation & Betrayal: Scientist invents glowing energy machine; shady boss takes over.",
        "Horror Factory: Scientist, now a prisoner, sees his invention used to torture people in a vast, dark factory.",
        "Revenge Awakens: Scientist grabs his old glowing syringe, eyes burning with determination, ready to fight back."
    };

    // State
    private int currentSlideIndex = 0;
    private float slideTimer = 0.0f;
    private bool cutscenePlaying = true;

    void Start()
    {
        Debug.Log("Cutscene Player started");

        // Hide all slides except the first one
        ShowSlide(0);

        // TODO: Display first caption
        Debug.Log($"Caption 1: {captions[0]}");
    }

    void Update()
    {
        if (!cutscenePlaying)
            return;

        // Update timer
        slideTimer += Time.deltaTime;

        // Skip cutscene if player presses Space or Enter
        if (Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.Enter))
        {
            Debug.Log("Cutscene skipped by player");
            FinishCutscene();
            return;
        }

        // Auto-advance to next slide
        if (slideTimer >= slideDuration)
        {
            slideTimer = 0.0f;
            currentSlideIndex++;

            if (currentSlideIndex >= slideEntityNames.Length)
            {
                // Cutscene finished
                FinishCutscene();
            }
            else
            {
                // Show next slide
                ShowSlide(currentSlideIndex);
                Debug.Log($"Caption {currentSlideIndex + 1}: {captions[currentSlideIndex]}");
            }
        }
    }

    void ShowSlide(int index)
    {
        // Hide all slides
        for (int i = 0; i < slideEntityNames.Length; i++)
        {
            // TODO: Find entity by name and enable/disable it
            // Entity slideEntity = FindEntityByName(slideEntityNames[i]);
            // slideEntity.SetActive(i == index);
        }

        Debug.Log($"Showing slide {index + 1}/{slideEntityNames.Length}");
    }

    void FinishCutscene()
    {
        Debug.Log($"Cutscene finished - Loading next scene: {nextScenePath}");
        cutscenePlaying = false;

        // Load the next scene (gameplay level)
        SceneManager.LoadScene(nextScenePath);
    }

    // Public methods for configuration
    public void SetSlideDuration(float duration)
    {
        slideDuration = duration;
    }

    public void SetNextScene(string scenePath)
    {
        nextScenePath = scenePath;
    }
}
