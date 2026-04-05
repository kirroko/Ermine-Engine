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
    public bool playerInside = false;

    private GameObject healthBar;
    private float timer;
    private float health = 0f;

    private float regenRate;

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

    private PlayerController2 playerController2;
    private float playerPrevSpeed;
    private float playerPrevSprintSpeed;
    private float slowMoveSpeed = 4f;
    private float slowSprintSpeed = 4f;

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

        regenRate = GameplayHUD.GetRegenRate(healthBar);

        ApplyPostEffects();

        playerController2 = player?.GetComponent<PlayerController2>();
        Debug.LogError("Playercontroller2 found: " + (playerController2 != null));

        if (playerController2 != null)
        {
            playerPrevSpeed = playerController2.walkSpeed;
            playerPrevSprintSpeed = playerController2.sprintSpeed;
        }
    }
    
    void Update()
    {
        if (spotFxFading)
        {
            UpdateSpotlightFade();
        }

        if (player == null) return;
        if (Physics.Internal_GetLightValue((ulong)gameObject.GetInstanceID()) == 0)
        {
            playerInside = false;
            OnSpotExit();
            return;
        }

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
            timer -= Time.deltaTime;
            if (timer <= 0f)
            {
                TakeDamage(damagePerTick);
                timer = tickInterval;
            }
        }
    }

    void TakeDamage(float dmg)
    {
        if (healthBar == null) return;

        health = GameplayHUD.GetHealth(healthBar);
        health = Math.Max(0f, health - dmg);
        GameplayHUD.SetHealth(healthBar, health);
        GameplayHUD.SetRegenRate(healthBar, 0f);
        // Flash red vignette on damage
        DamageVignetteHelper.FlashRedVignette();
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

        return true;
    }

    void OnSpotEnter()
    {
        Debug.Log("Player entered spotlight cone");
        GlobalAudio.PlaySFX("LightDamageLoop");
        StartSpotlightFade(true); // fade IN

        if (playerController2 != null)
        {
            playerController2.walkSpeed = slowMoveSpeed;
            playerController2.sprintSpeed = slowSprintSpeed;
        }
        Debug.Log("PlayerController speed  = " + playerController2.moveSpeed);
    }

    public void OnSpotExit()
    {
        Debug.Log("Player left spotlight cone");
        GlobalAudio.StopSFX("LightDamageLoop");
        StartSpotlightFade(false); // fade OUT
        GameplayHUD.SetRegenRate(healthBar, regenRate);

        if (playerController2 != null)
        {
            playerController2.walkSpeed = playerPrevSpeed;
            playerController2.sprintSpeed = playerPrevSprintSpeed;
        }
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
}