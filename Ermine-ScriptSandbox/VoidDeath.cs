using ErmineEngine;

public class VoidDeath : MonoBehaviour
{
    void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.name == "player col")
        {
            GameManager.I.PlayerRespawn();
        }
    }
}