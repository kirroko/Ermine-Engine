using ErmineEngine;

public class SpawnPointController : MonoBehaviour
{
    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Player")
        {
            GameManager.I.UpdateRespawnPoint(transform.position);
        }
    }
}