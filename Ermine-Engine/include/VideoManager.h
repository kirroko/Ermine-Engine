/* Start Header ************************************************************************/
/*!
\file       VideoManager.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       30/01/2026
\brief      VideoManager wraps the PL_MPEG library for MPEG1 video playback.
            Provides a simple interface for loading, playing, and managing videos.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "PreCompile.h"
#include <glad/glad.h>
#include <map>
#include <vector>
#include <memory>

namespace Ermine {

    // Forward declaration
    namespace graphics { class Shader; }

    /*!***********************************************************************
    \brief
        Internal structure holding state for a single video instance.
    *************************************************************************/
    struct VideoInstance
    {
        void* plm{ nullptr };               // PL_MPEG decoder instance (plm_t*)

        // YCbCr plane textures (for GPU conversion)
        GLuint textureY{ 0 };               // Luminance plane
        GLuint textureCb{ 0 };              // Chroma blue plane
        GLuint textureCr{ 0 };              // Chroma red plane

        // Output texture (RGB result)
        GLuint outputTexture{ 0 };
        GLuint fbo{ 0 };                    // Framebuffer for YCbCr->RGB conversion

        // Video properties
        int width{ 0 };
        int height{ 0 };
        double framerate{ 0.0 };
        double duration{ 0.0 };

        // Playback state
        bool isPlaying{ false };
        bool isPaused{ false };
        bool isLooping{ false };
        float volume{ 0.5f };
        float playbackSpeed{ 1.0f };
        bool muteAudio{ false };

        // Audio
        int audioChannelId{ -1 };
        std::vector<float> audioBuffer;     // Ring buffer for audio samples
        size_t audioWritePos{ 0 };
        size_t audioReadPos{ 0 };

        // File path for debugging
        std::string filePath;
    };

    /*!***********************************************************************
    \brief
        CVideoEngine is a static class that wraps the PL_MPEG library.
        Follows the same pattern as CAudioEngine for consistency.
    *************************************************************************/
    class CVideoEngine
    {
    public:
        // Core engine functions
        static void Init();
        static void Update(float deltaTime);
        static void Shutdown();

        // Video loading and management
        static int LoadVideo(const std::string& videoPath);
        static void UnloadVideo(int videoId);
        static bool IsVideoLoaded(int videoId);

        // Playback control
        static void Play(int videoId);
        static void Pause(int videoId);
        static void Resume(int videoId);
        static void Stop(int videoId);
        static void Seek(int videoId, double timeSeconds);

        // Settings
        static void SetLoop(int videoId, bool loop);
        static void SetVolume(int videoId, float volume);
        static void SetPlaybackSpeed(int videoId, float speed);
        static void SetMute(int videoId, bool mute);

        // Query state
        static bool IsPlaying(int videoId);
        static bool IsPaused(int videoId);
        static bool HasEnded(int videoId);
        static double GetPosition(int videoId);
        static double GetDuration(int videoId);
        static int GetWidth(int videoId);
        static int GetHeight(int videoId);
        static double GetFramerate(int videoId);
        static GLuint GetOutputTexture(int videoId);

        // Made public for internal callback access
        static VideoInstance* GetInstance(int videoId);

        // Static state (public for internal callback access)
        static std::shared_ptr<graphics::Shader> s_YCbCrShader;
        static GLuint s_QuadVAO;
        static GLuint s_QuadVBO;

    private:
        // Internal helpers (PL_MPEG types hidden in implementation)
        static void CreateVideoTextures(VideoInstance& instance);
        static void DestroyVideoTextures(VideoInstance& instance);

        // Static state
        static std::map<int, std::unique_ptr<VideoInstance>> s_Videos;
        static int s_NextVideoId;
        static bool s_Initialized;
    };

} // namespace Ermine
