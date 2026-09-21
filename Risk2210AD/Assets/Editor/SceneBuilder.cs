using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace Risk2210.EditorTools
{
    /// <summary>
    /// Creates MainMenu.unity and MainGame.unity and registers them in Build Settings.
    /// Everything else is assembled at runtime by GameBootstrap / MainMenuController.
    /// </summary>
    public static class SceneBuilder
    {
        private const string MenuPath = "Assets/Scenes/MainMenu.unity";
        private const string GamePath = "Assets/Scenes/MainGame.unity";

        [MenuItem("Risk 2210/Build Scenes")]
        public static void BuildScenes()
        {
            if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
            System.IO.Directory.CreateDirectory("Assets/Scenes");

            var menu = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);
            StyleCamera();
            new GameObject("MainMenu").AddComponent<UI.MainMenuController>();
            EditorSceneManager.SaveScene(menu, MenuPath);

            var game = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);
            StyleCamera();
            new GameObject("Risk2210").AddComponent<GameBootstrap>();
            EditorSceneManager.SaveScene(game, GamePath);

            EditorBuildSettings.scenes = new[]
            {
                new EditorBuildSettingsScene(MenuPath, true),
                new EditorBuildSettingsScene(GamePath, true),
            };
            PlayerSettings.productName = "Risk 2210 A.D.";
            PlayerSettings.colorSpace = ColorSpace.Linear;
            EditorSceneManager.OpenScene(MenuPath);
            Debug.Log("Risk 2210: scenes built and added to Build Settings.");
        }

        private static void StyleCamera()
        {
            var cam = Camera.main;
            if (cam == null) return;
            cam.orthographic = true;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = new Color(0.02f, 0.03f, 0.06f);
            var light = Object.FindFirstObjectByType<Light>();
            if (light != null) Object.DestroyImmediate(light.gameObject);
        }

        [MenuItem("Risk 2210/Play from Main Menu")]
        public static void PlayFromMenu()
        {
            if (!System.IO.File.Exists(MenuPath)) BuildScenes();
            EditorSceneManager.OpenScene(MenuPath);
            EditorApplication.isPlaying = true;
        }
    }
}
