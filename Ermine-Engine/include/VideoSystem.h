/* Start Header ************************************************************************/
/*!
\file       VideoSystem.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       30/01/2026
\brief      VideoSystem as a proper ECS System that manages video components.
            Integrates with the Ermine ECS architecture and handles
            VideoComponent updates and playback synchronization.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "Components.h"
#include "Systems.h"

namespace Ermine
{
    /*!***********************************************************************
    \brief
     VideoSystem manages all video components as a proper ECS System.
     It automatically tracks entities with VideoComponent through
     the ECS signature system.
    *************************************************************************/
    class VideoSystem : public System
    {
    private:
        // Static initialization flag
        static bool s_initialized;
        static bool s_isPaused;

    public:
        VideoSystem() = default;
        ~VideoSystem() = default;

        // Core system functions
        static void Init();
        static void Shutdown();

        // Main update function called by the engine
        void Update(float deltaTime);

        // Pause/Resume all videos
        static void PauseAll();
        static void ResumeAll();
        static bool IsPaused() { return s_isPaused; }

        // Individual VideoComponent management
        void UpdateVideoComponents(float deltaTime);
        void PlayEntityVideo(VideoComponent& videoComp);
        void PauseEntityVideo(VideoComponent& videoComp);
        void ResumeEntityVideo(VideoComponent& videoComp);
        void StopEntityVideo(VideoComponent& videoComp);
        void SeekEntityVideo(VideoComponent& videoComp, float timeSeconds);

        // Utility
        const std::set<EntityID>& GetEntities() const { return m_Entities; }
        static bool IsInitialized() { return s_initialized; }
    };
}
