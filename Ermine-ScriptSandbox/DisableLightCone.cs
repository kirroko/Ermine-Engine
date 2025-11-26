using ErmineEngine;

public class DisableLightCone : MonoBehaviour
{
    public float timer = 3.0f;
    private Vector3 oldPos;
    private Vector3 oldConePos;
    private bool disabled = false;
    private bool orbInside = false;

    private bool currentlyColliding = false;

    private GameObject lightCone = null;

    private void Start()
    {
        oldPos = transform.position;

        // Find matching LightConeX
        //lightCone = FindMatchingCone();
        //if (lightCone != null)
        //    oldConePos = lightCone.transform.position;

        if (transform.childCount > 0)
            lightCone = gameObject.transform.GetChild(0).gameObject;

        if (lightCone != null)
            oldConePos = lightCone.transform.position;
    }

    private void Update()
    {
        currentlyColliding = false;
        // orb burst
        if (!disabled && orbInside && Input.GetMouseButtonDown(0))
        {
            DisableLight();
            GameObject sphere = GameObject.Find("Sphere");
            Physics.RemovePhysic((ulong)sphere.GetInstanceID());
            GameObject.Destroy(sphere);
        }

        // if u want the lightcone to come back after disabling
        if (disabled)
        {
            timer -= Time.deltaTime;

            if (timer <= 0.0f)
            {
                RespawnLight();
            }
        }

        if (!currentlyColliding)
            orbInside = false;
    }

    private void DisableLight()
    {
        disabled = true;
        //gameObject.SetActive(false);
        //gameObject.SetActive(false);
        lightCone.SetActive(false);
        // we move the lightcone out of view for now until SetActive() is implemented
        //gameObject.transform.position = new Vector3(0, oldPos.y + 100, 0);
        //Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);

        //// Disable linked cone too
        //if (lightCone != null)
        //{
        //    lightCone.transform.position = new Vector3(0, oldPos.y + 100, 0);
        //    Physics.SetPosition((ulong)lightCone.GetInstanceID(), lightCone.transform.position);
        //}
    }

    void RespawnLight()
    {
        disabled = false;
        timer = 3.0f;

        //gameObject.SetActive(true);
        lightCone.SetActive(true);

        //// Respawn original light
        //gameObject.transform.position = oldPos;
        //Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);

        //// Respawn cone in original place
        //if (lightCone != null)
        //{
        //    lightCone.transform.position = oldConePos;
        //    Physics.SetPosition((ulong)lightCone.GetInstanceID(), lightCone.transform.position);
        //}
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            orbInside = true;
        }
    }

    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            orbInside = true;
            currentlyColliding = true;
        }
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            orbInside = false;
        }
    }

    /*
    private GameObject FindMatchingCone()
    {
        string name = gameObject.name;
        string number = "";

        // Read digits from the end of the string
        for (int i = name.Length - 1; i >= 0; i--)
        {
            char c = name[i];
            if (c >= '0' && c <= '9')
            {
                number = c + number;
            }
            else
            {
                break; // stop when we hit first non-digit
            }
        }

        if (number == "")
            return null; // no trailing number found

        string coneName = "LightCone" + number;
        return GameObject.Find(coneName);
    }*/
}