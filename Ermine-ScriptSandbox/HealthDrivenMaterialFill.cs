using System;
using ErmineEngine;

public class HealthDrivenMaterialFill : MonoBehaviour
{
    public string healthBarName = "Healthbar";
    public bool enableDebugLogs = true;

    private Material targetMaterial;
    private GameObject healthSource;
    private float targetFill = 1.0f;
    private bool subscribed = false;

    void Start()
    {
        ResolveTargetMaterial();
        ResolveHealthSource();
        SubscribeHealthEvent();
        SyncFromCurrentHealth(true);
        LogDebug("Start complete");
    }

    void OnEnable()
    {
        SubscribeHealthEvent();
    }

    void OnDisable()
    {
        UnsubscribeHealthEvent();
    }

    void OnDestroy()
    {
        UnsubscribeHealthEvent();
    }

    private void ResolveTargetMaterial()
    {
        if (gameObject != null)
            targetMaterial = gameObject.GetComponent<Material>();

        if (targetMaterial == null)
            Debug.LogWarning("HealthDrivenMaterialFill: attached entity has no Material component.");
        else
        {
            targetFill = targetMaterial.fill;
        }
    }

    private void ResolveHealthSource()
    {
        if (!string.IsNullOrEmpty(healthBarName))
            healthSource = GameObject.Find(healthBarName);

        if (healthSource == null)
            healthSource = GameplayHUD.GetHealthBar();
    }

    private void SubscribeHealthEvent()
    {
        if (subscribed) return;
        GameplayHUD.HealthChanged += OnHealthChanged;
        subscribed = true;
    }

    private void UnsubscribeHealthEvent()
    {
        if (!subscribed) return;
        GameplayHUD.HealthChanged -= OnHealthChanged;
        subscribed = false;
    }

    private void SyncFromCurrentHealth(bool immediate)
    {
        if (healthSource == null) return;
        float h = GameplayHUD.GetHealth(healthSource);
        float max = GameplayHUD.GetMaxHealth(healthSource);
        ApplyHealth(h, max, immediate);
    }

    private void OnHealthChanged(GameObject source, float health, float maxHealth)
    {
        if (healthSource == null || source == null) return;
        if (source.GetInstanceID() != healthSource.GetInstanceID())
        {
            return;
        }
        ApplyHealth(health, maxHealth, false);
    }

    private void ApplyHealth(float health, float maxHealth, bool immediate)
    {
        if (targetMaterial == null) return;

        float ratio = (maxHealth > 0.0f) ? (health / maxHealth) : 0.0f;
        ratio = Mathf.Clamp(ratio, 0.0f, 1.0f);
        targetFill = ratio;
        LogDebug("ApplyHealth health={0}, max={1}, ratio={2}, immediate={3}", health, maxHealth, ratio, immediate);
        targetMaterial.fill = targetFill;
        LogDebug("Set material.fill={0}", targetFill);
    }

    private void LogDebug(string format, params object[] args)
    {
        if (!enableDebugLogs) return;
        Debug.Log("[HealthDrivenMaterialFill] " + string.Format(format, args));
    }
}
