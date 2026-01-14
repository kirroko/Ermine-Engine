using System;
using ErmineEngine;

public class Test3 : MonoBehaviour
{
    public float radius = 5.0f;          // How wide the patrol circle is
    public float speed = 2.0f;           // How fast to move between points
    public int numPoints = 8;            // Number of patrol points in the circle
    public float waitTime = 1.0f;        // Pause time at each point

    private Vector3[] patrolPoints;
    private int currentIndex = 0;
    private float waitTimer = 0f;
    private bool waiting = false;

    void Start()
    {
        // Generate circular patrol points around the starting position
        Vector3 center = transform.position;
        patrolPoints = new Vector3[numPoints];

        for (int i = 0; i < numPoints; i++)
        {
            float angle = (float)(i * 2 * Math.PI / numPoints);
            float x = center.x + radius * (float)Math.Cos(angle);
            float z = center.z + radius * (float)Math.Sin(angle);
            patrolPoints[i] = new Vector3(x, center.y, z);
        }

        // Start moving toward the first point
        MoveToNextPoint();
    }

    void Update()
    {
        if (waiting)
        {
            waitTimer += Time.deltaTime;
            if (waitTimer >= waitTime)
            {
                waiting = false;
                MoveToNextPoint();
            }
            return;
        }

        // Check if we’re close to our current target
        Vector3 currentPos = transform.position;
        Vector3 target = patrolPoints[currentIndex];
        float distance = (target - currentPos).Magnitude;

        if (distance < 0.5f)
        {
            waiting = true;
            waitTimer = 0f;
        }
    }

    private void MoveToNextPoint()
    {
        // Go to next waypoint in the circle
        currentIndex = (currentIndex + 1) % patrolPoints.Length;

        // Ask the NavMeshAgent to move there
        NavAgent.SetDestination((ulong)gameObject.GetInstanceID(), patrolPoints[currentIndex]);
    }
}
