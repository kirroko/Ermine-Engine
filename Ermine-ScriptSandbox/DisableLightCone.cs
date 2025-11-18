using ErmineEngine;

public class DisableLightCone : MonoBehaviour
{
    private void Start()
    {
    }

    private void Update()
    {
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Sphere")
        {
            //gameObject.SetActive(false);
            gameObject.transform.position = new Vector3(0, 30, 0);
            Physics.SetPosition((ulong)gameObject.GetInstanceID(), gameObject.transform.position);
        }
    }
}