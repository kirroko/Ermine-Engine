/* Start Header ************************************************************************/
/*!
\file       VideoManager.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       30/01/2026
\brief      Implementation of the VideoManager (CVideoEngine) class.
            Wraps PL_MPEG for MPEG1 video decoding with GPU YCbCr to RGB conversion.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"

// PL_MPEG implementation - must be defined before including pl_mpeg.h
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include "VideoManager.h"
#include "Shader.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "GPUProfiler.h"

using namespace Ermine;
using namespace Ermine::graphics;

// Helper macro to cast void* to plm_t*
#define PLM(ptr) (static_cast<plm_t*>(ptr))

// Static member initialization
std::map<int, std::unique_ptr<VideoInstance>> CVideoEngine::s_Videos;
int CVideoEngine::s_NextVideoId = 1;
bool CVideoEngine::s_Initialized = false;
std::shared_ptr<graphics::Shader> CVideoEngine::s_YCbCrShader = nullptr;
GLuint CVideoEngine::s_QuadVAO = 0;
GLuint CVideoEngine::s_QuadVBO = 0;

// Forward declarations for internal functions
static void OnVideoFrameCallback(plm_t* plm, plm_frame_t* frame, void* user);
static void OnAudioSamplesCallback(plm_t* plm, plm_samples_t* samples, void* user);
static void UpdateYCbCrTextures(VideoInstance& instance, plm_frame_t* frame);
static void ConvertYCbCrToRGB(VideoInstance& instance);

/*!***********************************************************************
\brief
    Initialize the video engine. Creates shader and quad for YCbCr conversion.
*************************************************************************/
void CVideoEngine::Init()
{
    if (s_Initialized)
        return;

    // Load YCbCr to RGB conversion shader
    s_YCbCrShader = AssetManager::GetInstance().LoadShader(
        "../Resources/Shaders/ycbcr_vertex.glsl",
        "../Resources/Shaders/ycbcr_fragment.glsl"
    );

    if (!s_YCbCrShader || !s_YCbCrShader->IsValid())
    {
        EE_CORE_ERROR("Failed to load YCbCr shader for video playback");
        return;
    }

    // Create fullscreen quad for rendering
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,

        -1.0f,  1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &s_QuadVAO);
    glGenBuffers(1, &s_QuadVBO);

    glBindVertexArray(s_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    s_Initialized = true;
    EE_CORE_INFO("CVideoEngine initialized");
}

/*!***********************************************************************
\brief
    Update all playing videos. Call this every frame with delta time.
\param deltaTime Time since last frame in seconds.
*************************************************************************/
void CVideoEngine::Update(float deltaTime)
{
    if (!s_Initialized)
        return;

    for (auto& [id, instance] : s_Videos)
    {
        if (!instance || !instance->plm)
            continue;

        if (instance->isPlaying && !instance->isPaused)
        {
            // Decode video/audio for this frame
            double adjustedDelta = deltaTime * instance->playbackSpeed;
            plm_decode(PLM(instance->plm), adjustedDelta);

            // Check if video has ended
            if (plm_has_ended(PLM(instance->plm)))
            {
                if (instance->isLooping)
                {
                    plm_rewind(PLM(instance->plm));
                }
                else
                {
                    instance->isPlaying = false;
                }
            }
        }
    }
}

/*!***********************************************************************
\brief
    Shutdown the video engine and release all resources.
*************************************************************************/
void CVideoEngine::Shutdown()
{
    if (!s_Initialized)
        return;

    // Unload all videos
    for (auto& [id, instance] : s_Videos)
    {
        if (instance)
        {
            if (instance->plm)
            {
                plm_destroy(PLM(instance->plm));
                instance->plm = nullptr;
            }
            DestroyVideoTextures(*instance);
        }
    }
    s_Videos.clear();

    // Clean up shader quad
    if (s_QuadVAO)
    {
        glDeleteVertexArrays(1, &s_QuadVAO);
        s_QuadVAO = 0;
    }
    if (s_QuadVBO)
    {
        glDeleteBuffers(1, &s_QuadVBO);
        s_QuadVBO = 0;
    }

    s_YCbCrShader.reset();
    s_Initialized = false;

    EE_CORE_INFO("CVideoEngine shutdown");
}

/*!***********************************************************************
\brief
    Load a video file and return its ID.
\param videoPath Path to the .mpg video file.
\return Video ID (>0) on success, -1 on failure.
*************************************************************************/
int CVideoEngine::LoadVideo(const std::string& videoPath)
{
    if (!s_Initialized)
    {
        EE_CORE_ERROR("CVideoEngine not initialized");
        return -1;
    }

    // Create PL_MPEG instance
    plm_t* plm = plm_create_with_filename(videoPath.c_str());
    if (!plm)
    {
        EE_CORE_ERROR("Failed to load video: {}", videoPath);
        return -1;
    }

    // Create video instance
    auto instance = std::make_unique<VideoInstance>();
    instance->plm = plm;
    instance->filePath = videoPath;
    instance->width = plm_get_width(plm);
    instance->height = plm_get_height(plm);
    instance->framerate = plm_get_framerate(plm);
    instance->duration = plm_get_duration(plm);

    // Set up callbacks
    int videoId = s_NextVideoId++;
    plm_set_video_decode_callback(plm, OnVideoFrameCallback, reinterpret_cast<void*>(static_cast<intptr_t>(videoId)));
    plm_set_audio_decode_callback(plm, OnAudioSamplesCallback, reinterpret_cast<void*>(static_cast<intptr_t>(videoId)));

    // Set audio lead time for sync
    plm_set_audio_lead_time(plm, 0.02); // 20ms lead time

    // Create GPU textures
    CreateVideoTextures(*instance);

    s_Videos[videoId] = std::move(instance);

    EE_CORE_INFO("Loaded video: {} ({}x{}, {:.2f}fps, {:.2f}s) as ID {}",
        videoPath, plm_get_width(plm), plm_get_height(plm),
        plm_get_framerate(plm), plm_get_duration(plm), videoId);

    return videoId;
}

/*!***********************************************************************
\brief
    Unload a video and free its resources.
\param videoId The video ID to unload.
*************************************************************************/
void CVideoEngine::UnloadVideo(int videoId)
{
    auto it = s_Videos.find(videoId);
    if (it == s_Videos.end())
        return;

    auto& instance = it->second;
    if (instance)
    {
        // Stop audio if playing
        if (instance->audioChannelId != -1)
        {
            CAudioEngine::StopChannel(instance->audioChannelId);
        }

        if (instance->plm)
        {
            plm_destroy(PLM(instance->plm));
            instance->plm = nullptr;
        }
        DestroyVideoTextures(*instance);
    }

    s_Videos.erase(it);
    EE_CORE_INFO("Unloaded video ID {}", videoId);
}

/*!***********************************************************************
\brief
    Check if a video is loaded.
\param videoId The video ID to check.
\return True if loaded.
*************************************************************************/
bool CVideoEngine::IsVideoLoaded(int videoId)
{
    return s_Videos.find(videoId) != s_Videos.end();
}

/*!***********************************************************************
\brief
    Start playing a video from current position.
\param videoId The video ID to play.
*************************************************************************/
void CVideoEngine::Play(int videoId)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->isPlaying = true;
    instance->isPaused = false;
    plm_set_loop(PLM(instance->plm), instance->isLooping ? 1 : 0);
}

/*!***********************************************************************
\brief
    Pause video playback.
\param videoId The video ID to pause.
*************************************************************************/
void CVideoEngine::Pause(int videoId)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->isPaused = true;
}

/*!***********************************************************************
\brief
    Resume paused video playback.
\param videoId The video ID to resume.
*************************************************************************/
void CVideoEngine::Resume(int videoId)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->isPaused = false;
}

/*!***********************************************************************
\brief
    Stop video playback and rewind to beginning.
\param videoId The video ID to stop.
*************************************************************************/
void CVideoEngine::Stop(int videoId)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->isPlaying = false;
    instance->isPaused = false;
    plm_rewind(PLM(instance->plm));

    // Stop audio
    if (instance->audioChannelId != -1)
    {
        CAudioEngine::StopChannel(instance->audioChannelId);
        instance->audioChannelId = -1;
    }
}

/*!***********************************************************************
\brief
    Seek to a specific time in the video.
\param videoId The video ID.
\param timeSeconds Time to seek to in seconds.
*************************************************************************/
void CVideoEngine::Seek(int videoId, double timeSeconds)
{
    auto* instance = GetInstance(videoId);
    if (!instance || !instance->plm)
        return;

    // Clamp to valid range
    timeSeconds = std::max(0.0, std::min(timeSeconds, instance->duration));
    plm_seek(PLM(instance->plm), timeSeconds, FALSE);
}

// Settings
void CVideoEngine::SetLoop(int videoId, bool loop)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->isLooping = loop;
    if (instance->plm)
        plm_set_loop(PLM(instance->plm), loop ? 1 : 0);
}

void CVideoEngine::SetVolume(int videoId, float volume)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->volume = std::clamp(volume, 0.0f, 1.0f);
}

void CVideoEngine::SetPlaybackSpeed(int videoId, float speed)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->playbackSpeed = std::clamp(speed, 0.25f, 4.0f);
}

void CVideoEngine::SetMute(int videoId, bool mute)
{
    auto* instance = GetInstance(videoId);
    if (!instance)
        return;

    instance->muteAudio = mute;
}

// Query state
bool CVideoEngine::IsPlaying(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? (instance->isPlaying && !instance->isPaused) : false;
}

bool CVideoEngine::IsPaused(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? instance->isPaused : false;
}

bool CVideoEngine::HasEnded(int videoId)
{
    auto* instance = GetInstance(videoId);
    if (!instance || !instance->plm)
        return true;
    return plm_has_ended(PLM(instance->plm)) != 0;
}

double CVideoEngine::GetPosition(int videoId)
{
    auto* instance = GetInstance(videoId);
    if (!instance || !instance->plm)
        return 0.0;
    return plm_get_time(PLM(instance->plm));
}

double CVideoEngine::GetDuration(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? instance->duration : 0.0;
}

int CVideoEngine::GetWidth(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? instance->width : 0;
}

int CVideoEngine::GetHeight(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? instance->height : 0;
}

double CVideoEngine::GetFramerate(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? instance->framerate : 0.0;
}

GLuint CVideoEngine::GetOutputTexture(int videoId)
{
    auto* instance = GetInstance(videoId);
    return instance ? instance->outputTexture : 0;
}

// Private helpers
VideoInstance* CVideoEngine::GetInstance(int videoId)
{
    auto it = s_Videos.find(videoId);
    if (it == s_Videos.end())
        return nullptr;
    return it->second.get();
}

/*!***********************************************************************
\brief
    Callback called by PL_MPEG when a video frame is decoded.
*************************************************************************/
static void OnVideoFrameCallback(plm_t* plm, plm_frame_t* frame, void* user)
{
    (void)plm;
    int videoId = static_cast<int>(reinterpret_cast<intptr_t>(user));
    auto* instance = CVideoEngine::GetInstance(videoId);
    if (!instance)
        return;

    // Update YCbCr textures with decoded frame data
    UpdateYCbCrTextures(*instance, frame);

    // Convert YCbCr to RGB using shader
    ConvertYCbCrToRGB(*instance);
}

/*!***********************************************************************
\brief
    Callback called by PL_MPEG when audio samples are decoded.
*************************************************************************/
static void OnAudioSamplesCallback(plm_t* plm, plm_samples_t* samples, void* user)
{
    (void)plm;
    int videoId = static_cast<int>(reinterpret_cast<intptr_t>(user));
    auto* instance = CVideoEngine::GetInstance(videoId);
    if (!instance || instance->muteAudio)
        return;

    // For now, we skip audio implementation to keep things simple
    // Audio can be added later using FMOD streaming
    (void)samples;
}

/*!***********************************************************************
\brief
    Create GPU textures for video decoding.
*************************************************************************/
void CVideoEngine::CreateVideoTextures(VideoInstance& instance)
{
    int w = instance.width;
    int h = instance.height;

    // Y plane is full resolution
    glGenTextures(1, &instance.textureY);
    glBindTexture(GL_TEXTURE_2D, instance.textureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Cb and Cr planes are half resolution
    int cw = (w + 1) / 2;
    int ch = (h + 1) / 2;

    glGenTextures(1, &instance.textureCb);
    glBindTexture(GL_TEXTURE_2D, instance.textureCb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, cw, ch, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &instance.textureCr);
    glBindTexture(GL_TEXTURE_2D, instance.textureCr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, cw, ch, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Output RGB texture
    glGenTextures(1, &instance.outputTexture);
    glBindTexture(GL_TEXTURE_2D, instance.outputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Framebuffer for YCbCr to RGB conversion
    glGenFramebuffers(1, &instance.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, instance.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instance.outputTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        EE_CORE_ERROR("Video framebuffer incomplete for video: {}", instance.filePath);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GPUProfiler::TrackMemoryAllocation(w * h * 4 + w * h + cw * ch * 2, "VideoTexture");
}

/*!***********************************************************************
\brief
    Destroy GPU textures for a video instance.
*************************************************************************/
void CVideoEngine::DestroyVideoTextures(VideoInstance& instance)
{
    if (instance.fbo)
    {
        glDeleteFramebuffers(1, &instance.fbo);
        instance.fbo = 0;
    }
    if (instance.textureY)
    {
        glDeleteTextures(1, &instance.textureY);
        instance.textureY = 0;
    }
    if (instance.textureCb)
    {
        glDeleteTextures(1, &instance.textureCb);
        instance.textureCb = 0;
    }
    if (instance.textureCr)
    {
        glDeleteTextures(1, &instance.textureCr);
        instance.textureCr = 0;
    }
    if (instance.outputTexture)
    {
        glDeleteTextures(1, &instance.outputTexture);
        instance.outputTexture = 0;
    }

    int cw = (instance.width + 1) / 2;
    int ch = (instance.height + 1) / 2;
    GPUProfiler::TrackMemoryDeallocation(instance.width * instance.height * 4 +
        instance.width * instance.height + cw * ch * 2, "VideoTexture");
}

/*!***********************************************************************
\brief
    Update YCbCr textures with decoded frame data from PL_MPEG.
*************************************************************************/
static void UpdateYCbCrTextures(VideoInstance& instance, plm_frame_t* frame)
{
    // Update Y plane
    glBindTexture(GL_TEXTURE_2D, instance.textureY);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        frame->y.width, frame->y.height,
        GL_RED, GL_UNSIGNED_BYTE, frame->y.data);

    // Update Cb plane
    glBindTexture(GL_TEXTURE_2D, instance.textureCb);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        frame->cb.width, frame->cb.height,
        GL_RED, GL_UNSIGNED_BYTE, frame->cb.data);

    // Update Cr plane
    glBindTexture(GL_TEXTURE_2D, instance.textureCr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        frame->cr.width, frame->cr.height,
        GL_RED, GL_UNSIGNED_BYTE, frame->cr.data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

/*!***********************************************************************
\brief
    Convert YCbCr textures to RGB using GPU shader.
*************************************************************************/
static void ConvertYCbCrToRGB(VideoInstance& instance)
{
    if (!CVideoEngine::s_YCbCrShader || !CVideoEngine::s_YCbCrShader->IsValid())
        return;

    // Save current state
    GLint prevFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Bind our FBO and set viewport to video size
    glBindFramebuffer(GL_FRAMEBUFFER, instance.fbo);
    glViewport(0, 0, instance.width, instance.height);

    // Use shader
    CVideoEngine::s_YCbCrShader->Bind();

    // Bind YCbCr textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, instance.textureY);
    CVideoEngine::s_YCbCrShader->SetUniform1i("u_TextureY", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, instance.textureCb);
    CVideoEngine::s_YCbCrShader->SetUniform1i("u_TextureCb", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, instance.textureCr);
    CVideoEngine::s_YCbCrShader->SetUniform1i("u_TextureCr", 2);

    // Draw fullscreen quad
    glBindVertexArray(CVideoEngine::s_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Restore state
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
