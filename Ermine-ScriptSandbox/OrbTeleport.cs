using ErmineEngine;
using System;
using System.Threading;

public class OrbTeleport : MonoBehaviour
{
    private Transform origin;
    private Transform cam;
    private float health = 0f;
    public float damage = 10f;
    public float recallHealAmt = 10f;

    private float forwardOffset = 2.0f; // Distance in front of the player
    private float rightOffset = -0.3f;  // Slightly to the right
    private float upOffset = 4.2f;      // Above the player

    private bool orbShot = false;       // Tracks if we already shot an orb

    // Name of the entity with UISkillsComponent (must match your scene)
    public string skillsHUDName = "Skills";

    // Name of the entity with UIHealthbarComponent (must match your scene)
    public string healthBarName = "Healthbar";

    // Skill indices - match your UISkillsComponent skill slot order (0-based)
    // Slot 1 = Shoot (index 0)
    // Slot 2 = Return (index 1)
    // Slot 3 = Teleport (index 2)
    // Slot 4 = Disrupt (index 3)
    private const int SKILL_SHOOT = 0;
    private const int SKILL_RETURN = 1;
    private const int SKILL_TELEPORT = 2;
    private const int SKILL_DISRUPT = 3;

    //Health
    private GameObject healthBar;

    void Start()
    {
        origin = GameObject.Find("Player").GetComponent<Transform>();
        cam = GameObject.Find("Main Camera").transform;

        // Find healthbar by name
        healthBar = GameObject.Find(healthBarName);
        if (healthBar != null)
        {
            health = GameplayHUD.GetHealth(healthBar);
        }

        // Initialize: Shoot is ready (lit up), others are not
        SetSkillsForReadyToShoot();
    }

    // Helper: Set skills to "ready to shoot" state (no orb out)
    void SetSkillsForReadyToShoot()
    {
        UISystem.SetSkillSelected(skillsHUDName, SKILL_SHOOT, true);     // Shoot ready
        UISystem.SetSkillSelected(skillsHUDName, SKILL_RETURN, false);   // Return not ready
        UISystem.SetSkillSelected(skillsHUDName, SKILL_TELEPORT, false); // Teleport not ready
        UISystem.SetSkillSelected(skillsHUDName, SKILL_DISRUPT, false);  // Disrupt not ready (for now)
    }

    // Helper: Set skills to "orb is out" state (ready to teleport or recall)
    void SetSkillsForOrbOut()
    {
        UISystem.SetSkillSelected(skillsHUDName, SKILL_SHOOT, false);    // Shoot not ready (already shot)
        UISystem.SetSkillSelected(skillsHUDName, SKILL_RETURN, true);    // Return ready
        UISystem.SetSkillSelected(skillsHUDName, SKILL_TELEPORT, true);  // Teleport ready
        UISystem.SetSkillSelected(skillsHUDName, SKILL_DISRUPT, false);  // Disrupt not ready (for now)
    }

    void Update()
    {
        // Check if orb disappeared on its own (hit something, traveled too far, etc.)
        if (orbShot && GameObject.Find("Sphere") == null)
        {
            orbShot = false;
            SetSkillsForReadyToShoot();
        }

        // Regenerate health when orb is not out
        if (!orbShot)
        {
            RegenerateHealth();
        }

        if (Input.GetMouseButton(0))
        {
            if (!orbShot)
            {
                // First left click - shoot orb
                ShootOrb();
                orbShot = true;
                return;
            }

            // Second left click - teleport to orb
            TeleportToOrb();
            orbShot = false;
        }

        // Recall orb on 'R' key press
        if (Input.GetKeyDown(KeyCode.R))
            RecallOrb();
    }

    void RegenerateHealth()
    {
        float currentHealth = GameplayHUD.GetHealth(healthBar);
        float maxHealth = GameplayHUD.GetMaxHealth(healthBar);
        float regenRate = GameplayHUD.GetRegenRate(healthBar);

        if (currentHealth < maxHealth && regenRate > 0)
        {
            float newHealth = Math.Min(currentHealth + regenRate * Time.deltaTime, maxHealth);
            GameplayHUD.SetHealth(healthBar, newHealth);
        }
    }

    void ShootOrb()
    {
        // Only one orb at a time
        if (GameObject.Find("Sphere") != null) return;

        // Deal damage to player
        TakeDamage(damage);
        GlobalAudio.PlaySFX("Shoot");

        // Update UI: Orb is out, teleport and return are now ready
        SetSkillsForOrbOut();

        // Instantiate orb projectile
        var projectile = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");

        if (projectile != null)
        {
            projectile.transform.position = origin.transform.position + cam.forward * forwardOffset + cam.right * rightOffset + Vector3.up * upOffset;
            projectile.transform.rotation = transform.rotation;
            projectile.GetComponent<Sphere>().direction = -cam.forward;
        }
    }

    void TeleportToOrb()
    {
        // Find the orb
        GameObject sphere = GameObject.Find("Sphere");
        if (sphere == null)
        {
            orbShot = false;
            SetSkillsForReadyToShoot();
            return;
        }

        GlobalAudio.PlaySFX("Teleport");

        // Swap positions
        gameObject.transform.position = sphere.transform.position;
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), sphere.transform.position);

        // Remove orb
        Physics.RemovePhysic((ulong)sphere.GetInstanceID());
        GameObject.Destroy(sphere);

        // Back to ready to shoot
        SetSkillsForReadyToShoot();
    }

    void RecallOrb()
    {
        // Find the orb
        GameObject sphere = GameObject.Find("Sphere");
        if (sphere != null)
        {
            GlobalAudio.PlaySFX("Teleport"); // Or a custom recall sound

            // Remove orb
            Physics.RemovePhysic((ulong)sphere.GetInstanceID());
            HealDamage(recallHealAmt);
            GameObject.Destroy(sphere);

            // Back to ready to shoot
            SetSkillsForReadyToShoot();
        }

        // Reset state fully
        orbShot = false;
    }

    void TakeDamage(float dmg)
    {
        health = GameplayHUD.GetHealth(healthBar);
        health = Math.Max(0, health - dmg);


        GameplayHUD.SetHealth(healthBar, health);
    }

    void HealDamage(float heal)
    {
        health = GameplayHUD.GetHealth(healthBar);
        GameplayHUD.SetHealth(healthBar, health + heal);
    }
}