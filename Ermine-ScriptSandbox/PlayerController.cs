using ErmineEngine;

public class PlayerController : MonoBehaviour
{
    void Start()
    {

    }

    void Update()
    {
        if (Input.GetMouseButton(0))
        {
            var projectile = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");
            if (projectile != null)
            {
                projectile.transform.position = transform.position;
                projectile.transform.rotation = transform.rotation;
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