using System;
using ErmineEngine;

public class SpotLightTrigger : MonoBehaviour
{
    public float radius = 18.09f;          // your Radius
    public float innerAngleDeg = 13.3f;    // your Inner Angle
    public float outerAngleDeg = 37.0f;    // your Outer Angle

    public bool useOcclusionRaycast = false;

    public float tickInterval = 1f;
    public float damagePerTick = 10f;

    public string healthBarName = "Healthbar";

    private GameObject player;
    private bool playerInside = false;

    private GameObject healthBar;
    private float timer;
    private float health = 0f;
    float origRegenRate;
    float regenRate;


    private bool cancelRegenInSpotlight = true;




    // Damage feedback vignette
    private bool damageVignetteActive = false;
    private float damageVignetteElapsed = 0f;

    public float damageVignetteDuration = 0.9f;   // total duration
    public float damageVignetteIntensity = 0.75f;
    public float damageVignetteCoverage = 0.4f;
    public float damageVignetteRadius = 0.6f;
    public float damageVignetteFalloff = 0.3f;

    private bool prevVignetteEnabled = false;
    private float prevVignetteIntensity = 0f;
    private float prevVignetteRadius = 0f;
    private float prevVignetteCoverage = 0f;
    private float prevVignetteFalloff = 0f;
    private Vector3 prevVignetteRGBModifier = Vector3.zero;

    private bool vignetteExitFading = false;
    private float vignetteExitElapsed = 0f;
    public float vignetteExitFadeDuration = 0.5f;  // fade-out duration when exiting

    // ===== Base (outside spotlight) =====
    public float baseChromaticAberration = 0.003f;
    public float baseExposure = 1f;
    public float baseGamma = 2.2f;
    public float baseBloomStrength = 0.03f;

    // ===== Spotlight (inside) =====
    public float spotlightChromaticAberration = 0.3f;
    public float spotlightExposure = 3f;
    public float spotlightGamma = 3f;
    public float spotlightBloomStrength = 0.4f;

    private bool spotFxFading = false;
    private bool spotFxFadeIn = false;
    private float spotFxElapsed = 0f;
    public float spotFxDuration = 1f;

    // Start values (where the fade begins)
    private float startChromaticAberration;
    private float startExposure;
    private float startGamma;
    private float startBloom;

    private float currentChromaticAberration;
    private float currentExposure;
    private float currentGamma;
    private float currentBloom;




    void Start()
    {
        player = GameObject.Find("Player");

        healthBar = GameObject.Find(healthBarName);
        if (healthBar != null)
        {
            health = GameplayHUD.GetHealth(healthBar);
        }

        timer = 0;


        currentChromaticAberration = baseChromaticAberration;
        currentExposure = baseExposure;
        currentGamma = baseGamma;
        currentBloom = baseBloomStrength;

        ApplyPostEffects();

        origRegenRate = GameplayHUD.GetRegenRate(healthBar);
    }
    void Update()
    {
        if (damageVignetteActive)
        {
            UpdateDamageVignette();
        }
        if (vignetteExitFading)
        {
            UpdateVignetteExitFade();
        }

        if (spotFxFading)
        {
            UpdateSpotlightFade();
        }

        if (player == null) return;
        if (Physics.Internal_GetLightValue((ulong)gameObject.GetInstanceID()) == 0) return;

        

        bool inside = IsPointInsideSpot(player.transform.position);

        // Optional occlusion check (wall blocks spotlight)
        if (inside && useOcclusionRaycast)
        {
            inside = HasLineOfSight(player.transform.position);
        }

        // Enter / Exit events
        if (inside && !playerInside)
        {
            playerInside = true;
            OnSpotEnter();
        }
        else if (!inside && playerInside)
        {
            playerInside = false;
            OnSpotExit();
            timer = 0;
        }

        // Damage tick while inside
        if (playerInside)
        {

            CancelHealthRegen(); // Cancel regen every frame while inside

            timer -= Time.deltaTime;
            if (timer <= 0f)
            {
                TakeDamage(damagePerTick);
                timer = tickInterval;
            }

            

            // Optional: use intensity (0..1) to scale damage, audio, etc.
            // float intensity01 = GetSpotIntensity01(player.transform.position);
            // TakeDamage(damagePerTick * intensity01);  // example
        }
    }

    void TakeDamage(float dmg)
    {
        if (healthBar == null) return;

        health = GameplayHUD.GetHealth(healthBar);
        health = Math.Max(0f, health - dmg);
        GameplayHUD.SetHealth(healthBar, health);

        // Trigger red vignette damage feedback
        TriggerDamageVignette();
    }

    void TriggerDamageVignette()
    {
        // Cache current vignette state
        prevVignetteEnabled = PostEffects.EnableVignette;
        prevVignetteIntensity = PostEffects.VignetteIntensity;
        prevVignetteRadius = PostEffects.VignetteRadius;
        prevVignetteCoverage = PostEffects.VignetteCoverage;
        prevVignetteFalloff = PostEffects.VignetteFalloff;
        prevVignetteRGBModifier = PostEffects.VignetteMapRGBModifier;

        // Enable vignette, but do NOT snap to full damage values
        PostEffects.EnableVignette = true;
        PostEffects.VignetteMapRGBModifier = new Vector3(1.0f, 0.0f, 0.0f);

        damageVignetteElapsed = 0f;
        damageVignetteActive = true;
    }

    void UpdateDamageVignette()
    {
        float duration = Math.Max(0.0001f, damageVignetteDuration);
        damageVignetteElapsed += Time.deltaTime;

        float t = Math.Min(damageVignetteElapsed / duration, 1.0f);

        // Split into fade-in then fade-out
        float half = 0.5f;
        float blend;
        if (t < half)
    {
        // First half: fade in
        blend = t / half;

        PostEffects.VignetteIntensity =
            prevVignetteIntensity + (damageVignetteIntensity - prevVignetteIntensity) * blend;
        PostEffects.VignetteCoverage =
            prevVignetteCoverage + (damageVignetteCoverage - prevVignetteCoverage) * blend;
        PostEffects.VignetteRadius =
            prevVignetteRadius + (damageVignetteRadius - prevVignetteRadius) * blend;
        PostEffects.VignetteFalloff =
            prevVignetteFalloff + (damageVignetteFalloff - prevVignetteFalloff) * blend;
    }
    else
    {
        // Second half: fade out
        blend = (t - half) / half;

        PostEffects.VignetteIntensity =
            damageVignetteIntensity + (prevVignetteIntensity - damageVignetteIntensity) * blend;
        PostEffects.VignetteCoverage =
            damageVignetteCoverage + (prevVignetteCoverage - damageVignetteCoverage) * blend;
        PostEffects.VignetteRadius =
            damageVignetteRadius + (prevVignetteRadius - damageVignetteRadius) * blend;
        PostEffects.VignetteFalloff =
            damageVignetteFalloff + (prevVignetteFalloff - damageVignetteFalloff) * blend;
    }


        PostEffects.VignetteMapRGBModifier = new Vector3(1.0f, 0.0f, 0.0f); //vignette stays red

        /*
        // Fade from red back to original color
        Vector3 redTint = new Vector3(1.0f, 0.0f, 0.0f);
        PostEffects.VignetteMapRGBModifier = new Vector3(
            redTint.x + (prevVignetteRGBModifier.x - redTint.x) * t,
            redTint.y + (prevVignetteRGBModifier.y - redTint.y) * t,
            redTint.z + (prevVignetteRGBModifier.z - redTint.z) * t
        );
        */

        if (t >= 1.0f)
        {
            // If player is still in the spotlight, repeat the vignette effect
            if (playerInside)
            {
                damageVignetteElapsed = 0f;
            }
            else
            {
                // Restore previous vignette state
                PostEffects.EnableVignette = prevVignetteEnabled;
                PostEffects.VignetteIntensity = prevVignetteIntensity;
                PostEffects.VignetteRadius = prevVignetteRadius;
                PostEffects.VignetteCoverage = prevVignetteCoverage;
                PostEffects.VignetteFalloff = prevVignetteFalloff;
                PostEffects.VignetteMapRGBModifier = prevVignetteRGBModifier;
                damageVignetteActive = false;
            }
        }
    }

    bool IsPointInsideSpot(Vector3 point)
    {
        Vector3 lightPos = transform.worldPosition;

        Vector3 toPoint = point - lightPos;
        float distSq = toPoint.x * toPoint.x + toPoint.y * toPoint.y + toPoint.z * toPoint.z;

        if (distSq > radius * radius)
            return false;

        float dist = Mathf.Sqrt(distSq);
        if (dist < 1e-5f)
            return true;

        Vector3 dirToPoint = toPoint / dist;

        // Forward direction from rotation
        Vector3 forward = transform.worldRotation * new Vector3(0f, 0f, 1f);

        float cos = Vector3.Dot(forward.normalized, dirToPoint); // [-1..1]

        float halfOuterRad = (outerAngleDeg * 0.5f) * Mathf.Deg2Rad;
        float cosOuter = Mathf.Cos(halfOuterRad);

        return cos >= cosOuter;
    }

    float GetSpotIntensity01(Vector3 point)
    {
        Vector3 lightPos = transform.worldPosition;
        Vector3 toPoint = point - lightPos;

        float dist = Mathf.Sqrt(toPoint.x * toPoint.x + toPoint.y * toPoint.y + toPoint.z * toPoint.z);
        if (dist < 1e-5f) return 1f;

        Vector3 dirToPoint = toPoint / dist;
        Vector3 forward = (transform.worldRotation * new Vector3(0f, 0f, 1f)).normalized;

        float cos = Vector3.Dot(forward, dirToPoint);

        float cosInner = Mathf.Cos((innerAngleDeg * 0.5f) * Mathf.Deg2Rad);
        float cosOuter = Mathf.Cos((outerAngleDeg * 0.5f) * Mathf.Deg2Rad);

        if (cos >= cosInner) return 1f;
        if (cos <= cosOuter) return 0f;

        float t = (cos - cosOuter) / (cosInner - cosOuter);
        return t;
    }

    bool HasLineOfSight(Vector3 targetPos)
    {
        Vector3 origin = transform.worldPosition;
        Vector3 dir = targetPos - origin;
        float dist = Mathf.Sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (dist < 1e-5f) return true;

        dir = dir / dist;

        // If/when your engine exposes Physics.Raycast(origin, dir, dist):
        // return !Physics.Raycast(origin, dir, dist);

        return true;
    }

    void OnSpotEnter()
    {
        Debug.Log("Player entered spotlight cone");
        GlobalAudio.PlaySFX("LightDamageLoop");
        StartSpotlightFade(true); // fade IN
    }

    void OnSpotExit()
    {
        Debug.Log("Player left spotlight cone");
        GlobalAudio.StopSFX("LightDamageLoop");
        StartSpotlightFade(false); // fade OUT
        GameplayHUD.SetRegenRate(healthBar, origRegenRate); // Restore regen when exiting
                                                            // Start vignette fade-out
        StartVignetteExitFade();
    }

    void CancelHealthRegen()
    {
        if (healthBar == null) return;

        regenRate = GameplayHUD.GetRegenRate(healthBar);
        if (regenRate <= 0f) return;

        GameplayHUD.SetRegenRate(healthBar, 0f);
    }

    void StartSpotlightFade(bool fadeIn)
    {
        // Capture CURRENT values (we can't read engine state, so track what we last set)
        startChromaticAberration = currentChromaticAberration;
        startExposure = currentExposure;
        startGamma = currentGamma;
        startBloom = currentBloom;

        spotFxElapsed = 0f;
        spotFxFadeIn = fadeIn;
        spotFxFading = true;
    }

    void ApplyPostEffects()
    {
        PostEffects.ChromaticAberrationIntensity = currentChromaticAberration;
        PostEffects.Exposure = currentExposure;
        PostEffects.Gamma = currentGamma;
        PostEffects.BloomStrength = currentBloom;
    }

    void UpdateSpotlightFade()
    {
        float duration = Math.Max(0.0001f, spotFxDuration);
        spotFxElapsed += Time.deltaTime;

        float t = Math.Min(spotFxElapsed / duration, 1.0f);

        // Choose target
        float targetChromatic = spotFxFadeIn ? spotlightChromaticAberration : baseChromaticAberration;
        float targetExposure = spotFxFadeIn ? spotlightExposure : baseExposure;
        float targetGamma = spotFxFadeIn ? spotlightGamma : baseGamma;
        float targetBloom = spotFxFadeIn ? spotlightBloomStrength : baseBloomStrength;

        // Lerp
        currentChromaticAberration = startChromaticAberration + (targetChromatic - startChromaticAberration) * t;
        currentExposure = startExposure + (targetExposure - startExposure) * t;
        currentGamma = startGamma + (targetGamma - startGamma) * t;
        currentBloom = startBloom + (targetBloom - startBloom) * t;

        ApplyPostEffects();

        if (t >= 1.0f)
        {
            currentChromaticAberration = targetChromatic;
            currentExposure = targetExposure;
            currentGamma = targetGamma;
            currentBloom = targetBloom;

            ApplyPostEffects();
            spotFxFading = false;
        }
    }

    void StartVignetteExitFade()
    {
        damageVignetteActive = false;
        vignetteExitFading = true;
        vignetteExitElapsed = 0f;
    }

    void UpdateVignetteExitFade()
    {
        float duration = Math.Max(0.0001f, vignetteExitFadeDuration);
        vignetteExitElapsed += Time.deltaTime;

        float t = Math.Min(vignetteExitElapsed / duration, 1.0f);

        // Fade out vignette properties
        PostEffects.VignetteIntensity = prevVignetteIntensity + (0f - prevVignetteIntensity) * t;
        PostEffects.VignetteCoverage = prevVignetteCoverage + (0f - prevVignetteCoverage) * t;
        PostEffects.VignetteRadius = prevVignetteRadius + (0f - prevVignetteRadius) * t;
        PostEffects.VignetteFalloff = prevVignetteFalloff + (0f - prevVignetteFalloff) * t;

        if (t >= 1.0f)
        {
            // Restore previous vignette state
            PostEffects.EnableVignette = prevVignetteEnabled;
            PostEffects.VignetteIntensity = prevVignetteIntensity;
            PostEffects.VignetteRadius = prevVignetteRadius;
            PostEffects.VignetteCoverage = prevVignetteCoverage;
            PostEffects.VignetteFalloff = prevVignetteFalloff;
            PostEffects.VignetteMapRGBModifier = prevVignetteRGBModifier;
            vignetteExitFading = false;
        }
    }
}