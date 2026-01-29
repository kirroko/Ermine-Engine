/* Start Header ************************************************************************/
/*!
\file       VideoManager.cpp
\author     Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       01/29/2026
\brief      Video playback system using pl_mpeg with a dedicated preload thread.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#define PL_MPEG_IMPLEMENTATION
#include "VideoManager.h"

#include <glad/glad.h>
#include <cstring>

#include "AssetManager.h"
#include "AudioManager.h"
#include "Logger.h"
#include "fmod_common.h"
#include "fmod.h"
#include "fmod.hpp"

namespace Ermine
{
    static FMOD_RESULT F_CALL VideoPcmReadCallback(FMOD_SOUND* sound, void* data, unsigned int datalen)
    {
        if (!sound || !data)
            return FMOD_OK;

        void* user = nullptr;
        if (FMOD_Sound_GetUserData(sound, &user) != FMOD_OK || !user)
        {
            memset(data, 0, datalen);
            return FMOD_OK;
        }

        auto* video = reinterpret_cast<VideoManager::VideoData*>(user);
        if (!video || !video->audioEnabled || video->audioBuffer.empty())
        {
            memset(data, 0, datalen);
            return FMOD_OK;
        }

        const size_t samplesRequested = datalen / sizeof(float);
        float* out = reinterpret_cast<float*>(data);

        std::lock_guard<std::mutex> lock(video->audioMutex);
        const size_t totalSamples = video->audioBuffer.size();

        static size_t s_callbackCount = 0;
        if ((s_callbackCount++ % 120) == 0)
        {
            EE_CORE_INFO("VideoAudio: PCM callback requested {} samples, total {}", samplesRequested, totalSamples);
        }

        for (size_t i = 0; i < samplesRequested; ++i)
        {
            if (totalSamples == 0)
            {
                out[i] = 0.0f;
                continue;
            }

            if (video->audioReadIndex >= totalSamples)
            {
                if (video->loop)
                    video->audioReadIndex = 0;
                else
                {
                    out[i] = 0.0f;
                    continue;
                }
            }

            out[i] = video->audioBuffer[video->audioReadIndex];
            ++video->audioReadIndex;
        }

        return FMOD_OK;
    }

    VideoManager::VideoFrame VideoManager::AcquireFrameBuffer(unsigned int width, unsigned int height)
    {
        VideoFrame frame;
        frame.width = width;
        frame.height = height;

        const size_t ySize = static_cast<size_t>(width) * static_cast<size_t>(height);
        const size_t cbSize = static_cast<size_t>(width / 2) * static_cast<size_t>(height / 2);
        const size_t crSize = cbSize;

        std::lock_guard<std::mutex> lock(m_poolMutex);
        size_t index = kInvalidBufferIndex;
        if (!m_freeBuffers.empty())
        {
            index = m_freeBuffers.front();
            m_freeBuffers.pop_front();
        }
        else
        {
            index = m_framePool.size();
            m_framePool.emplace_back();
        }

        auto& buffer = m_framePool[index];
        if (buffer.ySize < ySize)
        {
            buffer.y = std::make_unique<uint8_t[]>(ySize);
            buffer.ySize = ySize;
        }
        if (buffer.cbSize < cbSize)
        {
            buffer.cb = std::make_unique<uint8_t[]>(cbSize);
            buffer.cbSize = cbSize;
        }
        if (buffer.crSize < crSize)
        {
            buffer.cr = std::make_unique<uint8_t[]>(crSize);
            buffer.crSize = crSize;
        }

        frame.bufferIndex = index;
        frame.y_buffer = buffer.y.get();
        frame.cb_buffer = buffer.cb.get();
        frame.cr_buffer = buffer.cr.get();
        return frame;
    }

    void VideoManager::ReleaseFrameBuffer(VideoFrame& frame)
    {
        if (frame.bufferIndex == kInvalidBufferIndex)
            return;

        std::lock_guard<std::mutex> lock(m_poolMutex);
        m_freeBuffers.push_back(frame.bufferIndex);
        frame.bufferIndex = kInvalidBufferIndex;
        frame.y_buffer = nullptr;
        frame.cb_buffer = nullptr;
        frame.cr_buffer = nullptr;
    }

    void VideoManager::ReleasePreloadedFrames(VideoData& video)
    {
        ReleaseFrameBuffer(video.currentFrame);
        video.hasCurrentFrame = false;
        for (auto& frame : video.frames)
            ReleaseFrameBuffer(frame);
        video.frames.clear();
    }
    void VideoManager::Init(int screenWidth, int screenHeight)
    {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;

        m_videoShader = AssetManager::GetInstance().LoadShader(
            "../Resources/Shaders/video_vertex.glsl",
            "../Resources/Shaders/video_fragment.glsl"
        );

        if (!m_videoShader || !m_videoShader->IsValid())
        {
            EE_CORE_ERROR("VideoManager: Failed to load video shaders");
        }

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        float vertices[] = {
            -1.0f, -1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 0.0f,

            -1.0f, -1.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);

        m_decodeStop = false;
        m_decodeThread = std::thread(&VideoManager::DecodeThreadLoop, this);
    }

    /**
     * @brief Shutdown video playback and decoding resources.
     */
    void VideoManager::Shutdown()
    {
        m_decodeStop = true;
        m_decodeCv.notify_all();
        if (m_decodeThread.joinable())
            m_decodeThread.join();

        CleanupAllVideos();

        if (m_VBO)
        {
            glDeleteBuffers(1, &m_VBO);
            m_VBO = 0;
        }

        if (m_VAO)
        {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
    }

    /**
     * @brief Update cached screen size for aspect-fit calculations.
     */
    void VideoManager::OnScreenResize(int width, int height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
        m_quadDirty = true;
    }

    /**
     * @brief Open a video stream and initialize GPU textures for playback.
     */
    bool VideoManager::LoadVideo(const std::string& name, const std::string& filepath, bool loop)
    {
        if (name.empty() || filepath.empty())
        {
            EE_CORE_ERROR("VideoManager: Invalid name or filepath.");
            return false;
        }

        if (VideoExists(name))
        {
            FreeVideo(name);
        }

        auto video = std::make_shared<VideoData>();
        video->filepath = filepath;
        video->loop = loop;

        video->plm = plm_create_with_filename(filepath.c_str());
        if (!video->plm)
        {
            EE_CORE_ERROR("VideoManager: Failed to load video {}", filepath);
            return false;
        }

        if (!plm_probe(video->plm, 5000 * 1024))
        {
            EE_CORE_ERROR("VideoManager: No MPEG streams found in {}", filepath);
            plm_destroy(video->plm);
            video->plm = nullptr;
            return false;
        }

        const int frameRate = static_cast<int>(plm_get_framerate(video->plm));
        const double duration = plm_get_duration(video->plm);

        video->width = plm_get_width(video->plm);
        video->height = plm_get_height(video->plm);
        video->totalFrames = (frameRate > 0) ? static_cast<int>(duration * frameRate + 0.5) : 0;
        if (video->totalFrames <= 0)
            video->totalFrames = 1;

        video->frameDuration = (frameRate > 0) ? (1.0 / static_cast<double>(frameRate)) : (duration / video->totalFrames);

        if (!FinishLoadingVideo(*video))
        {
            EE_CORE_ERROR("VideoManager: Failed to finalize video {}", name);
            ReleaseVideoResources(*video);
            return false;
        }

        // Initialize audio decoding if present
        if (plm_get_num_audio_streams(video->plm) > 0)
        {
            video->audioPlm = plm_create_with_filename(filepath.c_str());
            if (video->audioPlm && plm_probe(video->audioPlm, 5000 * 1024))
            {
                plm_set_video_enabled(video->audioPlm, 0);
                plm_set_audio_enabled(video->audioPlm, 1);

                video->audioSampleRate = plm_get_samplerate(video->audioPlm);
                if (video->audioSampleRate <= 0)
                    video->audioSampleRate = 44100;

                double audioDuration = plm_get_duration(video->audioPlm);
                if (audioDuration <= 0.0)
                    audioDuration = plm_get_duration(video->plm);
                if (audioDuration <= 0.0 && video->frameDuration > 0.0 && video->totalFrames > 0)
                    audioDuration = video->frameDuration * static_cast<double>(video->totalFrames);
                if (audioDuration <= 0.0)
                    audioDuration = 1.0;

                const uint64_t totalPcmSamples = static_cast<uint64_t>(audioDuration * static_cast<double>(video->audioSampleRate) * 2.0);

                video->audioBuffer.clear();
                video->audioBuffer.reserve(totalPcmSamples);
                video->audioReadIndex = 0;

                if (auto* core = CAudioEngine::GetCoreSystem())
                {
                    FMOD_CREATESOUNDEXINFO exinfo{};
                    exinfo.cbsize = sizeof(exinfo);
                    exinfo.numchannels = 2;
                    exinfo.defaultfrequency = video->audioSampleRate;
                    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
                    exinfo.length = static_cast<unsigned int>(totalPcmSamples * sizeof(float));
                    exinfo.decodebuffersize = PLM_AUDIO_SAMPLES_PER_FRAME;
                    exinfo.pcmreadcallback = VideoPcmReadCallback;
                    exinfo.userdata = video.get();

                    FMOD::Sound* sound = nullptr;
                    if (core->createSound(nullptr, FMOD_OPENUSER | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL, &exinfo, &sound) == FMOD_OK && sound)
                    {
                        sound->setUserData(video.get());
                        sound->setMode(video->loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
                        if (totalPcmSamples > 0)
                            sound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, static_cast<unsigned int>(totalPcmSamples), FMOD_TIMEUNIT_PCM);
                        video->audioSound = sound;
                        video->audioEnabled = true;
                        EE_CORE_INFO("VideoAudio: Created FMOD user sound at {} Hz", video->audioSampleRate);
                    }
                    else
                    {
                        EE_CORE_ERROR("VideoAudio: Failed to create FMOD user sound");
                    }
                }
            }
            else
            {
                EE_CORE_WARN("VideoAudio: Audio stream detected but pl_mpeg audio probe failed");
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            video->preloadRequested = true;
            video->preloaded = false;
            video->preloadFailed = false;
            video->hasCurrentFrame = false;
            m_videos[name] = video;
        }


        m_decodeCv.notify_one();
        return true;
    }

    /**
     * @brief Set the active video by name for playback/rendering.
     */
    void VideoManager::SetCurrentVideo(const std::string& name)
    {
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            auto it = m_videos.find(name);
            if (it == m_videos.end())
            {
                EE_CORE_WARN("VideoManager: Video {} not found.", name);
                return;
            }

            m_currentVideo = name;
            m_quadDirty = true;
        }

        m_decodeCv.notify_one();
    }

    /**
     * @brief Begin playback for the current video.
     */
    void VideoManager::Play()
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_currentVideo.empty())
            return;

        auto it = m_videos.find(m_currentVideo);
        if (it == m_videos.end())
            return;

        if (!it->second->loaded)
            return;
        if (!it->second->preloaded)
        {
            EE_CORE_INFO("VideoManager: Video {} not preloaded yet.", m_currentVideo);
            return;
        }

        m_isPlaying = true;
        m_decodeCv.notify_one();

        if (it->second->audioEnabled && it->second->audioSound)
        {
            if (auto* core = CAudioEngine::GetCoreSystem())
            {
                if (!it->second->audioChannel)
                {
                    core->playSound(it->second->audioSound, nullptr, false, &it->second->audioChannel);
                    if (it->second->audioChannel && it->second->audioSampleRate > 0)
                        it->second->audioChannel->setFrequency(static_cast<float>(it->second->audioSampleRate));
                    if (it->second->audioChannel)
                        it->second->audioChannel->setMode(it->second->loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
                    EE_CORE_INFO("VideoAudio: Started playback channel");
                }
                else
                {
                    bool paused = false;
                    it->second->audioChannel->getPaused(&paused);
                    if (paused)
                        it->second->audioChannel->setPaused(false);
                }
            }
        }
    }

    /**
     * @brief Pause playback without resetting state.
     */
    void VideoManager::Pause()
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_isPlaying = false;

        if (!m_currentVideo.empty())
        {
            auto it = m_videos.find(m_currentVideo);
            if (it != m_videos.end() && it->second->audioChannel)
                it->second->audioChannel->setPaused(true);
        }
    }

    /**
     * @brief Stop playback and reset the playhead to the beginning.
     */
    void VideoManager::Stop()
    {
        std::shared_ptr<VideoData> video;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_currentVideo.empty())
                return;

            auto it = m_videos.find(m_currentVideo);
            if (it == m_videos.end())
                return;

            video = it->second;
            video->currentFrameIndex = 0;
            video->elapsedTime = 0.0;
            video->done = false;
            m_isPlaying = false;
        }

        if (video)
        {
            if (video->preloaded && !video->frames.empty())
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                video->currentFrame = video->frames.front();
                video->hasCurrentFrame = true;
            }
            if (video->audioChannel)
            {
                video->audioChannel->stop();
                video->audioChannel = nullptr;
            }

            {
                std::lock_guard<std::mutex> alock(video->audioMutex);
                video->audioReadIndex = 0;
            }
        }
    }

    /**
     * @brief Advance playback timing and consume decoded frames.
     */
    void VideoManager::Update(float deltaTime)
    {
        std::shared_ptr<VideoData> current;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_currentVideo.empty())
                return;

            auto it = m_videos.find(m_currentVideo);
            if (it == m_videos.end())
                return;

            current = it->second;
        }

        if (!current || !current->loaded)
            return;

        UpdateAndAdvance(*current, deltaTime);
        m_decodeCv.notify_one();
    }

    /**
     * @brief Render the current decoded frame to screen.
     */
    void VideoManager::Render()
    {
        if (!m_renderEnabled)
            return;
        std::shared_ptr<VideoData> video;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_currentVideo.empty())
                return;

            auto it = m_videos.find(m_currentVideo);
            if (it == m_videos.end())
                return;

            video = it->second;
        }

        if (!video || !video->loaded)
            return;

        RenderVideoFrame(*video);
    }

    /**
     * @brief Check if the current video is playing.
     */
    bool VideoManager::IsVideoPlaying() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_currentVideo.empty())
            return false;

        auto it = m_videos.find(m_currentVideo);
        if (it == m_videos.end())
            return false;

        const auto& video = it->second;
        return m_isPlaying && !video->done && video->currentFrameIndex < video->totalFrames;
    }

    /**
     * @brief Check if a named video has completed playback.
     */
    bool VideoManager::IsVideoDonePlaying(const std::string& videoName) const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        auto it = m_videos.find(videoName);
        if (it == m_videos.end())
            return false;
        return it->second->done;
    }

    /**
     * @brief Check if a named video exists.
     */
    bool VideoManager::VideoExists(const std::string& videoName) const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_videos.find(videoName) != m_videos.end();
    }

    /**
     * @brief Remove a single video and release its resources.
     */
    void VideoManager::FreeVideo(const std::string& name)
    {
        std::shared_ptr<VideoData> video;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            auto it = m_videos.find(name);
            if (it == m_videos.end())
                return;

            video = it->second;
            video->stopDecoding = true;
            m_videos.erase(it);

            if (m_currentVideo == name)
            {
                m_currentVideo.clear();
                m_isPlaying = false;
            }
        }

        m_decodeCv.notify_all();

        if (video)
        {
            std::lock_guard<std::mutex> decodeLock(video->decodeMutex);
            ReleaseVideoResources(*video);
        }
    }

    /**
     * @brief Release all videos and reset playback state.
     */
    void VideoManager::CleanupAllVideos()
    {
        std::vector<std::shared_ptr<VideoData>> videos;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            for (auto& entry : m_videos)
            {
                entry.second->stopDecoding = true;
                videos.push_back(entry.second);
            }
            m_videos.clear();
            m_currentVideo.clear();
            m_isPlaying = false;
        }

        m_decodeCv.notify_all();

        for (auto& video : videos)
        {
            if (!video)
                continue;
            std::lock_guard<std::mutex> decodeLock(video->decodeMutex);
            ReleaseVideoResources(*video);
        }
    }

#pragma region Private Methods

    /**
     * @brief Create GPU textures required for Y/Cb/Cr planes.
     */
    bool VideoManager::FinishLoadingVideo(VideoData& video)
    {
        if (video.loaded)
            return true;

        if (video.width == 0 || video.height == 0)
            return false;

        glGenTextures(1, &video.tex_y);
        glBindTexture(GL_TEXTURE_2D, video.tex_y);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<int>(video.width), static_cast<int>(video.height), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &video.tex_cb);
        glBindTexture(GL_TEXTURE_2D, video.tex_cb);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<int>(video.width / 2), static_cast<int>(video.height / 2), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &video.tex_cr);
        glBindTexture(GL_TEXTURE_2D, video.tex_cr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<int>(video.width / 2), static_cast<int>(video.height / 2), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenBuffers(kPboCount, video.pbo_y);
        glGenBuffers(kPboCount, video.pbo_cb);
        glGenBuffers(kPboCount, video.pbo_cr);

        const size_t ySize = static_cast<size_t>(video.width) * static_cast<size_t>(video.height);
        const size_t cbSize = static_cast<size_t>(video.width / 2) * static_cast<size_t>(video.height / 2);
        const size_t crSize = cbSize;

        for (int i = 0; i < kPboCount; ++i)
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, video.pbo_y[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, ySize, nullptr, GL_STREAM_DRAW);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, video.pbo_cb[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, cbSize, nullptr, GL_STREAM_DRAW);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, video.pbo_cr[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, crSize, nullptr, GL_STREAM_DRAW);
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        video.pboSizeY = ySize;
        video.pboSizeCb = cbSize;
        video.pboSizeCr = crSize;
        video.pboIndex = 0;

        video.loaded = true;
        video.done = false;
        video.currentFrameIndex = 0;
        video.elapsedTime = 0.0;
        ReleasePreloadedFrames(video);
        video.hasCurrentFrame = false;
        video.preloaded = false;
        video.preloadRequested = false;
        video.preloadFailed = false;
        video.stopDecoding = false;
        m_quadDirty = true;

        return true;
    }

    /**
     * @brief Pop the next decoded frame when the frame duration elapses.
     */
    void VideoManager::UpdateAndAdvance(VideoData& video, float deltaTime)
    {
        if (!m_isPlaying || video.done)
            return;

        bool audioDriven = false;
        int targetFrame = video.currentFrameIndex;

        if (video.audioEnabled && video.audioChannel && video.frameDuration > 0.0)
        {
            unsigned int positionMs = 0;
            if (video.audioChannel->getPosition(&positionMs, FMOD_TIMEUNIT_MS) == FMOD_OK)
            {
                audioDriven = true;
                const double audioSeconds = static_cast<double>(positionMs) / 1000.0;
                targetFrame = static_cast<int>(audioSeconds / video.frameDuration);
                if (video.loop && video.totalFrames > 0)
                    targetFrame = targetFrame % video.totalFrames;
            }
        }

        if (!audioDriven)
        {
            video.elapsedTime += deltaTime;
            if (video.frameDuration > 0.0)
            {
                int framesToAdvance = static_cast<int>(video.elapsedTime / video.frameDuration);
                if (framesToAdvance <= 0)
                    return;
                video.elapsedTime -= static_cast<double>(framesToAdvance) * video.frameDuration;
                targetFrame = video.currentFrameIndex + framesToAdvance;
                if (video.loop && video.totalFrames > 0)
                    targetFrame = targetFrame % video.totalFrames;
            }
        }

        if (!video.preloaded || video.frames.empty())
            return;

        if (!video.loop && targetFrame >= static_cast<int>(video.frames.size()))
        {
            targetFrame = static_cast<int>(video.frames.size()) - 1;
            video.done = true;
            m_isPlaying = false;
        }

        if (targetFrame < 0 || targetFrame >= static_cast<int>(video.frames.size()))
            return;

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            video.currentFrame = video.frames[static_cast<size_t>(targetFrame)];
            video.hasCurrentFrame = true;
            video.currentFrameIndex = targetFrame;
        }
    }

    /**
     * @brief Upload the current frame planes to GPU and draw the quad.
     */
    void VideoManager::RenderVideoFrame(VideoData& video)
    {
        if (!m_videoShader || !m_videoShader->IsValid())
            return;

        if (!video.hasCurrentFrame)
            return;

        auto& frame = video.currentFrame;
        if (!frame.y_buffer || !frame.cb_buffer || !frame.cr_buffer)
            return;

        UpdateQuadForVideo(video);

        glDisable(GL_DEPTH_TEST);

        m_videoShader->Bind();
        m_videoShader->SetUniform1i("tex_y", 0);
        m_videoShader->SetUniform1i("tex_cb", 1);
        m_videoShader->SetUniform1i("tex_cr", 2);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        auto UploadPlane = [](GLuint texture, GLuint pbo, size_t& pboSize, int width, int height, const uint8_t* src)
        {
            const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
            if (pboSize < size)
            {
                glBufferData(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_STREAM_DRAW);
                pboSize = size;
            }

            void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            if (ptr)
            {
                memcpy(ptr, src, size);
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            }
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        };

        const int pboIndex = video.pboIndex;

        glActiveTexture(GL_TEXTURE0);
        UploadPlane(video.tex_y, video.pbo_y[pboIndex], video.pboSizeY, frame.width, frame.height, frame.y_buffer);

        glActiveTexture(GL_TEXTURE1);
        UploadPlane(video.tex_cb, video.pbo_cb[pboIndex], video.pboSizeCb, frame.width / 2, frame.height / 2, frame.cb_buffer);

        glActiveTexture(GL_TEXTURE2);
        UploadPlane(video.tex_cr, video.pbo_cr[pboIndex], video.pboSizeCr, frame.width / 2, frame.height / 2, frame.cr_buffer);

        video.pboIndex = (video.pboIndex + 1) % kPboCount;

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        m_videoShader->Unbind();

        glEnable(GL_DEPTH_TEST);
    }

    /**
     * @brief Update the full-screen quad based on fit mode and aspect ratio.
     */
    void VideoManager::UpdateQuadForVideo(const VideoData& video)
    {
        if (!m_quadDirty || m_screenWidth <= 0 || m_screenHeight <= 0)
            return;

        if (video.width == 0 || video.height == 0)
            return;

        const float screenAspect = static_cast<float>(m_screenWidth) / static_cast<float>(m_screenHeight);
        const float videoAspect = static_cast<float>(video.width) / static_cast<float>(video.height);

        float scaleX = 1.0f;
        float scaleY = 1.0f;

        if (m_fitMode == VideoFitMode::StretchToFill)
        {
            scaleX = 1.0f;
            scaleY = 1.0f;
        }
        else if (videoAspect > screenAspect)
        {
            scaleY = screenAspect / videoAspect;
        }
        else
        {
            scaleX = videoAspect / screenAspect;
        }

        const float left = -scaleX;
        const float right = scaleX;
        const float bottom = -scaleY;
        const float top = scaleY;

        float vertices[] = {
            left,  bottom, 0.0f, 1.0f,
            right, bottom, 1.0f, 1.0f,
            right, top,    1.0f, 0.0f,

            left,  bottom, 0.0f, 1.0f,
            right, top,    1.0f, 0.0f,
            left,  top,    0.0f, 0.0f
        };

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        m_quadDirty = false;
    }

    /**
     * @brief Background decode loop that fills the frame queue.
     */
    void VideoManager::DecodeThreadLoop()
    {
        while (!m_decodeStop)
        {
            std::shared_ptr<VideoData> video;

            {
                std::unique_lock<std::mutex> lock(m_stateMutex);
                video.reset();
                m_decodeCv.wait(lock, [this, &video]()
                {
                    if (m_decodeStop)
                        return true;
                    for (auto& entry : m_videos)
                    {
                        auto& candidate = entry.second;
                        if (!candidate || !candidate->loaded || candidate->stopDecoding)
                            continue;
                        if (!candidate->preloadRequested || candidate->preloaded)
                            continue;
                        video = candidate;
                        return true;
                    }
                    return false;
                });
            }

            if (m_decodeStop)
                break;

            if (!video)
                continue;

            {
                std::lock_guard<std::mutex> decodeLock(video->decodeMutex);
                if (video->stopDecoding || !video->plm)
                    continue;

                ReleasePreloadedFrames(*video);
                video->preloaded = false;
                video->preloadFailed = false;

                while (!video->stopDecoding)
                {
                    plm_frame_t* frame = plm_decode_video(video->plm);
                    if (!frame)
                        break;

                    VideoFrame decoded = AcquireFrameBuffer(frame->y.width, frame->y.height);
                    const size_t y_size = static_cast<size_t>(frame->y.width) * static_cast<size_t>(frame->y.height);
                    const size_t cr_size = static_cast<size_t>(frame->cr.width) * static_cast<size_t>(frame->cr.height);
                    const size_t cb_size = static_cast<size_t>(frame->cb.width) * static_cast<size_t>(frame->cb.height);

                    memcpy(decoded.y_buffer, frame->y.data, y_size);
                    memcpy(decoded.cr_buffer, frame->cr.data, cr_size);
                    memcpy(decoded.cb_buffer, frame->cb.data, cb_size);

                    video->frames.push_back(decoded);
                }

                if (video->audioEnabled && video->audioPlm)
                {
                    std::lock_guard<std::mutex> alock(video->audioMutex);
                    video->audioBuffer.clear();
                    while (!video->stopDecoding)
                    {
                        plm_samples_t* samples = plm_decode_audio(video->audioPlm);
                        if (!samples)
                            break;
                        const size_t sampleCount = static_cast<size_t>(samples->count) * 2;
                        const size_t oldSize = video->audioBuffer.size();
                        video->audioBuffer.resize(oldSize + sampleCount);
                        memcpy(video->audioBuffer.data() + oldSize, samples->interleaved, sampleCount * sizeof(float));
                    }
                    video->audioReadIndex = 0;
                }

                video->totalFrames = static_cast<int>(video->frames.size());
                if (video->totalFrames <= 0)
                {
                    video->preloadFailed = true;
                    video->done = true;
                }
                else
                {
                    video->currentFrameIndex = 0;
                    video->elapsedTime = 0.0;
                    video->currentFrame = video->frames.front();
                    video->hasCurrentFrame = true;
                    video->done = false;
                }

                video->preloaded = true;
                video->preloadRequested = false;

                if (video->plm)
                {
                    plm_destroy(video->plm);
                    video->plm = nullptr;
                }

                if (video->audioPlm)
                {
                    plm_destroy(video->audioPlm);
                    video->audioPlm = nullptr;
                }
            }
        }
    }

    /**
     * @brief Release all GL and CPU resources for a video.
     */
    void VideoManager::ReleaseVideoResources(VideoData& video)
    {
        if (video.plm)
        {
            plm_destroy(video.plm);
            video.plm = nullptr;
        }

        if (video.audioPlm)
        {
            plm_destroy(video.audioPlm);
            video.audioPlm = nullptr;
        }

        if (video.audioChannel)
        {
            video.audioChannel->stop();
            video.audioChannel = nullptr;
        }

        if (video.audioSound)
        {
            video.audioSound->release();
            video.audioSound = nullptr;
        }

        if (video.tex_y)
        {
            glDeleteTextures(1, &video.tex_y);
            video.tex_y = 0;
        }

        if (video.tex_cb)
        {
            glDeleteTextures(1, &video.tex_cb);
            video.tex_cb = 0;
        }

        if (video.tex_cr)
        {
            glDeleteTextures(1, &video.tex_cr);
            video.tex_cr = 0;
        }

        if (video.pbo_y[0] || video.pbo_y[1] || video.pbo_y[2])
        {
            glDeleteBuffers(kPboCount, video.pbo_y);
            glDeleteBuffers(kPboCount, video.pbo_cb);
            glDeleteBuffers(kPboCount, video.pbo_cr);
            memset(video.pbo_y, 0, sizeof(video.pbo_y));
            memset(video.pbo_cb, 0, sizeof(video.pbo_cb));
            memset(video.pbo_cr, 0, sizeof(video.pbo_cr));
        }
        video.pboSizeY = 0;
        video.pboSizeCb = 0;
        video.pboSizeCr = 0;

        ReleasePreloadedFrames(video);
        video.stopDecoding = true;
        {
            std::lock_guard<std::mutex> alock(video.audioMutex);
            video.audioBuffer.clear();
            video.audioReadIndex = 0;
        }
        video.audioEnabled = false;
        video.preloaded = false;
        video.preloadRequested = false;
        video.preloadFailed = false;
        video.loaded = false;
        video.done = false;
    }
}

#pragma endregion
