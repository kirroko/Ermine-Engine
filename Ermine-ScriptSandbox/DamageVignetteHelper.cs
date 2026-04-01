using ErmineEngine;
using System;

// Simple static helper with fade effect
public static class DamageVignetteHelper
{
    private static float timeSinceLastDamage = 999f;
    private static float vignetteTimeout = 0.15f;
    private static float fadeDuration = 0.4f; // Fade out over 0.4 seconds
    
    // Call this when damage is dealt (not every frame - only on damage tick)
    public static void FlashRedVignette()
    {
        timeSinceLastDamage = 0f;
    }
    
    // Call this every frame from PlayerController
    public static void UpdateVignette()
    {
        timeSinceLastDamage += Time.deltaTime;
        
        if (timeSinceLastDamage > vignetteTimeout + fadeDuration)
        {
            // Fully cleared
            PostEffects.EnableVignette = false;
            PostEffects.VignetteIntensity = 0f;
            PostEffects.VignetteMapRGBModifier = new Vector3(0.0f, 0.0f, 0.0f);
        }
        else if (timeSinceLastDamage > vignetteTimeout)
        {
            // Fading out
            float fadeProgress = (timeSinceLastDamage - vignetteTimeout) / fadeDuration;
            float t = Math.Min(fadeProgress, 1.0f);
            
            PostEffects.EnableVignette = true;
            PostEffects.VignetteIntensity = 0.75f * (1.0f - t);
            PostEffects.VignetteCoverage = 0.5f * (1.0f - t);
            PostEffects.VignetteRadius = 0.5f + 0.5f * t;
            PostEffects.VignetteFalloff = 0.4f + 0.6f * t;
            PostEffects.VignetteMapRGBModifier = new Vector3(1.0f - t, 0.0f, 0.0f);
        }
        else
        {
            // Active damage - full red vignette
            PostEffects.EnableVignette = true;
            PostEffects.VignetteIntensity = 0.75f;
            PostEffects.VignetteCoverage = 0.5f;
            PostEffects.VignetteRadius = 0.5f;
            PostEffects.VignetteFalloff = 0.4f;
            PostEffects.VignetteMapRGBModifier = new Vector3(1.0f, 0.0f, 0.0f);
        }
    }
}
