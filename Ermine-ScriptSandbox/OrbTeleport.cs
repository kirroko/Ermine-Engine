using ErmineEngine;
using System;
using System.Threading;

public class OrbTeleport : MonoBehaviour
{
    private Transform origin;
    private Transform cam;
    private float health = 0f;
    public float damage = 10f;

    void Start()
    {
        //origin = GameObject.Find("Origin").GetComponent<Transform>();
        origin = GameObject.Find("Player").GetComponent<Transform>();
        cam = GameObject.Find("Main Camera").transform;
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
    }

    void Update()
    {
        if (Input.GetMouseButton(0))
        {
            TakeDamage(damage);
            GlobalAudio.PlaySFX("Shoot");
            var projectile = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");
            if (projectile != null)
            {
                projectile.transform.position = new Vector3(origin.transform.position.x, origin.transform.position.y + 1.5f, origin.transform.position.z);
                projectile.transform.rotation = transform.rotation;
                projectile.GetComponent<Sphere>().direction = -cam.forward;
            }
        }

        if (Input.GetMouseButton(1))
        {
            // Swap position with ball and destroy it
            GlobalAudio.PlaySFX("Teleport");
            GameObject sphere = GameObject.Find("Sphere");
            if (sphere == null)
                return;
            Physics.SetPosition((ulong)gameObject.GetInstanceID(), sphere.transform.position);
            GameObject.Destroy(sphere);
        }
    }

    void TakeDamage(float dmg)
    {
        health = Math.Max(0, health - dmg);

        GameObject bar = GameplayHUD.GetHealthBar();
        GameplayHUD.SetHealth(bar, health);
    }
}