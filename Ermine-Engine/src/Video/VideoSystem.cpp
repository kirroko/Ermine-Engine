/* Start Header ************************************************************************/
/*!
\file       VideoSystem.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       30/01/2026
\brief      Implementation of VideoSystem as a proper ECS System.
            Manages video playback through VideoComponent and CVideoEngine.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "VideoSystem.h"
#include "VideoManager.h"
#include "EditorGUI.h"
#include "Renderer.h"

using namespace Ermine;

bool VideoSystem::s_initialized = false;
bool VideoSystem::s_isPaused = false;

void VideoSystem::Init()
{
    if (!s_initialized)
    {
        CVideoEngine::Init();
        s_initialized = true;
        EE_CORE_INFO("VideoSystem initialized");
    }
}

void VideoSystem::Shutdown()
{
    if (s_initialized)
    {
        CVideoEngine::Shutdown();
        s_initialized = false;
        EE_CORE_INFO("VideoSystem shutdown");
    }
}

void VideoSystem::PauseAll()
{
    if (s_isPaused) return;

    auto& ecs = ECS::GetInstance();
    for (EntityID entity : ECS::GetInstance().GetSystem<VideoSystem>()->m_Entities)
    {
        if (!ecs.IsEntityValid(entity)) continue;
        if (ecs.HasComponent<VideoComponent>(entity))
        {
            auto& videoComp = ecs.GetComponent<VideoComponent>(entity);
            if (videoComp.videoId != -1 && videoComp.isPlaying)
            {
                CVideoEngine::Pause(videoComp.videoId);
            }
        }
    }
    s_isPaused = true;
}

void VideoSystem::ResumeAll()
{
    if (!s_isPaused) return;

    auto& ecs = ECS::GetInstance();
    for (EntityID entity : ECS::GetInstance().GetSystem<VideoSystem>()->m_Entities)
    {
        if (!ecs.IsEntityValid(entity)) continue;
        if (ecs.HasComponent<VideoComponent>(entity))
        {
            auto& videoComp = ecs.GetComponent<VideoComponent>(entity);
            if (videoComp.videoId != -1 && videoComp.isPlaying)
            {
                CVideoEngine::Resume(videoComp.videoId);
            }
        }
    }
    s_isPaused = false;
}

void VideoSystem::Update(float deltaTime)
{
    auto& ecs = ECS::GetInstance();
    static bool s_WasPlaying = false;
    static bool s_HasAutoPlayed = false;

    bool isPlaying = (editor::EditorGUI::s_state == editor::EditorGUI::SimState::playing);

    // Handle PAUSED state
    if (editor::EditorGUI::s_state == editor::EditorGUI::SimState::paused)
    {
        if (!s_isPaused)
        {
            PauseAll();
        }
        // Still update CVideoEngine for state management (no frame decode when paused)
        CVideoEngine::Update(0.0f);
        return;
    }

    // Handle STOPPED state
    if (!isPlaying)
    {
        if (s_WasPlaying)
        {
            // Stop all videos
            for (EntityID entity : m_Entities)
            {
                if (!ecs.IsEntityValid(entity)) continue;
                if (ecs.HasComponent<ObjectMetaData>(entity))
                {
                    const auto& meta = ecs.GetComponent<ObjectMetaData>(entity);
                    if (!meta.selfActive)
                        continue;
                }
                if (ecs.HasComponent<VideoComponent>(entity))
                {
                    auto& videoComp = ecs.GetComponent<VideoComponent>(entity);
                    if (videoComp.videoId != -1)
                    {
                        CVideoEngine::Stop(videoComp.videoId);
                    }
                    // Reset trigger flags
                    videoComp.shouldPlay = false;
                    videoComp.shouldStop = false;
                    videoComp.shouldPause = false;
                    videoComp.isPlaying = false;
                    videoComp.playbackPosition = 0.0f;
                }
            }
        }

        s_WasPlaying = false;
        s_HasAutoPlayed = false;
        s_isPaused = false;

        CVideoEngine::Update(0.0f);
        return;
    }

    // ===== PLAYING STATE =====

    // Resume if we were paused
    if (s_isPaused)
    {
        ResumeAll();
    }

    // Handle auto-play on first frame of play mode
    if (!s_HasAutoPlayed)
    {
        EE_CORE_INFO("VideoSystem: First frame of play mode, checking {} entities for auto-play", m_Entities.size());
        for (EntityID entity : m_Entities)
        {
            if (!ecs.IsEntityValid(entity)) continue;
            if (ecs.HasComponent<ObjectMetaData>(entity))
            {
                const auto& meta = ecs.GetComponent<ObjectMetaData>(entity);
                if (!meta.selfActive)
                    continue;
            }
            if (ecs.HasComponent<VideoComponent>(entity))
            {
                auto& videoComp = ecs.GetComponent<VideoComponent>(entity);
                EE_CORE_INFO("VideoSystem: Entity {} has VideoComponent, path='{}', playOnStart={}",
                    entity, videoComp.videoPath, videoComp.playOnStart);
                if (videoComp.playOnStart && !videoComp.videoPath.empty())
                {
                    videoComp.shouldPlay = true;
                    EE_CORE_INFO("VideoSystem: Setting shouldPlay=true for entity {}", entity);
                }
            }
        }
        s_HasAutoPlayed = true;
    }

    s_WasPlaying = true;

    // Update the video engine (decodes frames)
    CVideoEngine::Update(deltaTime);

    // Update individual video components
    UpdateVideoComponents(deltaTime);
}

void VideoSystem::UpdateVideoComponents(float deltaTime)
{
    (void)deltaTime;
    auto& ecs = ECS::GetInstance();

    for (EntityID entity : m_Entities)
    {
        if (!ecs.IsEntityValid(entity)) continue;

        // Check if entity is active
        if (ecs.HasComponent<ObjectMetaData>(entity))
        {
            const auto& meta = ecs.GetComponent<ObjectMetaData>(entity);
            if (!meta.selfActive)
                continue;
        }

        if (!ecs.HasComponent<VideoComponent>(entity)) continue;

        auto& videoComp = ecs.GetComponent<VideoComponent>(entity);

        // Handle play trigger
        if (videoComp.shouldPlay)
        {
            EE_CORE_INFO("VideoSystem: Processing shouldPlay trigger for path '{}'", videoComp.videoPath);
            videoComp.shouldPlay = false;
            PlayEntityVideo(videoComp);
        }

        // Handle pause trigger
        if (videoComp.shouldPause)
        {
            videoComp.shouldPause = false;
            PauseEntityVideo(videoComp);
        }

        // Handle stop trigger
        if (videoComp.shouldStop)
        {
            videoComp.shouldStop = false;
            StopEntityVideo(videoComp);
        }

        // Sync state from CVideoEngine
        if (videoComp.videoId != -1)
        {
            videoComp.isPlaying = CVideoEngine::IsPlaying(videoComp.videoId);
            videoComp.playbackPosition = static_cast<float>(CVideoEngine::GetPosition(videoComp.videoId));
            videoComp.duration = static_cast<float>(CVideoEngine::GetDuration(videoComp.videoId));
            videoComp.videoWidth = CVideoEngine::GetWidth(videoComp.videoId);
            videoComp.videoHeight = CVideoEngine::GetHeight(videoComp.videoId);
            videoComp.outputTextureId = CVideoEngine::GetOutputTexture(videoComp.videoId);

            // Handle video end with looping
            if (CVideoEngine::HasEnded(videoComp.videoId))
            {
                if (videoComp.isLooping)
                {
                    CVideoEngine::Seek(videoComp.videoId, 0.0);
                    CVideoEngine::Play(videoComp.videoId);
                }
                else
                {
                    videoComp.isPlaying = false;
                }
            }
        }
    }
}

void VideoSystem::PlayEntityVideo(VideoComponent& videoComp)
{
    if (videoComp.videoPath.empty())
    {
        EE_CORE_WARN("VideoSystem: Cannot play video - no path specified");
        return;
    }

    // Load video if not already loaded
    if (videoComp.videoId == -1)
    {
        videoComp.videoId = CVideoEngine::LoadVideo(videoComp.videoPath);
        if (videoComp.videoId == -1)
        {
            EE_CORE_ERROR("VideoSystem: Failed to load video: {}", videoComp.videoPath);
            return;
        }

        // Get video properties
        videoComp.duration = static_cast<float>(CVideoEngine::GetDuration(videoComp.videoId));
        videoComp.videoWidth = CVideoEngine::GetWidth(videoComp.videoId);
        videoComp.videoHeight = CVideoEngine::GetHeight(videoComp.videoId);
        videoComp.outputTextureId = CVideoEngine::GetOutputTexture(videoComp.videoId);

        // Mark materials dirty so any material using this video gets the texture registered
        auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
        if (renderer)
        {
            renderer->MarkMaterialsDirty();
            EE_CORE_INFO("VideoSystem: Marked materials dirty for video texture registration");
        }
    }

    // Apply settings
    CVideoEngine::SetLoop(videoComp.videoId, videoComp.isLooping);
    CVideoEngine::SetVolume(videoComp.videoId, videoComp.volume);
    CVideoEngine::SetPlaybackSpeed(videoComp.videoId, videoComp.playbackSpeed);
    CVideoEngine::SetMute(videoComp.videoId, videoComp.muteAudio);

    // Play
    CVideoEngine::Play(videoComp.videoId);
    videoComp.isPlaying = true;

    EE_CORE_INFO("VideoSystem: Playing video: {}", videoComp.videoPath);
}

void VideoSystem::PauseEntityVideo(VideoComponent& videoComp)
{
    if (videoComp.videoId != -1)
    {
        CVideoEngine::Pause(videoComp.videoId);
        videoComp.isPlaying = false;
    }
}

void VideoSystem::ResumeEntityVideo(VideoComponent& videoComp)
{
    if (videoComp.videoId != -1)
    {
        CVideoEngine::Resume(videoComp.videoId);
        videoComp.isPlaying = true;
    }
}

void VideoSystem::StopEntityVideo(VideoComponent& videoComp)
{
    if (videoComp.videoId != -1)
    {
        CVideoEngine::Stop(videoComp.videoId);
        videoComp.isPlaying = false;
        videoComp.playbackPosition = 0.0f;
    }
}

void VideoSystem::SeekEntityVideo(VideoComponent& videoComp, float timeSeconds)
{
    if (videoComp.videoId != -1)
    {
        CVideoEngine::Seek(videoComp.videoId, static_cast<double>(timeSeconds));
        videoComp.playbackPosition = timeSeconds;
    }
}
