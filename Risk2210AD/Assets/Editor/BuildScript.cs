using System.IO;
using UnityEditor;
using UnityEngine;

namespace Risk2210.EditorTools
{
    /// <summary>Standalone macOS (Apple Silicon) build. `-executeMethod Risk2210.EditorTools.BuildScript.BuildMac`.</summary>
    public static class BuildScript
    {
        [MenuItem("Risk 2210/Build macOS App")]
        public static void BuildMac()
        {
            if (!File.Exists("Assets/Scenes/MainGame.unity")) SceneBuilder.BuildScenes();
            Directory.CreateDirectory("Builds");
            EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTargetGroup.Standalone, BuildTarget.StandaloneOSX);
            UnityEditor.OSXStandalone.UserBuildSettings.architecture = UnityEditor.Build.OSArchitecture.ARM64;
            PlayerSettings.productName = "Risk 2210 A.D.";
            PlayerSettings.companyName = "Risk2210";
            PlayerSettings.SetApplicationIdentifier(BuildTargetGroup.Standalone, "com.risk2210.game");
            PlayerSettings.macRetinaSupport = true;
            var opts = new BuildPlayerOptions
            {
                scenes = new[] { "Assets/Scenes/MainMenu.unity", "Assets/Scenes/MainGame.unity" },
                locationPathName = "Builds/Risk2210AD.app",
                target = BuildTarget.StandaloneOSX,
                options = BuildOptions.None,
            };
            var report = BuildPipeline.BuildPlayer(opts);
            Debug.Log("Build result: " + report.summary.result + " (" + report.summary.totalSize / (1024 * 1024) + " MB, errors " + report.summary.totalErrors + ")");
            if (Application.isBatchMode) EditorApplication.Exit(report.summary.result == UnityEditor.Build.Reporting.BuildResult.Succeeded ? 0 : 1);
        }
    }
}
