#ifndef AUDIOENGINE_H_
#define AUDIOENGINE_H_

#include "fmod_studio.hpp"
#include "fmod.hpp"
#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <iostream>

using namespace std;

namespace Ermine {

    struct Vector3 {
        float x;
        float y;
        float z;
    };

    struct Implementation {
        Implementation();
        ~Implementation();
        void Update();

        FMOD::Studio::System* mpStudioSystem;
        FMOD::System* mpSystem;
        int mnNextChannelId;

        typedef map<string, FMOD::Sound*> SoundMap;
        typedef map<int, FMOD::Channel*> ChannelMap;
        typedef map<string, FMOD::Studio::EventInstance*> EventMap;
        typedef map<string, FMOD::Studio::Bank*> BankMap;

        BankMap mBanks;
        EventMap mEvents;
        SoundMap mSounds;
        ChannelMap mChannels;
    };

    class CAudioEngine {
    public:
        // Core engine functions
        static void Init();
        static void Update();
        static void Shutdown();
        static int ErrorCheck(FMOD_RESULT result);

        // Sound loading and management
        static void LoadSound(const string& strSoundName, bool b3d = true, bool bLooping = false, bool bStream = false);
        static void UnLoadSound(const string& strSoundName);

        // Sound playback and control
        static int PlaySounds(const string& strSoundName, const Vector3& vPos = Vector3{ 0, 0, 0 }, float fVolumedB = 0.0f);
        static void SetChannel3dPosition(int nChannelId, const Vector3& vPosition);
        static void SetChannelVolume(int nChannelId, float fVolumedB);
        static bool IsPlaying(int nChannelId);
        static void StopChannel(int nChannelId);
        static void StopAllChannels();

        // FMOD Studio functions
        static void LoadBank(const std::string& strBankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags);
        static void LoadEvent(const std::string& strEventName);
        static void PlayEvent(const string& strEventName);
        static void StopEvent(const string& strEventName, bool bImmediate = false);
        static bool IsEventPlaying(const string& strEventName);
        static void GetEventParameter(const string& strEventName, const string& strEventParameter, float* parameter);
        static void SetEventParameter(const string& strEventName, const string& strParameterName, float fValue);

        // 3D Audio listener
        static void Set3dListenerAndOrientation(const Vector3& vPosition, const Vector3& vLook, const Vector3& vUp);

        // Utility functions
        static float dbToVolume(float dB);
        static float VolumeTodB(float volume);
        static FMOD_VECTOR VectorToFmod(const Vector3& vPosition);
    };
}
#endif