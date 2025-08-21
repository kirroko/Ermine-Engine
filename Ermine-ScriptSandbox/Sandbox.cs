using System;
using ErmineEngine;

public class Sandbox : MonoBehaviour
{
    void Start()
    {
        float dt = Time.deltaTime;
        Debug.Log("Hello world!");
        Debug.Log("ID: " + GetInstanceID());
    }

    void Update()
    {

    }
}
