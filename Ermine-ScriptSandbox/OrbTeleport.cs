using ErmineEngine;
using System;
using System.Threading;

public class OrbTeleport : MonoBehaviour
{
    private Transform origin;
    private Transform cam;
    private float health = 0f;
    public float damage = 10f;

    private float forwardOffset = 2.0f; // Distance in front of the player
    private float rightOffset = -0.3f;  // Slightly to the right
    private float upOffset = 4.2f;      // Above the player

    private bool orbShot = false;       // Tracks if we already shot an orb

    void Start()
    {
        origin = GameObject.Find("Player").GetComponent<Transform>();
        cam = GameObject.Find("Main Camera").transform;
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
    }

    void Update()
    {
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
            return;
        }

        GlobalAudio.PlaySFX("Teleport");

        // Swap positions
        gameObject.transform.position = sphere.transform.position;
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), sphere.transform.position);

        // Remove orb
        Physics.RemovePhysic((ulong)sphere.GetInstanceID());
        GameObject.Destroy(sphere);
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
            GameObject.Destroy(sphere);
        }

        // Reset state fully
        orbShot = false;
    }

    void TakeDamage(float dmg)
    {
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
        health = Math.Max(0, health - dmg);

        GameObject bar = GameplayHUD.GetHealthBar();
        GameplayHUD.SetHealth(bar, health);
    }
}