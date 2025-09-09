using ErmineEngine;

public class Sandbox : MonoBehaviour
{
    void Start()
    {
        Debug.Log("ID: " + GetInstanceID());
        this.tag = "Box";
        this.name = "Box2";
        Debug.Log("Name: " + gameObject.name);
        Debug.Log("Tag: " + this.tag);
        Debug.Log("Transform position: " + gameObject.transform.position.ToString());
        Debug.Log("Transform rotation: " + transform.rotation.ToString());
    }

    void Update()
    {
        transform.Rotate(new Vector3(0,10f,0) * Time.deltaTime);
        if (Input.GetKeyDown(KeyCode.S))
        {
            transform.Translate(new Vector3(0f,-10.0f,0f) * Time.deltaTime);
        }
    }
}
