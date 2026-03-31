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

    // Damage feedback vignette
    private bool damageVignetteActive = false;
    private float damageVignetteElapsed = 0f;
    private const float damageVignetteDuration = 0.4f;
    private const float damageVignetteIntensity = 0.75f;
    private const float damageVignetteCoverage = 0.4f;
    private bool prevVignetteEnabled = false;
    private float prevVignetteIntensity = 0f;
    private float prevVignetteRadius = 0f;
    private float prevVignetteCoverage = 0f;
    private float prevVignetteFalloff = 0f;
    private Vector3 prevVignetteRGBModifier = Vector3.zero;

    void Start()
    {
        player = GameObject.Find("Player");

        healthBar = GameObject.Find(healthBarName);
        if (healthBar != null)
        {
            health = GameplayHUD.GetHealth(healthBar);
        }

        timer = 0;
    }
    void Update()
    {
        if (damageVignetteActive)
        {
            UpdateDamageVignette();
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

        // Apply red damage vignette
        PostEffects.EnableVignette = true;
        PostEffects.VignetteIntensity = damageVignetteIntensity;
        PostEffects.VignetteCoverage = damageVignetteCoverage;
        PostEffects.VignetteRadius = 0.6f;
        PostEffects.VignetteFalloff = 0.3f;
        PostEffects.VignetteMapRGBModifier = new Vector3(1.0f, 0.0f, 0.0f); // Red tint

        damageVignetteElapsed = 0f;
        damageVignetteActive = true;
    }

    void UpdateDamageVignette()
    {
        float duration = Math.Max(0.0001f, damageVignetteDuration);
        damageVignetteElapsed += Time.deltaTime;

        float t = Math.Min(damageVignetteElapsed / duration, 1.0f);
        
        // Fade out damage vignette
        PostEffects.VignetteIntensity = damageVignetteIntensity + (prevVignetteIntensity - damageVignetteIntensity) * t;
        PostEffects.VignetteCoverage = damageVignetteCoverage + (prevVignetteCoverage - damageVignetteCoverage) * t;
        PostEffects.VignetteRadius = 0.6f + (prevVignetteRadius - 0.6f) * t;
        PostEffects.VignetteFalloff = 0.3f + (prevVignetteFalloff - 0.3f) * t;

        // Fade from red back to original color
        Vector3 redTint = new Vector3(1.0f, 0.0f, 0.0f);
        PostEffects.VignetteMapRGBModifier = new Vector3(
            redTint.x + (prevVignetteRGBModifier.x - redTint.x) * t,
            redTint.y + (prevVignetteRGBModifier.y - redTint.y) * t,
            redTint.z + (prevVignetteRGBModifier.z - redTint.z) * t
        );

        if (t >= 1.0f)
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
    }

    void OnSpotExit()
    {
        Debug.Log("Player left spotlight cone");
        GlobalAudio.StopSFX("LightDamageLoop");
    }
}