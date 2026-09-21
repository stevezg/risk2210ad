using System;
using System.Collections;
using UnityEngine;

namespace Risk2210.View
{
    /// <summary>Frame-independent easing helpers used by every animation in the view layer.</summary>
    public static class Ease
    {
        public static float Linear(float t) => t;
        public static float InQuad(float t) => t * t;
        public static float OutQuad(float t) => 1 - (1 - t) * (1 - t);
        public static float InOutCubic(float t) => t < 0.5f ? 4 * t * t * t : 1 - Mathf.Pow(-2 * t + 2, 3) / 2;
        public static float OutBack(float t) { const float c1 = 1.70158f, c3 = c1 + 1; return 1 + c3 * Mathf.Pow(t - 1, 3) + c1 * Mathf.Pow(t - 1, 2); }
        public static float OutElastic(float t)
        {
            if (t <= 0 || t >= 1) return Mathf.Clamp01(t);
            const float c4 = (2 * Mathf.PI) / 3;
            return Mathf.Pow(2, -10 * t) * Mathf.Sin((t * 10 - 0.75f) * c4) + 1;
        }
        public static float InOutSine(float t) => -(Mathf.Cos(Mathf.PI * t) - 1) / 2;
    }

    public static class Tween
    {
        /// <summary>Runs `step(u)` with eased u in 0..1 over `duration` seconds (unscaled by Time.timeScale).</summary>
        public static IEnumerator Run(float duration, Func<float, float> ease, Action<float> step, Action done = null)
        {
            float t = 0;
            while (t < duration)
            {
                t += Time.deltaTime;
                step(ease(Mathf.Clamp01(t / duration)));
                yield return null;
            }
            step(1f);
            done?.Invoke();
        }

        public static IEnumerator Move(Transform tr, Vector3 from, Vector3 to, float duration, Func<float, float> ease)
            => Run(duration, ease, u => tr.position = Vector3.LerpUnclamped(from, to, u));

        public static IEnumerator Scale(Transform tr, Vector3 from, Vector3 to, float duration, Func<float, float> ease)
            => Run(duration, ease, u => tr.localScale = Vector3.LerpUnclamped(from, to, u));

        public static IEnumerator Wait(float seconds)
        {
            float t = 0;
            while (t < seconds) { t += Time.deltaTime; yield return null; }
        }
    }
}
