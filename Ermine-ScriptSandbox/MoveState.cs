using ErmineEngine;
using System;

public class Move : MonoBehaviour
{
    public float stepDistance = 3.0f;
    public float reachDist = 0.6f;

    public float rayDistance = 0.8f;
    public float rayHeight = 0.8f;
    public float rayForwardOffset = 2.0f;

    public float turnCooldown = 0.5f; // prevents spam turning
    public float repathInterval = 0.25f; // prevents spamming SetDestination

    private ulong entityID;

    private Vector3 dir;
    private Vector3 target;

    private float turnTimer = 0f;
    private float repathTimer = 0f;

    private bool jumping = false;
    private ulong jumpLinkEntityID = 0;

    // stun guard
    public float stunDuration = 5.0f;
    private bool isStunned = false;
    private float stunTimer = 0.0f;
    public static bool RightClickStunArmed = false;
    private float armTimer = 0.0f;

    private void TryStun()
    {
        if (isStunned)
            return;

        isStunned = true;
        stunTimer = stunDuration;

        // stop immediately while stunned
        NavAgent.SetDestination(entityID, transform.position);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();

        Vector3 f = transform.forward;
        if (Math.Abs(f.x) >= Math.Abs(f.z))
            dir = new Vector3(f.x >= 0 ? 1f : -1f, 0f, 0f);
        else
            dir = new Vector3(0f, 0f, f.z >= 0 ? 1f : -1f);

        FaceDir();
        PushTargetForward(true);
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(1))
            armTimer = 0.3f;

        if (armTimer > 0.0f)
            armTimer -= Time.deltaTime;

        RightClickStunArmed = armTimer > 0f;

        if (isStunned)
        {
            //Debug.Log("stunned");
            stunTimer -= Time.deltaTime;
            if (stunTimer <= 0.0f)
            {
                isStunned = false;
            }
            return; // do NOTHING while stunned
        }

        if (jumping)
        {
            //Debug.Log("CALL StartJump: me=" + entityID + " link=" + jumpLinkEntityID);
            NavAgent.StartJump(entityID, jumpLinkEntityID);

            jumping = false;
            jumpLinkEntityID = 0;

            return;
        }

        if (turnTimer > 0f) turnTimer -= Time.deltaTime;
        if (repathTimer > 0f) repathTimer -= Time.deltaTime;

        // only check turning if cooldown is over
        if (turnTimer <= 0f && HitsSomethingInFront())
        {
            TurnAround();
            FaceDir();
            PushTargetForward(true);

            turnTimer = turnCooldown;  // lock turning for a moment
            return;
        }

        float dist = (target - transform.position).Magnitude;

        if (dist <= reachDist)
        {
            PushTargetForward(true);
        }
        else if (repathTimer <= 0f)
        {
            PushTargetForward(false);
        }
    }

    private bool HitsSomethingInFront()
    {
        Vector3 origin = transform.position
                       + new Vector3(0f, rayHeight, 0f)
                       + dir * rayForwardOffset;

        RaycastHit hit;
        bool didHit = Physics.Raycast(origin, dir, out hit, rayDistance);

        if (!didHit)
            return false;
        else
            Debug.Log(hit.transform.gameObject.name);

        // Ignore self-hit
        ulong hitID = (ulong)hit.transform.gameObject.GetInstanceID();
        if (hitID == entityID) return false;

        // Debug.Log("Hit: " + hit.transform.gameObject.name);
        return true;
    }

    private void PushTargetForward(bool force)
    {
        if (!force && repathTimer > 0f) return;

        Vector3 pos = transform.position;
        target = new Vector3(
            pos.x + dir.x * stepDistance,
            pos.y,
            pos.z + dir.z * stepDistance
        );

        NavAgent.SetDestination(entityID, target);
        repathTimer = repathInterval;
    }

    private void TurnAround()
    {
        dir = new Vector3(-dir.x, 0f, -dir.z);
    }

    private void FaceDir()
    {
        float yaw = (dir.z > 0) ? 0f :
                    (dir.z < 0) ? 180f :
                    (dir.x > 0) ? 90f : 270f;

        Vector3 rot = transform.rotation.eulerAngles;
        rot.y = yaw;
        transform.rotation = Quaternion.Euler(rot.x, rot.y, rot.z);
    }

    void OnCollisionEnter(Collision col)
    {
        if (jumping) return;
        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }

        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
        }
    }

    void OnCollisionStay(Collision col)
    {
        if (jumping) return;
        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }

        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
        }
    }

    void OnCollisionExit(Collision col)
    {
        if (jumping) return;
        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }
    }
}
