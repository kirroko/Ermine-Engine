using System;
using ErmineEngine;

public class RotatingPlatform : MonoBehaviour
{
    public float speed = 2.0f;
    public bool active = false;

    void Start()
    {
        Debug.Log("Hello World");
    }

    void Update()
    {
        if (active)
        {
            transform.Rotate(Vector3.up * (Time.deltaTime * speed));
        }
    }

    public void IsActive(bool state)
    {
        active = state;
    }
}

