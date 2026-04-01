using ErmineEngine;
using System;

// Enhanced damage feedback with multiple effects
public static class DamageVignetteHelper
{
    private static float timeSinceLastDamage = 999f;
    private static float fadeInDuration = 0.15f;   // Fade in over 0.15s
    private static float holdDuration = 0.25f;     // Hold at full for 0.25s
    private static float fadeOutDuration = 0.6f;   // Fade out over 0.6s
    
    // Effect intensities
    private static float maxVignetteIntensity = 0.7f;
    private static float maxChromaticAberration = 0.015f;
    private static float minSaturation = 0.5f;     // Desaturate to 50%
    
    // Store original values
    private static bool hasCachedOriginals = false;
    private static float originalSaturation = 1.0f;
    
    // Call this when damage is dealt
    public static void FlashRedVignette()
    {
        if (!hasCachedOriginals)
        {
            originalSaturation = 1.0f;
            hasCachedOriginals = true;
        }
        
        timeSinceLastDamage = 0f;
    }
    
    // Call this every frame from PlayerController
    public static void UpdateVignette()
    {
        timeSinceLastDamage += Time.deltaTime;
        
        float fadeInEnd = fadeInDuration;
        float holdEnd = fadeInEnd + holdDuration;
        float totalDuration = holdEnd + fadeOutDuration;
        
        if (timeSinceLastDamage > totalDuration)
        {
            // Fully cleared
            PostEffects.EnableVignette = false;
            PostEffects.VignetteIntensity = 0f;
            PostEffects.VignetteMapRGBModifier = new Vector3(0.0f, 0.0f, 0.0f);
            PostEffects.EnableChromaticAberration = false;
            PostEffects.ChromaticAberrationIntensity = 0f;
            PostEffects.Saturation = originalSaturation;
        }
        else if (timeSinceLastDamage > holdEnd)
        {
            // Fading out phase
            float fadeProgress = (timeSinceLastDamage - holdEnd) / fadeOutDuration;
            float t = Math.Min(fadeProgress, 1.0f);
            // Smooth ease out (cubic)
            float easeOut = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            float intensity = 1.0f - easeOut;
            
            ApplyEffects(intensity);
        }
        else if (timeSinceLastDamage > fadeInEnd)
        {
            // Hold at full intensity
            ApplyEffects(1.0f);
        }
        else
        {
            // Fading in phase
            float fadeProgress = timeSinceLastDamage / fadeInDuration;
            float t = Math.Min(fadeProgress, 1.0f);
            // Smooth ease in (quadratic)
            float easeIn = t * t;
            
            ApplyEffects(easeIn);
        }
    }
    
    private static void ApplyEffects(float intensity)
    {
        // Vignette
        PostEffects.EnableVignette = true;
        PostEffects.VignetteIntensity = maxVignetteIntensity * intensity;
        PostEffects.VignetteCoverage = 0.45f * intensity;
        PostEffects.VignetteRadius = 0.5f + 0.5f * (1.0f - intensity);
        PostEffects.VignetteFalloff = 0.35f + 0.65f * (1.0f - intensity);
        PostEffects.VignetteMapRGBModifier = new Vector3(intensity, 0.0f, 0.0f);
        
        // Chromatic aberration
        PostEffects.EnableChromaticAberration = intensity > 0.01f;
        PostEffects.ChromaticAberrationIntensity = maxChromaticAberration * intensity;
        
        // Desaturation
        float currentSat = originalSaturation - (originalSaturation - minSaturation) * intensity;
        PostEffects.Saturation = currentSat;
    }
}
