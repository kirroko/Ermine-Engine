/* Start Header ************************************************************************/
/*!
\file       AudioSystem.cpp
\author     Hurng Kai Rui, h.kairui, 2301278, h.kairui\@digipen.edu
\date       15/9/2025
\brief      Implementation of AudioSystem as a proper ECS System.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AudioSystem.h"

#include "MathVector.h"
#include "AudioManager.h" // Include the CAudioEngine

class ECS;

using namespace Ermine;

bool AudioSystem::s_initialized = false;

void GlobalAudioComponent::PlayMusic(int index) {
    AudioSystem::PlayGlobalMusic(*this, index);
}

void GlobalAudioComponent::StopMusic() {
    AudioSystem::StopGlobalMusic(*this);
}

void GlobalAudioComponent::SetMusicVolume(float volume) {
    musicVolume = std::clamp(volume, 0.0f, 1.0f);

    // Update currently playing music volume if there's music playing
    if (currentMusicChannelId != -1 && currentMusicIndex >= 0 && currentMusicIndex < music.size()) {
        const auto& musicSource = music[currentMusicIndex];
        float finalVolume = AudioSystem::ConvertVolumeToFMOD(
            musicSource.volume * masterVolume * musicVolume
        );
        CAudioEngine::SetChannelVolume(currentMusicChannelId, finalVolume);
    }
}

void GlobalAudioComponent::PlaySFX(int index) {
    AudioSystem::PlayGlobalSFX(*this, index);
}

void GlobalAudioComponent::PlaySFX(const std::string& name) {
    AudioSystem::PlayGlobalSFX(*this, name);
}

void GlobalAudioComponent::SetSFXVolume(float volume) {
    sfxVolume = std::clamp(volume, 0.0f, 1.0f);
    // Note: SFX volumes will be applied to new SFX plays
    // Existing SFX can't be updated since we don't track their channel IDs
}

int GlobalAudioComponent::GetSFXIndex(const std::string& name) const {
    for (size_t i = 0; i < sfx.size(); ++i) {
        if (sfx[i].audioName == name) {
            return static_cast<int>(i);
        }
    }
    return -1; // Not found
}

int GlobalAudioComponent::GetMusicIndex(const std::string& name) const {
    for (size_t i = 0; i < music.size(); ++i) {
        if (music[i].audioName == name) {
            return static_cast<int>(i);
        }
    }
    return -1; // Not found
}

void GlobalAudioComponent::AddMusicSource(const std::string& name, const std::string& path) {
    // Check if music with this name already exists
    for (const auto& musicSource : music) {
        if (musicSource.audioName == name) {
            std::cout << "Music source '" << name << "' already exists!" << std::endl;
            return;
        }
    }

    // Add new music source
    AudioSource newMusic;
    newMusic.audioName = name;
    newMusic.audioPath = path;
    newMusic.volume = 1.0f; // Default volume
    music.push_back(newMusic);
}

void GlobalAudioComponent::AddSFXSource(const std::string& name, const std::string& path) {
    // Check if SFX with this name already exists
    for (const auto& sfxSource : sfx) {
        if (sfxSource.audioName == name) {
            std::cout << "SFX source '" << name << "' already exists!" << std::endl;
            return;
        }
    }

    // Add new SFX source
    AudioSource newSFX;
    newSFX.audioName = name;
    newSFX.audioPath = path;
    newSFX.volume = 1.0f; // Default volume
    sfx.push_back(newSFX);
}



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

    // Update listener to follow main camera 
    //if (HasMainCamera()) {
    //    auto& camera = GetMainCamera();
    //    CAudioEngine::SetListenerAttributes(
    //        camera.GetPosition(),
    //        camera.GetVelocity(),    // Calculate from previous position
    //        camera.GetForward(),
    //        camera.GetUp()
    //    );
    //}

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

        // **NEW: Update volume if it changed while playing**
        if (audioComp.isPlaying && audioComp.channelId != -1)
        {
            float currentVolume = ConvertVolumeToFMOD(audioComp.volume);
            std::cout << "Updating volume for channel " << audioComp.channelId
                << " to " << audioComp.volume << " (dB: " << currentVolume << ")" << std::endl;
            CAudioEngine::SetChannelVolume(audioComp.channelId, currentVolume);
        }

        // Update 3D position if following transform and currently playing
        if (audioComp.isPlaying && audioComp.followTransform && audioComp.channelId != -1)
        {
            UpdateEntityAudio3D(audioComp, transform);
        }

        // Check if audio is still playing (update isPlaying status)
        if (audioComp.channelId != -1)
        {
            audioComp.isPlaying = CAudioEngine::IsPlaying(audioComp.channelId);
        }
    }
}

void AudioSystem::UpdateGlobalAudio(GlobalAudioComponent& globalAudio)
{
    // Handle any global audio state updates here
    // Check if current music is still playing, update volumes, etc.

    if (globalAudio.currentMusicChannelId != -1)
    {
        // Check if music is still playing
        if (!CAudioEngine::IsPlaying(globalAudio.currentMusicChannelId))
        {
            globalAudio.currentMusicChannelId = -1;
            globalAudio.currentMusicIndex = -1;
        }
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

    // Convert Vec3 to Vector3D for CAudioEngine compatibility
    Vector3D position(0.0f, 0.0f, 0.0f);
    globalAudio.currentMusicChannelId = CAudioEngine::PlaySounds(musicSource.audioPath, position, finalVolume);
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

    // Convert Vec3 to Vector3D for CAudioEngine compatibility
    Vector3D position(0.0f, 0.0f, 0.0f);
    CAudioEngine::PlaySounds(sfxSource.audioPath, position, finalVolume);
}

void AudioSystem::PlayGlobalSFX(GlobalAudioComponent& globalAudio, const std::string& name)
{
    // Find the SFX by name
    for (size_t i = 0; i < globalAudio.sfx.size(); ++i)
    {
        if (globalAudio.sfx[i].audioName == name)
        {
            PlayGlobalSFX(globalAudio, static_cast<int>(i));
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
        CAudioEngine::StopChannel(globalAudio.currentMusicChannelId);
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
        audioComp.isPlaying = CAudioEngine::IsEventPlaying(audioComp.eventName);
        audioComp.channelId = -1; // Events don't use channel IDs
    }
    else if (!audioComp.soundName.empty())
    {
        // Handle regular sounds
        CAudioEngine::LoadSound(audioComp.soundName, audioComp.is3D, audioComp.isLooping, audioComp.isStreaming);

        // Convert Vec3 to Vector3D for CAudioEngine compatibility
        Vector3D position(0.0f, 0.0f, 0.0f);
        if (audioComp.is3D)
        {
            position = Vector3D(transform.position.x, transform.position.y, transform.position.z);
        }

        // Convert 0-1 volume to dB
        float volumeDB = ConvertVolumeToFMOD(audioComp.volume);

        audioComp.channelId = CAudioEngine::PlaySounds(audioComp.soundName, position, volumeDB);
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
        CAudioEngine::StopChannel(audioComp.channelId);
        audioComp.channelId = -1;
    }

    audioComp.isPlaying = false;
}

void AudioSystem::UpdateEntityAudio3D(AudioComponent& audioComp, const Transform& transform)
{
    if (audioComp.channelId != -1 && audioComp.is3D)
    {
        // Convert Vec3 to Vector3D for CAudioEngine compatibility
        Vector3D position(transform.position.x, transform.position.y, transform.position.z);
        CAudioEngine::SetChannel3dPosition(audioComp.channelId, position);
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

// Helper function is no longer needed since Vec3 and Vector3D are the same type