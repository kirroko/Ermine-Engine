using ErmineEngine;
using System;

public class Chase : MonoBehaviour
{
    public string playerName = "Player";

    public float losePlayerDistance = 10.0f;

    public float repathInterval = 0.10f;

    private GameObject playerGO;
    private ulong entityID;

    private bool collidingWithPlayer = false;
    private float repathTimer = 0f;

    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        CachePlayerIfNeeded();
    }

    void Update()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return;

        float dist = (playerGO.transform.position - transform.position).Magnitude;

        // Too far -> back to Test3 (previous state)
        if (dist > losePlayerDistance)
        {
            StateMachine.RequestPreviousState(entityID);
            return;
        }

        // If colliding with player -> go to Attack (next state)
        if (collidingWithPlayer)
        {
            StateMachine.RequestNextState(entityID);
            return;
        }

        // Keep chasing: update destination periodically
        repathTimer -= Time.deltaTime;
        if (repathTimer <= 0f)
        {
            NavAgent.SetDestination(entityID, playerGO.transform.position);
            repathTimer = repathInterval;
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == playerName)
            collidingWithPlayer = true;
    }

    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name == playerName)
            collidingWithPlayer = true;
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == playerName)
            collidingWithPlayer = false;
    }
}

