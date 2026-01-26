using ErmineEngine;
using System;

public class Test3 : MonoBehaviour
{
    public float radius = 5f;
    public int pointCount = 16;
    public float reachDist = 0.5f;

    // If we’re not getting closer for this long, we give up and pick the next point.
    public float stuckTime = 0.75f;
    public float minProgressEpsilon = 0.02f;

    private Vector3[] patrolPoints;
    private int currentIndex = -1;

    private float stuckTimer = 0f;
    private float lastDist = float.MaxValue;

    private ulong entityID;

    private bool jumping = false;
    private ulong jumpLinkEntityID = 0;

    private void MoveToNextPoint()
    {
        if (patrolPoints == null || patrolPoints.Length == 0)
            return;

        currentIndex = (currentIndex + 1) % patrolPoints.Length;

        stuckTimer = 0f;
        lastDist = float.MaxValue;

        NavAgent.SetDestination(entityID, patrolPoints[currentIndex]);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();

        // Build patrol points around the spawn position
        Vector3 center = transform.position;
        patrolPoints = new Vector3[pointCount];

        for (int i = 0; i < pointCount; i++)
        {
            float t = (float)i / (float)pointCount;
            float ang = t * 6.28318530718f; // 2*pi

            patrolPoints[i] = new Vector3(
                center.x + (float)Math.Cos(ang) * radius,
                center.y,
                center.z + (float)Math.Sin(ang) * radius
            );
        }

        MoveToNextPoint(); // start moving
    }

    void Update()
    {
        if (jumping)
        {
            Debug.Log("CALL StartJump: me=" + entityID + " link=" + jumpLinkEntityID);
            NavAgent.StartJump(entityID, jumpLinkEntityID);

            jumping = false;
            jumpLinkEntityID = 0; // clear after use
            return;
        }

        if (patrolPoints == null || patrolPoints.Length == 0)
            return;

        Vector3 pos = transform.position;
        Vector3 target = patrolPoints[currentIndex];

        float dist = (target - pos).Magnitude;

        // Reached target -> go next
        if (dist <= reachDist)
        {
            MoveToNextPoint();
            return;
        }

        // Stuck detection: if we’re not getting closer, count time
        if (dist >= lastDist - minProgressEpsilon)
            stuckTimer += Time.deltaTime;
        else
            stuckTimer = 0f;

        lastDist = dist;

        if (stuckTimer >= stuckTime)
        {
            // Give up on this point and try the next one
            MoveToNextPoint();
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (jumping) return;

        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }
    }

    //void OnCollisionStay(Collision col)
    //{
    //    if (jumping) return;

    //    if (col.gameObject.name == "JumpArea")
    //    {
    //        jumping = true;
    //        jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
    //    }
    //}

    //void OnCollisionExit(Collision col)
    //{
    //    if (jumping) return;

    //    if (col.gameObject.name == "JumpArea")
    //    {
    //        jumping = true;
    //        jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
    //    }
    //}
}

