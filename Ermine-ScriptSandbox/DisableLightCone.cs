using ErmineEngine;

public class DisableLightCone : MonoBehaviour
{
    public float timer = 3.0f;
    private Vector3 oldPos;
    private bool hit = false;

    private void Start()
    {
        oldPos = transform.position;
    }

    private void Update()
    {
        if (hit)
        {
            timer -= Time.deltaTime;

            if (timer <= 0.0f)
            {
                gameObject.transform.position = oldPos;
                timer = 3.0f;
                hit = false;
            }
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            hit = true;
            //gameObject.SetActive(false);
            gameObject.transform.position = new Vector3(0, 30, 0);
            Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);
        }
    }
}