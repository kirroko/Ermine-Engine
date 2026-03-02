/* Start Header ************************************************************************/
/*!
\file       PostEffects.cs
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       25/02/2026
\brief      This file contains APIs related to post-processing effects in the Ermine Engine.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public static class PostEffects
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetExposure(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetContrast(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetSaturation(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetGamma(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetVignetteIntensity(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetVignetteRadius(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetBloomStrength(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetGrainIntensity(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetGrainSize(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetChromaticAberrationIntensity(float value);


        public static float Exposure
        {
            set => SetExposure(value);
        }

        public static float Contrast
        {
            set => SetContrast(value);
        }

        public static float Saturation
        {
            set => SetSaturation(value);
        }

        public static float Gamma
        {
            set => SetGamma(value);
        }

        public static float VignetteIntensity
        {
            set => SetVignetteIntensity(value);
        }

        public static float VignetteRadius
        {
            set => SetVignetteRadius(value);
        }

        public static float BloomStrength
        {
            set => SetBloomStrength(value);
        }

        public static float GrainIntensity
        {
            set => SetGrainIntensity(value);
        }

        public static float GrainSize
        {
            set => SetGrainSize(value);
        }

        public static float ChromaticAberrationIntensity
        {
            set => SetChromaticAberrationIntensity(value);
        }
    }
}
