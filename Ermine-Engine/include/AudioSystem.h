/* Start Header ************************************************************************/
/*!
\file       AudioSystem.h
\author     [Your Name]
\date       [Current Date]
\brief      AudioSystem as a proper ECS System that manages audio components.
            Integrates with the Ermine ECS architecture and handles both
            GlobalAudioComponent and individual AudioComponent updates.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "Components.h"
#include "AudioManager.h"
#include "Systems.h" // For the base System class

namespace Ermine
{
    /*!***********************************************************************
    \brief
     AudioSystem manages all audio components as a proper ECS System.
     It automatically tracks entities with AudioComponent + Transform through
     the ECS signature system.
    *************************************************************************/
    class AudioSystem : public System
    {
    private:
        // Static initialization flag
        static bool s_initialized;

    public:
        AudioSystem() = default;
        ~AudioSystem() = default;

        // Core system functions
        static void Init();
        static void Shutdown();

        // Main update function called by the engine
        void Update();

        // GlobalAudioComponent management (static functions for global access)
        static void UpdateGlobalAudio(GlobalAudioComponent& globalAudio);
        static void PlayGlobalMusic(GlobalAudioComponent& globalAudio, int index);
        static void PlayGlobalSFX(GlobalAudioComponent& globalAudio, int index);
        static void PlayGlobalSFX(GlobalAudioComponent& globalAudio, const std::string& name);
        static void StopGlobalMusic(GlobalAudioComponent& globalAudio);

        // Individual AudioComponent management (instance methods)
        void UpdateAudioComponents();
        void PlayEntityAudio(AudioComponent& audioComp, const Transform& transform);
        void StopEntityAudio(AudioComponent& audioComp);
        void UpdateEntityAudio3D(AudioComponent& audioComp, const Transform& transform);

        // Utility functions
        static float ConvertVolumeToFMOD(float volume01);
        static float ConvertVolumeFromFMOD(float volumeDB);
    };
}