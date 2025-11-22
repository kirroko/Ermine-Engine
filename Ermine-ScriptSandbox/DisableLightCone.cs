using ErmineEngine;

public class DisableLightCone : MonoBehaviour
{
    public float timer = 3.0f;
    private Vector3 oldPos;
    private bool disabled = false;
    private bool orbInside = false;

    private void Start()
    {
        oldPos = transform.position;
    }

    private void Update()
    {
        // orb burst
        if (!disabled && orbInside && Input.GetMouseButtonDown(0))
        {
            DisableCone();
        }

        // if u want the lightcone to come back after disabling
        if (disabled)
        {
            timer -= Time.deltaTime;

            if (timer <= 0.0f)
            {
                gameObject.transform.position = oldPos;
                Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);
                timer = 3.0f;
                disabled = false;
            }
        }
    }

    private void DisableCone()
    {
        disabled = true;
        //gameObject.SetActive(false);

        // we move the lightcone out of view for now until SetActive() is implemented
        gameObject.transform.position = new Vector3(0, oldPos.y + 100, 0);
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            orbInside = true;
        }
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            orbInside = false;
        }
    }
}