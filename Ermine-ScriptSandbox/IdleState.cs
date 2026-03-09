using ErmineEngine;
using System;

public class Idle : MonoBehaviour
{
    public string playerName = "Player";

    public float viewDistance = 15.0f;
    public float rayHeight = 0.8f;
    public float rayForwardOffset = 2.0f;

    private GameObject playerGO;
    private ulong entityID;

    private ulong jumpLinkEntityID = 0;
    private bool insideJumpArea = false;

    public float jumpCooldown = 3.0f;
    private float jumpCooldownTimer = 0.0f;

    public float stunDuration = 5.0f;
    private bool isStunned = false;
    private float stunTimer = 0.0f;
    public static bool RightClickStunArmed = false;
    private float armTimer = 0.0f;

    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    private void TryStun()
    {
        if (isStunned)
            return;

        isStunned = true;
        stunTimer = stunDuration;

        // stop immediately while stunned
        NavAgent.SetDestination(entityID, transform.position);
    }

    private bool HasLineOfSightToPlayer()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return false;

        Vector3 origin = transform.position
                       + new Vector3(0f, rayHeight, 0f)
                       + transform.forward * rayForwardOffset;

        Vector3 playerPoint = playerGO.transform.position;
        Vector3 toPlayer = playerPoint - origin;

        float dist = toPlayer.Magnitude;
        if (dist <= 0.0001f) return true;
        if (dist > viewDistance) return false;

        Vector3 dirToPlayer = toPlayer / dist;

        RaycastHit hit;
        bool didHit = Physics.Raycast(origin, dirToPlayer, out hit, dist);
        if (!didHit) return false;

        return hit.transform != null &&
               hit.transform.gameObject != null &&
               hit.transform.gameObject.name == playerName;
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        CachePlayerIfNeeded();
    }

    void Update()
    {
        if (jumpCooldownTimer > 0.0f)
            jumpCooldownTimer -= Time.deltaTime;

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

        // If player is visible, switch state
        if (HasLineOfSightToPlayer())
        {
            StateMachine.RequestNextState(entityID);
            return;
        }

        // If standing in jump area, jump
        if (insideJumpArea && jumpCooldownTimer <= 0.0f)
        {
            NavAgent.StartJump(entityID, jumpLinkEntityID);
            jumpCooldownTimer = jumpCooldown;
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "JumpArea")
        {
            insideJumpArea = true;
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
        if (col.gameObject.name == "JumpArea")
        {
            insideJumpArea = true;
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
        if (col.gameObject.name == "JumpArea")
        {
            if (jumpLinkEntityID == (ulong)col.gameObject.GetInstanceID())
            {
                insideJumpArea = false;
                jumpLinkEntityID = 0;
            }
        }
        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
        }
    }
}