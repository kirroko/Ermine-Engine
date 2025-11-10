using ErmineEngine;

public class OrbTeleport : MonoBehaviour
{
    private Transform origin;

    void Start()
    {
        //origin = GameObject.Find("Origin").GetComponent<Transform>();
        origin = GameObject.Find("Player").GetComponent<Transform>();
    }

    void Update()
    {
        if (Input.GetMouseButton(0))
        {
            var projectile = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");
            if (projectile != null)
            {
                projectile.transform.position = new Vector3(origin.transform.position.x, 1.5f, origin.transform.position.z);
                projectile.transform.rotation = transform.rotation;
                projectile.GetComponent<Sphere>().direction = -transform.forward;
            }
        }

        if (Input.GetMouseButton(1))
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