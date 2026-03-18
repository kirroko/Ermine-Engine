using ErmineEngine;
using System;

public class Idle : MonoBehaviour
{
    public string playerName = "Player";

    public float viewDistance = 15.0f;
    public float rayHeight = 5.5f;
    public float rayForwardOffset = 2.0f;
    public float closeDetectDistance = 2.0f;

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

    private Animator anim;
    public float stunRecoverDelay = 8.0f;
    private float recoverTimer = 0.0f;

    // STUN FEEDBACK
    private GameObject stunVFX;
    private string stunPrefabPath = "../Resources/Prefabs/EnemyStunSpark.prefab";

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

        if (stunVFX != null)
        {
            Debug.Log("IdleState: Activating Stun VFX");
            stunVFX.SetActive(true);
            stunVFX.transform.position = transform.position + new Vector3(0, 1.0f, 0);
        }

        if (anim != null)
        {
            anim.SetBool("IsHit", true);
        }

        // stop immediately while stunned
        NavAgent.SetDestination(entityID, transform.position);
    }

    private bool HasLineOfSightToPlayer()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return false;

        Vector3 enemyPos = transform.position;
        Vector3 playerPoint = playerGO.transform.position;

        // fallback for very close targets on tiny platforms
        Vector3 flatToPlayer = playerPoint - enemyPos;
        flatToPlayer.y = 0f;
        if (flatToPlayer.Magnitude <= closeDetectDistance)
            return true;

        Vector3 origin = enemyPos
                       + new Vector3(0f, rayHeight, 0f)
                       + transform.forward * rayForwardOffset;

        Vector3 toPlayer = playerPoint - origin;

        float dist = toPlayer.Magnitude;
        if (dist <= 0.0001f) return true;
        if (dist > viewDistance) return false;

        Vector3 dirToPlayer = toPlayer / dist;

        RaycastHit hit;
        bool didHit = Physics.Raycast(origin, dirToPlayer, out hit, dist);
        if (!didHit) return false;

                var hitGO = hit.transform.gameObject;

        // Ignore self-hit
        ulong hitID = (ulong)hitGO.GetInstanceID();
        if (hitID == entityID) return false;

        string n = hitGO.name;
        if (n.StartsWith("SpawnPoint_")) return false;

        return hit.transform != null &&
               hit.transform.gameObject != null &&
               hit.transform.gameObject.name == playerName;
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        anim = GetComponent<Animator>();
        CachePlayerIfNeeded();

        // Instantiate Stun VFX
        stunVFX = Prefab.Instantiate(stunPrefabPath);
        if (stunVFX != null)
        {
            stunVFX.SetActive(false);
        }
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(1))
            armTimer = 0.3f;

        if (armTimer > 0.0f)
            armTimer -= Time.deltaTime;

        RightClickStunArmed = armTimer > 0f;

        if (jumpCooldownTimer > 0.0f)
            jumpCooldownTimer -= Time.deltaTime;

        if (isStunned)
        {
            if (anim != null)
            {
                anim.SetBool("IsHit", true);
            }

            // Keep VFX attached
            if (stunVFX != null && stunVFX.activeSelf)
            {
                stunVFX.transform.position = transform.position + new Vector3(0, 1.0f, 0);
            }

            //Debug.Log("stunned");
            stunTimer -= Time.deltaTime;
            if (stunTimer <= 0.0f)
            {
                isStunned = false;
                recoverTimer = stunRecoverDelay;

                if (stunVFX != null) stunVFX.SetActive(false);

                if (anim != null)
                    anim.SetBool("IsHit", false);
            }
            return; // do NOTHING while stunned
        }
        if (recoverTimer > 0.0f)
        {
            recoverTimer -= Time.deltaTime;
            return;
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
