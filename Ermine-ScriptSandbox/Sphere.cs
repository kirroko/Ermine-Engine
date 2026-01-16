using ErmineEngine;

public class Sphere : MonoBehaviour
{
    public Vector3 direction;
    public float speed = 10.0f;

    private float timeAlive = 3.0f;

    private void Start()
    {
    }

    private void Update()
    {
        timeAlive -= Time.deltaTime;
        transform.position -= direction * speed * Time.deltaTime;

        Physics.SetPosition((ulong)gameObject.GetInstanceID(), transform.position);
        if (timeAlive < 0.0f)
        {
            Physics.RemovePhysic((ulong)gameObject.GetInstanceID());
            GameObject.Destroy(gameObject);
        }

    }

    void OnCollisionEnter(Collision col)
    {
        /*
        Debug.Log("Yes me lord? : " + gameObject.name);

        Debug.Log("Jobs done : " + col.gameObject.name);
        if(col.gameObject.name != "Player")
        {
            col.gameObject.transform.position = new Vector3(0, 30, 0);

            Physics.SetPosition((ulong)col.gameObject.GetInstanceID(), col.gameObject.transform.position);
        }
        */
    }
}