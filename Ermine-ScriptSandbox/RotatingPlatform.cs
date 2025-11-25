using System;
using ErmineEngine;

public class RotatingPlatform : MonoBehaviour
{
    public float speed = 2.0f;
    public bool active = true;

    private Rigidbody rb;

    void Start()
    {
        rb = GetComponent<Rigidbody>();
    }

    void Update()
    {
        if (!active) return;

        // Create a rotation for this frame
        //Quaternion delta = Quaternion.Euler(0f, speed * Time.deltaTime, 0f);

        // Apply rotation manually (like MoveRotation)
        transform.Rotate(new Vector3(0f, speed * Time.deltaTime, 0f));
    }

    public void IsActive(bool state)
    {
        active = state;
    }
}

