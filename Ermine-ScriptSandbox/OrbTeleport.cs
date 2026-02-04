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
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
        healthBar = GameplayHUD.GetHealthBar();

        // Initialize: no skill selected at start
        UISystem.SelectOnlySkill(skillsHUDName, -1);
    }

    void Update()
    {
        // Check if orb disappeared on its own (hit something, traveled too far, etc.)
        if (orbShot && GameObject.Find("Sphere") == null)
        {
            orbShot = false;
            UISystem.SelectOnlySkill(skillsHUDName, -1);  // Deselect all
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

    void ShootOrb()
    {
        // Only one orb at a time
        if (GameObject.Find("Sphere") != null) return;

        // Deal damage to player
        TakeDamage(damage);
        GlobalAudio.PlaySFX("Shoot");

        // Update UI: Shoot skill is now selected (orb is out, ready to teleport)
        UISystem.SelectOnlySkill(skillsHUDName, SKILL_SHOOT);

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
            UISystem.SelectOnlySkill(skillsHUDName, -1);
            return;
        }

        GlobalAudio.PlaySFX("Teleport");

        // Update UI: Teleport skill is now selected (teleporting)
        UISystem.SelectOnlySkill(skillsHUDName, SKILL_TELEPORT);

        // Swap positions
        gameObject.transform.position = sphere.transform.position;
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), sphere.transform.position);

        // Remove orb
        Physics.RemovePhysic((ulong)sphere.GetInstanceID());
        GameObject.Destroy(sphere);

        // Note: SKILL_TELEPORT stays selected briefly until next Update() detects orb is gone
    }

    void RecallOrb()
    {
        // Find the orb
        GameObject sphere = GameObject.Find("Sphere");
        if (sphere != null)
        {
            GlobalAudio.PlaySFX("Teleport"); // Or a custom recall sound

            // Update UI: Return skill is now selected
            UISystem.SelectOnlySkill(skillsHUDName, SKILL_RETURN);

            // Remove orb
            Physics.RemovePhysic((ulong)sphere.GetInstanceID());
            HealDamage(recallHealAmt);
            GameObject.Destroy(sphere);
        }

        // Reset state fully
        orbShot = false;

        // Note: SKILL_RETURN stays selected briefly until next Update() detects orb is gone
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