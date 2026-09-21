using UnityEngine;

namespace Risk2210.View
{
    /// <summary>Cheap bloom for the Built-in Render Pipeline: bright-pass, two blur passes, additive composite.</summary>
    [RequireComponent(typeof(Camera))]
    public sealed class BloomEffect : MonoBehaviour
    {
        [Range(0, 3)] public float Intensity = 1.3f;
        [Range(0, 1)] public float Threshold = 0.45f;
        [Range(1, 4)] public int Downsample = 2;
        public bool Scanlines;

        private Material mat;

        private void OnEnable()
        {
            var shader = Shader.Find("Risk2210/Bloom");
            if (shader != null) mat = new Material(shader);
        }

        private void OnRenderImage(RenderTexture src, RenderTexture dst)
        {
            if (mat == null) { Graphics.Blit(src, dst); return; }
            int w = src.width / Downsample, h = src.height / Downsample;
            var bright = RenderTexture.GetTemporary(w, h, 0, src.format);
            var blur = RenderTexture.GetTemporary(w, h, 0, src.format);
            mat.SetFloat("_Threshold", Threshold);
            mat.SetFloat("_Intensity", Intensity);
            mat.SetFloat("_Scanlines", Scanlines ? 1 : 0);
            Graphics.Blit(src, bright, mat, 0);
            mat.SetVector("_Dir", new Vector4(1f / w, 0, 0, 0));
            Graphics.Blit(bright, blur, mat, 1);
            mat.SetVector("_Dir", new Vector4(0, 1f / h, 0, 0));
            Graphics.Blit(blur, bright, mat, 1);
            mat.SetTexture("_BloomTex", bright);
            Graphics.Blit(src, dst, mat, 2);
            RenderTexture.ReleaseTemporary(bright);
            RenderTexture.ReleaseTemporary(blur);
        }
    }
}
