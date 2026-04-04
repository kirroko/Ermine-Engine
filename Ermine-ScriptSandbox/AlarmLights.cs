using System;
using ErmineEngine;

public class AlarmLights : MonoBehaviour
{
    public float speed = .8f;
    public bool active = true;

    void Update()
    {
        if (!active) return;

        // Rotate platform
        transform.Rotate(new Vector3(0f, speed * Time.deltaTime, 0f));

        // Sync with physics
        Physics.SetRotationQuat(
            (ulong)gameObject.GetInstanceID(),
            transform.rotation
        );
    }

    void ActivateLights()
    {

    }

    void ActivateAlarm()
    {

    }
}
