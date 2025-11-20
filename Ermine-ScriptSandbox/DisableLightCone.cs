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
        // if u want the lightcone to come back after disabling
        if (hit)
        {
            timer -= Time.deltaTime;

            if (timer <= 0.0f)
            {
                gameObject.transform.position = oldPos;
                Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);
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

            // we move the lightcone out of view for now until SetActive() is implemented
            gameObject.transform.position = new Vector3(0, oldPos.y + 100, 0);
            Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);
        }
    }
}