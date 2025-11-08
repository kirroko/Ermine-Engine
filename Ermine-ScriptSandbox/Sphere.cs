using ErmineEngine;

public class Sphere : MonoBehaviour
{
    public Vector3 direction;
    public float speed = 2.0f;

    private float timeAlive = 3.0f;

    private void Start()
    {
    }

    private void Update()
    {
        timeAlive -= Time.deltaTime;
        transform.position -= direction * speed * Time.deltaTime;

        if (timeAlive < 0.0f)
            GameObject.Destroy(gameObject);
    }
}