using ErmineEngine;

public class OrbTeleport : MonoBehaviour
{
    private Transform origin;
    private GameObject hudEntity; // Reference to HUD entity for skill flash effects

    void Start()
    {
        //origin = GameObject.Find("Origin").GetComponent<Transform>();
        origin = GameObject.Find("Player").GetComponent<Transform>();

        // Find HUD entity that has the UIComponent
        hudEntity = GameObject.Find("HUD");
        if (hudEntity == null)
        {
            Debug.LogWarning("OrbTeleport: Cannot find HUD entity! Skill flash effects will not work.");
        }
    }

    void Update()
    {
        // Left Mouse Button - Shoot Orb (Skill Slot 0)
        if (Input.GetMouseButtonDown(0))
        {
            var projectile = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");
            if (projectile != null)
            {
                projectile.transform.position = new Vector3(origin.transform.position.x, 1.5f, origin.transform.position.z);
                projectile.transform.rotation = transform.rotation;
                projectile.GetComponent<Sphere>().direction = -transform.forward;
            }
        }

        // Right Mouse Button - Teleport to Orb (Skill Slot 2)
        if (Input.GetMouseButtonDown(1))
        {
            // Swap position with ball and destroy it
            GameObject sphere = GameObject.Find("Sphere");
            if (sphere == null)
                return;
            transform.position = sphere.transform.position;
            GameObject.Destroy(sphere);
        }
    }
}