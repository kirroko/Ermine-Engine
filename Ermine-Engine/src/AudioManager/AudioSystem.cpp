/* Start Header ************************************************************************/
/*!
\file       AudioSystem.cpp
\author     [Your Name]
\date       [Current Date]
\brief      Implementation of AudioSystem as a proper ECS System.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AudioSystem.h"

#include "MathVector.h"

class ECS;

using namespace Ermine;

bool AudioSystem::s_initialized = false;

void AudioSystem::Init()
{
    if (!s_initialized)
    {
        // Initialize the underlying CAudioEngine
        CAudioEngine::Init();
        s_initialized = true;
    }
}

void AudioSystem::Update()
{
    // First, update the underlying FMOD system
    CAudioEngine::Update();

    // Update all entities with AudioComponents
    UpdateAudioComponents();
}

void AudioSystem::UpdateAudioComponents()
{
    auto& ecs = ECS::GetInstance();

    // Iterate through all entities tracked by this system
    // m_Entities contains entities that match our signature (AudioComponent + Transform)
    for (EntityID entity : m_Entities)
    {
        if (!ecs.IsEntityValid(entity))
            continue;

        auto& audioComp = ecs.GetComponent<AudioComponent>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);

        // Handle play requests
        if (audioComp.shouldPlay)
        {
            PlayEntityAudio(audioComp, transform);
            audioComp.shouldPlay = false;
        }

        // Handle stop requests
        if (audioComp.shouldStop)
        {
            StopEntityAudio(audioComp);
            audioComp.shouldStop = false;
        }

        // Update 3D position if following transform and currently playing
        if (audioComp.isPlaying && audioComp.followTransform && audioComp.channelId != -1)
        {
            UpdateEntityAudio3D(audioComp, transform);
        }
    }
}

void AudioSystem::UpdateGlobalAudio(GlobalAudioComponent& globalAudio)
{
    // Handle any global audio state updates here
    // Check if current music is still playing, update volumes, etc.

    if (globalAudio.currentMusicChannelId != -1)
    {
        // You might want to add IsChannelPlaying to your CAudioEngine
        // For now, we'll assume it keeps playing until stopped
    }
}

void AudioSystem::PlayGlobalMusic(GlobalAudioComponent& globalAudio, int index)
{
    if (index < 0 || index >= globalAudio.music.size())
        return;

    // Stop current music if playing
    StopGlobalMusic(globalAudio);

    const auto& musicSource = globalAudio.music[index];

    // Load and play the music
    CAudioEngine::LoadSound(musicSource.audioPath, false, true, true); // 2D, looping, streaming

    // Convert volume from 0-1 to dB and apply master/music volume
    float finalVolume = ConvertVolumeToFMOD(musicSource.volume * globalAudio.masterVolume * globalAudio.musicVolume);

    globalAudio.currentMusicChannelId = CAudioEngine::PlaySounds(musicSource.audioPath, Vec3{ 0,0,0 }, finalVolume);
    globalAudio.currentMusicIndex = index;
}

void AudioSystem::PlayGlobalSFX(GlobalAudioComponent& globalAudio, int index)
{
    if (index < 0 || index >= globalAudio.sfx.size())
        return;

    const auto& sfxSource = globalAudio.sfx[index];

    // Load and play the SFX
    CAudioEngine::LoadSound(sfxSource.audioPath, false, false, false); // 2D, no loop, no stream

    // Convert volume from 0-1 to dB and apply master/SFX volume
    float finalVolume = ConvertVolumeToFMOD(sfxSource.volume * globalAudio.masterVolume * globalAudio.sfxVolume);

    CAudioEngine::PlaySounds(sfxSource.audioPath, Vec3{ 0,0,0 }, finalVolume);
}

void AudioSystem::PlayGlobalSFX(GlobalAudioComponent& globalAudio, const std::string& name)
{
    // Find the SFX by name
    for (int i = 0; i < globalAudio.sfx.size(); ++i)
    {
        if (globalAudio.sfx[i].audioName == name)
        {
            PlayGlobalSFX(globalAudio, i);
            return;
        }
    }

    // SFX not found
    std::cout << "SFX '" << name << "' not found in GlobalAudioComponent" << std::endl;
}

void AudioSystem::StopGlobalMusic(GlobalAudioComponent& globalAudio)
{
    if (globalAudio.currentMusicChannelId != -1)
    {
        // Note: You'll need to add StopChannel to your CAudioEngine
        // For now, this is a placeholder
        // CAudioEngine::StopChannel(globalAudio.currentMusicChannelId);

        globalAudio.currentMusicChannelId = -1;
        globalAudio.currentMusicIndex = -1;
    }
}

void AudioSystem::PlayEntityAudio(AudioComponent& audioComp, const Transform& transform)
{
    if (audioComp.soundName.empty() && audioComp.eventName.empty())
        return;

    if (!audioComp.eventName.empty())
    {
        // Handle FMOD Studio events
        CAudioEngine::LoadEvent(audioComp.eventName);
        CAudioEngine::PlayEvent(audioComp.eventName);
        audioComp.isPlaying = true;
    }
    else if (!audioComp.soundName.empty())
    {
        // Handle regular sounds
        CAudioEngine::LoadSound(audioComp.soundName, audioComp.is3D, audioComp.isLooping, audioComp.isStreaming);

        Vec3 position = audioComp.is3D ? transform.position : Vec3{ 0,0,0 };
        audioComp.channelId = CAudioEngine::PlaySounds(audioComp.soundName, position, audioComp.volume);
        audioComp.isPlaying = (audioComp.channelId != -1);
    }
}

void AudioSystem::StopEntityAudio(AudioComponent& audioComp)
{
    if (!audioComp.eventName.empty())
    {
        CAudioEngine::StopEvent(audioComp.eventName);
    }
    else if (audioComp.channelId != -1)
    {
        // Note: You'll need to add StopChannel to your CAudioEngine
        // CAudioEngine::StopChannel(audioComp.channelId);
        audioComp.channelId = -1;
    }

    audioComp.isPlaying = false;
}

void AudioSystem::UpdateEntityAudio3D(AudioComponent& audioComp, const Transform& transform)
{
    if (audioComp.channelId != -1 && audioComp.is3D)
    {
        CAudioEngine::SetChannel3dPosition(audioComp.channelId, transform.position);
    }
}

void AudioSystem::Shutdown()
{
    if (s_initialized)
    {
        CAudioEngine::Shutdown();
        s_initialized = false;
    }
}

// Utility functions for volume conversion
float AudioSystem::ConvertVolumeToFMOD(float volume01)
{
    // Convert 0-1 volume to FMOD dB range (-60 to 0)
    if (volume01 <= 0.0f) return -60.0f;
    if (volume01 >= 1.0f) return 0.0f;

    return CAudioEngine::VolumeTodB(volume01);
}

float AudioSystem::ConvertVolumeFromFMOD(float volumeDB)
{
    // Convert FMOD dB to 0-1 range
    return CAudioEngine::dbToVolume(volumeDB);
}