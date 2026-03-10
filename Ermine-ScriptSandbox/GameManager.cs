using System;
using ErmineEngine;

public class GameManager : MonoBehaviour
{
    public static GameManager I;

    public GameObject player;
    private Vector3 startPos;
    private Vector3 respawnPos;

    void Awake()
    {
        I = this;
    }
    void Start()
    {
        player = GameObject.Find("Player");
        if (player != null)startPos = player.transform.position;
        respawnPos = startPos;
    }

    

    public void PlayerRespawn()
    {
        if (player == null) return;
        player.transform.position = respawnPos;
        Physics.SetPosition((ulong)player.GetInstanceID(), respawnPos);
    }

    public void UpdateRespawnPoint(Vector3 pos)
    {
        respawnPos = pos;
    }
}

