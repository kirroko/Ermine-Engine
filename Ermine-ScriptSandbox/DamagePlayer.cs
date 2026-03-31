using ErmineEngine;
using System;

public class DamagePlayer : MonoBehaviour
{
    private float health = 0f;
    public float timer = 1f;
    public float damage = 10f;
    private bool playerInside = false;

    // Name of the entity with UIHealthbarComponent (must match your scene)
    public string healthBarName = "Healthbar";

    //Health
    private GameObject healthBar;

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

    private void Start()
    {
        // Find healthbar by name
        healthBar = GameObject.Find(healthBarName);
        if (healthBar != null)
        {
            health = GameplayHUD.GetHealth(healthBar);
        }
    }

    private void Update()
    {
        if (damageVignetteActive)
        {
            UpdateDamageVignette();
        }

        if (playerInside)
        {
            timer -= Time.deltaTime;

            if (timer <= 0f)
            {
                TakeDamage(damage);
                timer = 1f;
            }
        }
    }

    void TakeDamage(float dmg)
    {
        if (healthBar == null) return;

        health = GameplayHUD.GetHealth(healthBar);
        health = Math.Max(0, health - dmg);

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

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Player")
            playerInside = true;
    }
    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name == "Player")
            playerInside = true;
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == "Player")
        {
            playerInside = false;
            timer = 1f; // Reset timer when leaving
        }
    }
}