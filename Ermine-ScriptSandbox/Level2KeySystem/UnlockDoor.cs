using ErmineEngine;

public class UnlockDoor : MonoBehaviour
{
    public static UnlockDoor I;

    public float doorOpenSpeed = 2f;
    public float doorOpenDistance = 3f;

    private int numOfKeys = 0;
    private int ComputerUnlocked = 0;

    private Transform doorLeft;
    private Transform doorRight;

    private float leftClosedX;
    private float rightClosedX;

    private float timer = 0f;
    public float openDuration = 2f;

    private bool doorUnlocked = false;

    void Awake()
    {
        I = this;

        GameObject leftObj = GameObject.Find("FinalGateL");
        GameObject rightObj = GameObject.Find("FinalGateR");

        if (leftObj != null && leftObj.transform != null)
        {
            doorLeft = leftObj.transform;
            leftClosedX = doorLeft.position.x;
        }
        else
        {
            Debug.Log("FinalGateL not found in the scene.");
        }

        if (rightObj != null && rightObj.transform != null)
        {
            doorRight = rightObj.transform;
            rightClosedX = doorRight.position.x;
        }
        else
        {
            Debug.Log("FinalGateR not found in the scene.");
        }
    }

    void Update()
    {
        if (!doorUnlocked) return;

        // Extra safety checks
        if (doorLeft == null || doorRight == null)
        {
            Debug.Log("Door transforms missing. Cannot open door.");
            return;
        }

        timer += Time.deltaTime * doorOpenSpeed;

        if (timer <= openDuration)
        {
            float t = timer / openDuration;

            float leftX = Mathf.Lerp(leftClosedX, leftClosedX - doorOpenDistance, t);
            float rightX = Mathf.Lerp(rightClosedX, rightClosedX + doorOpenDistance, t);

            doorLeft.position = new Vector3(
                leftX,
                doorLeft.position.y,
                doorLeft.position.z
            );

            doorRight.position = new Vector3(
                rightX,
                doorRight.position.y,
                doorRight.position.z
            );
        }
        else
        {
            if (doorLeft != null)
            {
                doorLeft.position = new Vector3(
                    leftClosedX - doorOpenDistance,
                    doorLeft.position.y,
                    doorLeft.position.z
                );
            }

            if (doorRight != null)
            {
                doorRight.position = new Vector3(
                    rightClosedX + doorOpenDistance,
                    doorRight.position.y,
                    doorRight.position.z
                );
            }

            doorUnlocked = false;
        }
    }

    public void IncrementKeys() { numOfKeys++; }
    public void DecrementKeys() { numOfKeys--; }

    public void UnlockDoorBool()
    {
        if (doorLeft == null || doorRight == null)
        {
            Debug.Log("Cannot unlock door because door objects are missing.");
            return;
        }

        doorUnlocked = true;
    }

    public void Evaluate()
    {
        ComputerUnlocked++;
        DecrementKeys();

        if (ComputerUnlocked >= 2 && numOfKeys <= 0)
        {
            UnlockDoorBool();
            Debug.Log("Door Unlocked!");

            GameObject hint = GameObject.Find("Hint4");
            GameObject keyHint = GameObject.Find("Hint5");

            if (hint != null && keyHint != null)
            {
                hint.SetActive(false);
                keyHint.SetActive(true);
            }
        }
    }
}