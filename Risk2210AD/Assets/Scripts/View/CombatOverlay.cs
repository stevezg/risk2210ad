using System.Collections;
using System.Collections.Generic;
using Risk2210.Core;
using UnityEngine;

namespace Risk2210.View
{
    /// <summary>
    /// Combat break-out: the camera zooms onto the two territories and 3D dice tumble in above the board,
    /// then snap to the rolled faces (attacker red, defender white, 8-sided rolls in cyan).
    /// </summary>
    public sealed class CombatOverlay : MonoBehaviour
    {
        public float SpinDuration = 0.65f;

        private Font font;
        private readonly List<GameObject> live = new List<GameObject>();

        private struct Face { public Vector3 Normal; public Vector3 Up; }
        private static readonly Face[] Faces =
        {
            new Face { Normal = Vector3.back, Up = Vector3.up }, new Face { Normal = Vector3.forward, Up = Vector3.up },
            new Face { Normal = Vector3.right, Up = Vector3.up }, new Face { Normal = Vector3.left, Up = Vector3.up },
            new Face { Normal = Vector3.up, Up = Vector3.forward }, new Face { Normal = Vector3.down, Up = Vector3.back },
        };

        private void Awake()
        {
            font = Resources.Load<Font>("gun4f");
            if (font == null) font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        }

        public IEnumerator Play(GameEvent ev, BoardView board, TacticalCamera cam)
        {
            Vector3 a = board.WorldPos(ev.From), d = board.WorldPos(ev.To);
            Vector3 mid = (a + d) * 0.5f;
            float spread = 0.55f;

            var dice = new List<(GameObject go, int value, bool d8)>();
            for (int i = 0; i < ev.AttackRolls.Length; i++)
            {
                bool d8 = i < ev.AttackD8;
                var go = MakeDie(a + (d - a).normalized * 0.9f + Vector3.up * (i - 1) * spread, d8 ? new Color(0.3f, 0.9f, 1f) : new Color(0.95f, 0.2f, 0.25f), d8);
                dice.Add((go, ev.AttackRolls[i], d8));
            }
            for (int i = 0; i < ev.DefendRolls.Length; i++)
            {
                bool d8 = i < ev.DefendD8;
                var go = MakeDie(d + (a - d).normalized * 0.9f + Vector3.up * (i - 0.5f) * spread, d8 ? new Color(0.3f, 0.9f, 1f) : new Color(0.92f, 0.92f, 0.95f), d8);
                dice.Add((go, ev.DefendRolls[i], d8));
            }

            var spins = new List<Vector3>();
            foreach (var _ in dice) spins.Add(Random.onUnitSphere * Random.Range(720f, 1440f));
            var startRot = new List<Quaternion>();
            foreach (var die in dice) startRot.Add(die.go.transform.rotation);

            yield return Tween.Run(SpinDuration, Ease.OutQuad, u =>
            {
                for (int i = 0; i < dice.Count; i++)
                {
                    var t = dice[i].go.transform;
                    t.rotation = startRot[i] * Quaternion.Euler(spins[i] * u);
                    t.localScale = Vector3.one * (0.45f + 0.15f * Mathf.Sin(u * Mathf.PI));
                }
            });
            for (int i = 0; i < dice.Count; i++) SnapToValue(dice[i].go, dice[i].value);
            cam.Shake(0.08f);
            yield return Tween.Wait(0.75f);

            // fade out
            yield return Tween.Run(0.25f, Ease.InQuad, u =>
            {
                foreach (var die in dice) die.go.transform.localScale = Vector3.one * 0.45f * (1 - u);
            });
            foreach (var die in dice) Destroy(die.go);
            live.Clear();
        }

        private GameObject MakeDie(Vector3 pos, Color color, bool d8)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
            Destroy(go.GetComponent<Collider>());
            go.name = d8 ? "D8" : "D6";
            go.transform.position = new Vector3(pos.x, pos.y, -1.2f);
            go.transform.rotation = Random.rotation;
            go.transform.localScale = Vector3.one * 0.45f;
            var mr = go.GetComponent<MeshRenderer>();
            mr.sharedMaterial = BoardView.SolidMaterial(color);
            for (int f = 0; f < Faces.Length; f++)
            {
                var tgo = new GameObject("Face" + f);
                tgo.transform.SetParent(go.transform, false);
                tgo.transform.localPosition = Faces[f].Normal * 0.51f;
                tgo.transform.localRotation = Quaternion.LookRotation(-Faces[f].Normal, Faces[f].Up);
                var tm = tgo.AddComponent<TextMesh>();
                tm.font = font; tm.fontSize = 64; tm.characterSize = 0.11f; tm.anchor = TextAnchor.MiddleCenter; tm.alignment = TextAlignment.Center;
                tm.color = color.grayscale > 0.6f ? new Color(0.05f, 0.05f, 0.1f) : Color.white;
                tm.fontStyle = FontStyle.Bold;
                tm.text = (f + 1).ToString();
                tgo.GetComponent<MeshRenderer>().sharedMaterial = font.material;
            }
            live.Add(go);
            return go;
        }

        /// <summary>Rotates the cube so the face showing `value` looks at the camera (camera looks along +z).</summary>
        private static void SnapToValue(GameObject die, int value)
        {
            int f = 0;
            var tm = die.transform.GetChild(f).GetComponent<TextMesh>();
            tm.text = value.ToString();
            var q = Quaternion.LookRotation(-Faces[f].Normal, Faces[f].Up);
            die.transform.rotation = Quaternion.Inverse(q);
        }
    }
}
