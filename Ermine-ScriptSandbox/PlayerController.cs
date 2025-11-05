using ErmineEngine;

public class PlayerController : MonoBehaviour
{
    public Vector3 origin = new Vector3(0, 2, 0);

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
                projectile.transform.position = transform.position + origin;
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