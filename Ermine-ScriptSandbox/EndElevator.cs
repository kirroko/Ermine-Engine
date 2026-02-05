using ErmineEngine;

public class EndElevator : MonoBehaviour
{
    private bool playerIn;

    // Elevator movement settings
    public float elevatorMoveSpeed = 2f; // Speed of the elevator's movement
    public float elevatorMoveDuration = 3f; // Duration for the elevator to move downward

    // Door movement settings
    public float doorMoveSpeed = 1f; // Speed of the door's movement
    public float doorMoveDuration = 2f; // Duration for the door to move to the closed position

    private float timer = 0f; // Timer to track movement time
    private Vector3 startPosition;

    // Door variables
    private GameObject elevatorDoor;
    private float doorOpenPos = -6f;
    private float doorClosePos = 8f;
    private bool doorClosed = false; // To track if the door has closed

    public string sceneName = "m4-LEVEL2_AI.scene";

    void OnCollisionEnter(Collision collision)
    {
        // Check if the player collides with the elevator and sets playerIn to true
        Debug.Log("Player entered the elevator");
        playerIn = true;
    }

    void Start()
    {
        startPosition = transform.position; // Store the starting position
        elevatorDoor = GameObject.Find("ElevatorDoor");
    }

    void Update()
    {
        // Start the door closing and then the downward movement when the player enters
        if (playerIn)
        {
            if (!doorClosed)
            {
                // Close the door before moving the elevator
                timer += Time.deltaTime * doorMoveSpeed;

                // If we haven't reached the door close position, move the door
                if (timer <= doorMoveDuration)
                {
                    float doorMovement = Mathf.Lerp(doorOpenPos, doorClosePos, timer / doorMoveDuration);
                    elevatorDoor.transform.position = new Vector3(doorMovement, elevatorDoor.transform.position.y, elevatorDoor.transform.position.z);
                }
                else
                {
                    // Once the door is closed, stop its movement and mark it as closed
                    elevatorDoor.transform.position = new Vector3(doorClosePos, elevatorDoor.transform.position.y, elevatorDoor.transform.position.z);
                    doorClosed = true;
                    timer = 0f; // Reset the timer to start the downward movement
                }
            }
            else
            {
                // Now move the elevator down after the door has closed
                timer += Time.deltaTime * elevatorMoveSpeed;

                // If we haven't exceeded the moveDuration, move downward
                if (timer <= elevatorMoveDuration)
                {
                    float downwardMovement = Mathf.Lerp(0f, -elevatorMoveSpeed * elevatorMoveDuration, timer / elevatorMoveDuration);
                    transform.position = startPosition + new Vector3(0f, downwardMovement, 0f);
                }
                else
                {
                    // Once the timer exceeds moveDuration, stop the downward movement
                    transform.position = startPosition + new Vector3(0f, -elevatorMoveSpeed * elevatorMoveDuration, 0f);

                    // Load the next scene after the elevator movement is complete
                    LoadNextScene();
                }
            }
        }
    }


    void LoadNextScene()
    {
        SceneManager.LoadScene($"../Resources/Scenes/{sceneName}");
    }
}
