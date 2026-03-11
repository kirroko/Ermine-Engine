using ErmineEngine;
using System.Resources;

public class CutsceneVideo: MonoBehaviour
{
    public string nextSceneName = "../Resources/Scenes/m4-test_copy_copy.scene";

    // TEMP FIX PARAMETER (set video duration here in seconds)
    public float videoDurationSeconds = 16f;


    public string VideoFileName = "intro_cinematic.mpeg";

    private bool finished = false;
    private float elapsedTime = 0f;

    private GameObject blackScreen;

    void Start()
    {
        
        if (VideoManager.Load(VideoFileName, "../Resources/Videos/" + VideoFileName, false))
        {
            VideoManager.SetCurrent(VideoFileName);
            VideoManager.SetFitMode(VideoFitMode.StretchToFill);
            VideoManager.SetRenderEnabled(true);
            VideoManager.Play();
        }
        else
        {
            Debug.LogError("CutsceneVideo: Failed to load video '" + VideoFileName + "'.");
        }

        blackScreen = transform.GetChild(0).gameObject;
    }

    void Update()
    {
        if (finished) return;

        elapsedTime += Time.deltaTime;


        if (elapsedTime >= videoDurationSeconds)
        {
            Debug.Log("[CutsceneVideo] Timeout reached (" + videoDurationSeconds + "s)");

            finished = true;
            blackScreen.SetActive(true);
            VideoManager.Stop();
            VideoManager.Free(VideoFileName);
            SceneManager.LoadScene(nextSceneName);
            return;
        }

        bool done = !VideoManager.IsPlaying();
        
        

        if (done)
        {
            Debug.Log("[CutsceneVideo] Video finished!");

            finished = true;

            Debug.Log("[CutsceneVideo] Stopping video");
            VideoManager.Stop();

            Debug.Log("[CutsceneVideo] Freeing video");
            VideoManager.Free(VideoFileName);

            Debug.Log("[CutsceneVideo] Loading next scene: " + nextSceneName);
            SceneManager.LoadScene(nextSceneName);
        }
    }

    void OnDestroy()
    {
        VideoManager.Stop();
        VideoManager.Free(VideoFileName);
    }
}
