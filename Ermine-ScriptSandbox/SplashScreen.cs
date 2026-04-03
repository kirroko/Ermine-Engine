using System;
using ErmineEngine;

public class SplashScreen : MonoBehaviour
{
    public GameObject splash1;
    public GameObject splash2;
    public GameObject splash3;

    public float fadeInDuration = 1.0f;
    public float holdDuration = 1.0f;
    public float fadeOutDuration = 1.0f;

    public float normalSpeed = 1.0f;
    public float fastSpeed = 3.0f;

    private UIImage img1;
    private UIImage img2;
    private UIImage img3;

    private int currentSplashIndex = 0;
    private int phase = 0; // 0 fade in, 1 hold, 2 fade out
    private float timer = 0.0f;
    private bool finished = false;

    void Start()
    {
        splash1 = GameObject.Find("Digipen Logo");
        splash2 = GameObject.Find("Ermine Logo");
        splash3 = GameObject.Find("Game Title");
        img1 = GetImage(splash1, "splash1");
        img2 = GetImage(splash2, "splash2");
        img3 = GetImage(splash3, "splash3");

        if (img1 == null || img2 == null || img3 == null)
        {
            Debug.LogError("SplashScreenSequence: Missing one or more UIImage references.");
            return;
        }

        img1.alpha = 0.0f;
        img2.alpha = 0.0f;
        img3.alpha = 0.0f;
    }

    void Update()
    {
        if (finished)
            return;

        UIImage current = GetCurrentImage();
        if (current == null)
            return;

        float currentSpeed = normalSpeed;

        // Change this line if credits.cs uses another input function
        if (Input.GetKeyDown(KeyCode.Space))
            currentSpeed = fastSpeed;

        timer += Time.deltaTime * currentSpeed;

        Debug.Log($"Current Splash: {currentSplashIndex + 1}, Phase: {phase}, Timer: {timer:F2}, Speed: {currentSpeed}");

        if (phase == 0)
        {
            float alpha = timer / fadeInDuration;
            if (alpha > 1.0f) alpha = 1.0f;

            current.alpha = alpha;

            if (timer >= fadeInDuration)
            {
                timer = 0.0f;
                phase = 1;
            }
        }
        else if (phase == 1)
        {
            current.alpha = 1.0f;

            if (timer >= holdDuration)
            {
                timer = 0.0f;
                phase = 2;
            }
        }
        else if (phase == 2)
        {
            float alpha = 1.0f - (timer / fadeOutDuration);
            if (alpha < 0.0f) alpha = 0.0f;

            current.alpha = alpha;

            if (timer >= fadeOutDuration)
            {
                current.alpha = 0.0f;
                timer = 0.0f;
                phase = 0;
                currentSplashIndex++;

                if (currentSplashIndex >= 3)
                {
                    finished = true;
                    SceneManager.LoadScene("../Resources/Scenes/mainmenu_video_bg.scene");
                }
            }
        }
    }

    private UIImage GetImage(GameObject obj, string label)
    {
        if (obj == null)
        {
            Debug.LogError("SplashScreenSequence: " + label + " is null.");
            return null;
        }

        UIImage image = obj.GetComponent<UIImage>();
        if (image == null)
        {
            Debug.LogError("SplashScreenSequence: UIImage missing on " + label + ".");
            return null;
        }

        return image;
    }

    private UIImage GetCurrentImage()
    {
        if (currentSplashIndex == 0) return img1;
        if (currentSplashIndex == 1) return img2;
        if (currentSplashIndex == 2) return img3;
        return null;
    }
}