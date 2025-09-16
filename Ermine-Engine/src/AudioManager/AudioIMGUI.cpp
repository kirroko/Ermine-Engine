/* Start Header ************************************************************************/
/*!
\file       AudioImGUI.cpp
\author     Hurng Kai Rui, h.kairui, 2301278, h.kairui\@digipen.edu
\date       15/9/2025
\brief      This file contains the implementation of AudioImGUI for managing audio through ImGUI.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AudioImGUI.h"
#include "AudioSystem.h"
#include "FrameController.h"
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;


namespace Ermine
{
    AudioImGUI::AudioImGUI() : ImGUIWindow("Audio Manager")
    {
        SetStatusMessage("Audio Manager Initialized", 2.0f);
    }

    AudioImGUI::~AudioImGUI() {}

    void AudioImGUI::Update()
    {
        UpdateStatus();
    }

    void AudioImGUI::Render()
    {
        if (ImGui::Begin("Audio Manager"))
        {
            // Status message display
            if (m_StatusTimer > 0.0f)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_StatusMessage.c_str());
                ImGui::Separator();
            }

            // Tabs for different audio management sections
            if (ImGui::BeginTabBar("AudioTabs"))
            {
                if (ImGui::BeginTabItem("Entity Audio"))
                {
                    RenderEntityAudioControls();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Audio Tester"))
                {
                    RenderAudioTester();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Audio Browser"))
                {
                    RenderAudioBrowser();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Global Audio"))
                {
                    RenderGlobalAudioControls();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void AudioImGUI::RenderEntityAudioControls()
    {
        ImGui::Text("Entity Audio Management");
        ImGui::Separator();

        // Get entities with audio components
        auto audioEntities = GetEntitiesWithAudioComponent();

        if (audioEntities.empty())
        {
            ImGui::Text("No entities with AudioComponent found.");
            ImGui::Text("Note: Engine currently doesn't support adding new components.");
            return;
        }

        // Entity selection
        ImGui::Text("Select Entity:");
        static int currentEntityIndex = 0;
        std::vector<std::string> entityNames;

        for (size_t i = 0; i < audioEntities.size(); ++i)
        {
            auto& ecs = ECS::GetInstance();
            std::string name = "Entity " + std::to_string(audioEntities[i]);

            // Try to get object metadata for a better name
            if (ecs.HasComponent<ObjectMetaData>(audioEntities[i]))
            {
                auto& metadata = ecs.GetComponent<ObjectMetaData>(audioEntities[i]);
                if (!metadata.name.empty())
                {
                    name = metadata.name + " (" + std::to_string(audioEntities[i]) + ")";
                }
            }

            entityNames.push_back(name);
        }

        if (ImGui::BeginCombo("##EntitySelect", entityNames[currentEntityIndex].c_str()))
        {
            for (size_t i = 0; i < entityNames.size(); ++i)
            {
                bool isSelected = (currentEntityIndex == static_cast<int>(i));
                if (ImGui::Selectable(entityNames[i].c_str(), isSelected))
                {
                    currentEntityIndex = static_cast<int>(i);
                    m_SelectedEntity = audioEntities[i];
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (currentEntityIndex < audioEntities.size())
        {
            m_SelectedEntity = audioEntities[currentEntityIndex];
            auto& ecs = ECS::GetInstance();
            auto& audioComp = ecs.GetComponent<AudioComponent>(m_SelectedEntity);

            ImGui::Separator();
            ImGui::Text("Entity ID: %u", m_SelectedEntity);

            // Display and edit audio component properties
            ImGui::Text("Sound Path:");
            ImGui::SameLine();
            if (ImGui::Button("Browse##EntityBrowse"))
            {
                m_ShowEntitySoundBrowser = true;
            }

            char soundBuffer[256];
            strncpy(soundBuffer, audioComp.soundName.c_str(), sizeof(soundBuffer) - 1);
            soundBuffer[sizeof(soundBuffer) - 1] = '\0';

            if (ImGui::InputText("##EntitySoundPath", soundBuffer, sizeof(soundBuffer)))
            {
                audioComp.soundName = std::string(soundBuffer);
            }

            // Audio file browser popup for entity
            if (m_ShowEntitySoundBrowser)
            {
                ImGui::OpenPopup("Select Audio File##Entity");
            }

            if (ImGui::BeginPopupModal("Select Audio File##Entity", &m_ShowEntitySoundBrowser, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (RenderAudioFileSelector())
                {
                    audioComp.soundName = m_SelectedAudioFile;
                    m_ShowEntitySoundBrowser = false;
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::Button("Cancel"))
                {
                    m_ShowEntitySoundBrowser = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::SliderFloat("Volume", &audioComp.volume, 0.0f, 1.0f);
            ImGui::Checkbox("3D Audio", &audioComp.is3D);
            ImGui::Checkbox("Looping", &audioComp.isLooping);
            ImGui::Checkbox("Streaming", &audioComp.isStreaming);
            ImGui::Checkbox("Follow Transform", &audioComp.followTransform);

            ImGui::Separator();

            // Audio control buttons
            if (ImGui::Button("Play Audio"))
            {
                audioComp.shouldPlay = true;
                SetStatusMessage("Playing audio for Entity " + std::to_string(m_SelectedEntity));
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Audio"))
            {
                audioComp.shouldStop = true;
                SetStatusMessage("Stopping audio for Entity " + std::to_string(m_SelectedEntity));
            }

            // Display current status
            ImGui::Separator();
            ImGui::Text("Status:");
            ImGui::Text("Playing: %s", audioComp.isPlaying ? "Yes" : "No");
            ImGui::Text("Channel ID: %d", audioComp.channelId);
        }
    }

    void AudioImGUI::RenderAudioTester()
    {
        ImGui::Text("Audio Tester - Quick Sound Testing");
        ImGui::Separator();

        // Sound path input with browse button
        ImGui::Text("Sound File:");
        ImGui::SameLine();
        if (ImGui::Button("Browse##TesterBrowse"))
        {
            m_ShowTesterSoundBrowser = true;
        }

        ImGui::InputText("##TesterSoundPath", m_SoundPath, sizeof(m_SoundPath));

        // Audio file browser popup for tester
        if (m_ShowTesterSoundBrowser)
        {
            ImGui::OpenPopup("Select Audio File##Tester");
        }

        if (ImGui::BeginPopupModal("Select Audio File##Tester", &m_ShowTesterSoundBrowser, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (RenderAudioFileSelector())
            {
                strncpy(m_SoundPath, m_SelectedAudioFile.c_str(), sizeof(m_SoundPath) - 1);
                m_SoundPath[sizeof(m_SoundPath) - 1] = '\0';
                m_ShowTesterSoundBrowser = false;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("Cancel"))
            {
                m_ShowTesterSoundBrowser = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // Audio settings - track if they changed
        bool settingsChanged = false;

        ImGui::SliderFloat("Test Volume", &m_Volume, 0.0f, 1.0f);
        // Note: Volume can be updated in real-time (handled separately)

        if (ImGui::Checkbox("3D Audio##Test", &m_Is3D))
        {
            settingsChanged = true;
        }
        if (ImGui::Checkbox("Looping##Test", &m_IsLooping))
        {
            settingsChanged = true;
        }
        if (ImGui::Checkbox("Streaming##Test", &m_IsStreaming))
        {
            settingsChanged = true;
        }

        if (m_Is3D)
        {
            if (ImGui::InputFloat3("Position", m_Position))
            {
                // 3D position can be updated in real-time
                if (m_TestChannelId != -1 && CAudioEngine::IsPlaying(m_TestChannelId))
                {
                    Vec3 position{ m_Position[0], m_Position[1], m_Position[2] };
                    CAudioEngine::SetChannel3dPosition(m_TestChannelId, position);
                }
            }
        }

        // Show warning if settings changed while playing
        bool isTestPlaying = (m_TestChannelId != -1 && CAudioEngine::IsPlaying(m_TestChannelId));

        if (settingsChanged && isTestPlaying)
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                "Settings changed! Click 'Replay' to apply new settings.");
        }

        ImGui::Separator();

        // Play/Replay button
        const char* buttonText = isTestPlaying ? "Replay" : "Play";
        if (ImGui::Button(buttonText))
        {
            // Stop current audio if playing
            if (m_TestChannelId != -1 && CAudioEngine::IsPlaying(m_TestChannelId))
            {
                CAudioEngine::StopChannel(m_TestChannelId);
                m_TestChannelId = -1;
            }

            // Play with current settings
            m_TestChannelId = PlayTestAudio();
        }

        ImGui::SameLine();

        // Stop button
        if (isTestPlaying)
        {
            if (ImGui::Button("Stop"))
            {
                StopTestAudio();
            }
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::Button("Stop");
            ImGui::EndDisabled();
        }

        // Status display
        ImGui::Separator();
        ImGui::Text("Test Audio Status:");
        ImGui::Text("Playing: %s", isTestPlaying ? "Yes" : "No");
        if (isTestPlaying)
        {
            ImGui::Text("Channel ID: %d", m_TestChannelId);
            ImGui::Text("Volume: %.2f", m_Volume);
            ImGui::Text("3D Audio: %s", m_Is3D ? "Yes" : "No");
            ImGui::Text("Looping: %s", m_IsLooping ? "Yes" : "No");
            ImGui::Text("Streaming: %s", m_IsStreaming ? "Yes" : "No");
        }

        ImGui::Separator();
        ImGui::Text("Common Audio Files:");
        if (ImGui::Button("test.wav"))
        {
            strncpy(m_SoundPath, "../Resources/Audio/test.wav", sizeof(m_SoundPath) - 1);
        }
    }

    void AudioImGUI::RenderAudioBrowser()
    {
        ImGui::Text("Audio File Browser");
        ImGui::Separator();

        if (ImGui::Button("Refresh Audio Files"))
        {
            RefreshAudioFiles();
            SetStatusMessage("Audio files refreshed");
        }

        ImGui::Separator();

        // Display audio files in a list
        if (ImGui::BeginListBox("Available Audio Files", ImVec2(-1, 300)))
        {
            for (size_t i = 0; i < m_AudioFiles.size(); ++i)
            {
                bool isSelected = (m_SelectedAudioIndex == static_cast<int>(i));
                if (ImGui::Selectable(m_AudioFiles[i].filename.c_str(), isSelected))
                {
                    m_SelectedAudioIndex = static_cast<int>(i);
                    m_SelectedAudioFile = m_AudioFiles[i].fullPath;
                }

                // Show tooltip with full path
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", m_AudioFiles[i].fullPath.c_str());
                }
            }
            ImGui::EndListBox();
        }

        if (m_SelectedAudioIndex >= 0 && m_SelectedAudioIndex < m_AudioFiles.size())
        {
            ImGui::Separator();
            ImGui::Text("Selected: %s", m_AudioFiles[m_SelectedAudioIndex].filename.c_str());
            ImGui::Text("Path: %s", m_AudioFiles[m_SelectedAudioIndex].fullPath.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("Tip: Use the 'Audio Tester' tab to test playback with different settings.");
        }
    }

    void AudioImGUI::RenderGlobalAudioControls()
    {
        ImGui::Text("Global Audio System");
        ImGui::Separator();

        // Note: Since your current implementation doesn't have global audio entities,
        // this section provides information about the global audio system status

        ImGui::Text("Audio System Status:");
        ImGui::Text("Initialized: %s", AudioSystem::IsInitialized() ? "Yes" : "No");

        ImGui::Separator();
        ImGui::Text("Global Controls:");

        static float masterVolume = 1.0f;
        if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f))
        {
            // You can implement global volume control here if needed
            SetStatusMessage("Master volume updated");
        }

        if (ImGui::Button("Reload Audio System"))
        {
            AudioSystem::Shutdown();
            AudioSystem::Init();
            SetStatusMessage("Audio System Reloaded");
        }

        ImGui::Separator();
        ImGui::TextWrapped("Note: Global audio components are not currently instantiated in your scene. "
            "To use advanced global audio features, you would need to create entities with GlobalAudioComponent.");
    }

    bool AudioImGUI::RenderAudioFileSelector()
    {
        ImGui::Text("Select an audio file:");
        ImGui::Separator();

        if (ImGui::Button("Refresh"))
        {
            RefreshAudioFiles();
        }

        bool fileSelected = false;

        if (ImGui::BeginListBox("##AudioFileList", ImVec2(400, 200)))
        {
            for (size_t i = 0; i < m_AudioFiles.size(); ++i)
            {
                bool isSelected = (m_SelectedAudioIndex == static_cast<int>(i));
                if (ImGui::Selectable(m_AudioFiles[i].filename.c_str(), isSelected))
                {
                    m_SelectedAudioIndex = static_cast<int>(i);
                    m_SelectedAudioFile = m_AudioFiles[i].fullPath;
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", m_AudioFiles[i].fullPath.c_str());
                }

                // Double-click to select
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    m_SelectedAudioIndex = static_cast<int>(i);
                    m_SelectedAudioFile = m_AudioFiles[i].fullPath;
                    fileSelected = true;
                }
            }
            ImGui::EndListBox();
        }

        if (m_SelectedAudioIndex >= 0 && m_SelectedAudioIndex < m_AudioFiles.size())
        {
            ImGui::Separator();
            ImGui::Text("Selected: %s", m_AudioFiles[m_SelectedAudioIndex].filename.c_str());

            if (ImGui::Button("Select"))
            {
                fileSelected = true;
            }
        }

        return fileSelected;
    }

    void AudioImGUI::RefreshAudioFiles()
    {
        m_AudioFiles.clear();
        m_SelectedAudioIndex = -1;
        m_SelectedAudioFile.clear();

        // Scan for audio files in the Resources/Audio directory
        std::string audioDir = "../Resources/Audio/";

        if (fs::exists(audioDir) && fs::is_directory(audioDir))
        {
            for (const auto& entry : fs::directory_iterator(audioDir))
            {
                if (entry.is_regular_file())
                {
                    std::string extension = entry.path().extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                    // Check for audio file extensions
                    if (extension == ".wav" || extension == ".mp3" || extension == ".ogg")
                    {
                        AudioFileInfo info;
                        info.filename = entry.path().filename().string();
                        info.fullPath = entry.path().string();
                        m_AudioFiles.push_back(info);
                    }
                }
            }
        }

        // Sort files alphabetically
        std::sort(m_AudioFiles.begin(), m_AudioFiles.end(),
            [](const AudioFileInfo& a, const AudioFileInfo& b) {
                return a.filename < b.filename;
            });
    }

    void AudioImGUI::SetStatusMessage(const std::string& message, float duration)
    {
        m_StatusMessage = message;
        m_StatusTimer = duration;
    }

    void AudioImGUI::UpdateStatus()
    {
        if (m_StatusTimer > 0.0f)
        {
            m_StatusTimer -= FrameController::GetDeltaTime();
            if (m_StatusTimer <= 0.0f)
            {
                m_StatusMessage.clear();
            }
        }
    }

    std::vector<EntityID> AudioImGUI::GetEntitiesWithAudioComponent()
    {
        std::vector<EntityID> audioEntities;
        auto& ecs = ECS::GetInstance();

        // Get the audio system to access its tracked entities
        auto audioSystem = ecs.GetSystem<AudioSystem>();
        if (audioSystem)
        {
            // Access the entities tracked by the AudioSystem
            for (EntityID entity : audioSystem->GetEntities())
            {
                if (ecs.IsEntityValid(entity) && ecs.HasComponent<AudioComponent>(entity))
                {
                    audioEntities.push_back(entity);
                }
            }
        }

        return audioEntities;
    }

    int AudioImGUI::PlayTestAudio()
    {
        if (strlen(m_SoundPath) == 0)
        {
            SetStatusMessage("Please enter a sound file path");
            return -1; // Return -1 to indicate failure
        }

        try
        {
            // Load the sound
            CAudioEngine::LoadSound(m_SoundPath, m_Is3D, m_IsLooping, m_IsStreaming);

            // Play the sound
            Vec3 position = m_Is3D ? Vec3{ m_Position[0], m_Position[1], m_Position[2] } : Vec3{ 0, 0, 0 };
            int channelId = CAudioEngine::PlaySounds(m_SoundPath, position, m_Volume);

            if (channelId != -1)
            {
                SetStatusMessage("Playing test audio: " + std::string(m_SoundPath));
                return channelId; // Return the successful channel ID
            }
            else
            {
                SetStatusMessage("Failed to play audio");
                return -1; // Return -1 to indicate failure
            }
        }
        catch (const std::exception& e)
        {
            SetStatusMessage("Error playing audio: " + std::string(e.what()));
            return -1; // Return -1 to indicate failure
        }
    }

    void AudioImGUI::StopAllAudio()
    {
        CAudioEngine::StopAllChannels();
        SetStatusMessage("All audio channels stopped");
    }

    void AudioImGUI::StopTestAudio()
    {
        if (m_TestChannelId != -1)
        {
            CAudioEngine::StopChannel(m_TestChannelId);
            m_TestChannelId = -1;
            SetStatusMessage("Test audio stopped");
        }
        else
        {
            SetStatusMessage("No test audio playing");
        }
    }
}