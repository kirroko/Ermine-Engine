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
        startPos = player.transform.position;
        respawnPos = startPos;
    }

    void Update()
    {
        //if (player.transform.position.y < -ff)
        //{
        //    player.transform.position = startPos;   
        //}
    }

    public void PlayerRespawn()
    {
        player.transform.position = respawnPos;
        Physics.SetPosition((ulong)player.GetInstanceID(), respawnPos);
    }

    public void UpdateRespawnPoint(Vector3 pos)
    {
        respawnPos = pos;
    }
}

