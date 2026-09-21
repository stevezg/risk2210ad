using System.Collections;
using System.Collections.Generic;
using Risk2210.Core;
using UnityEngine;

namespace Risk2210.View
{
    public enum HighlightMode { None, Hover, Selected, Option, Target, FortifyTarget, Attacking, Defending }

    /// <summary>Visual + hit-box of one territory. Owner colour changes are animated.</summary>
    public sealed class TerritoryView : MonoBehaviour
    {
        public int Id { get; private set; }
        public TerritoryNode Node { get; private set; }
        public Vector2 Center { get; private set; }

        private MeshRenderer meshRenderer;
        private MaterialPropertyBlock block;
        private TextMesh label, unitsText, cmdText;
        private GameObject stationIcon, devastatedIcon;
        private Color fillColor = new Color(0.08f, 0.11f, 0.2f, 0.85f);
        private Color borderColor;
        private float hover, pulse, selectedGlow;
        private HighlightMode mode;
        private Coroutine colorRoutine;

        private static readonly int FillId = Shader.PropertyToID("_FillColor");
        private static readonly int BorderId = Shader.PropertyToID("_BorderColor");
        private static readonly int HoverId = Shader.PropertyToID("_Hover");
        private static readonly int PulseId = Shader.PropertyToID("_Pulse");

        public static TerritoryView Create(Transform parent, TerritoryNode node, List<Vector2> polygon, Material material, Color regionColor, Font font, Sprite sprite)
        {
            var go = new GameObject(node.Name);
            go.transform.SetParent(parent, false);
            var view = go.AddComponent<TerritoryView>();
            view.Id = node.Id;
            view.Node = node;
            view.borderColor = regionColor;

            var mesh = VectorLandforms.BuildMesh(polygon, out Vector2 centroid);
            view.Center = centroid;
            go.transform.position = new Vector3(centroid.x, centroid.y, 0);

            var mf = go.AddComponent<MeshFilter>();
            mf.sharedMesh = mesh;
            view.meshRenderer = go.AddComponent<MeshRenderer>();
            view.meshRenderer.sharedMaterial = material;
            view.meshRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            view.meshRenderer.receiveShadows = false;
            view.block = new MaterialPropertyBlock();

            if (sprite != null)
            {
                var spriteGo = new GameObject("Texture");
                spriteGo.transform.SetParent(go.transform, false);
                spriteGo.transform.localPosition = new Vector3(0, 0, 0.01f);
                var sr = spriteGo.AddComponent<SpriteRenderer>();
                sr.sprite = sprite;
                sr.sortingOrder = -1;
            }

            var col = go.AddComponent<PolygonCollider2D>();
            var local = new Vector2[polygon.Count];
            for (int i = 0; i < polygon.Count; i++) local[i] = polygon[i] - centroid;
            col.SetPath(0, local);

            view.label = MakeText(go.transform, "Label", new Vector3(0, -0.34f, -0.1f), 0.11f, new Color(0.75f, 0.85f, 0.95f), font, TextAnchor.UpperCenter);
            view.label.text = node.Name;
            view.unitsText = MakeText(go.transform, "Units", new Vector3(0, 0.06f, -0.1f), 0.26f, Color.white, font, TextAnchor.MiddleCenter);
            view.unitsText.fontStyle = FontStyle.Bold;
            view.cmdText = MakeText(go.transform, "Commanders", new Vector3(0, 0.3f, -0.1f), 0.1f, new Color(1f, 0.92f, 0.55f), font, TextAnchor.MiddleCenter);

            view.stationIcon = GameObject.CreatePrimitive(PrimitiveType.Quad);
            Object.Destroy(view.stationIcon.GetComponent<Collider>());
            view.stationIcon.name = "Station";
            view.stationIcon.transform.SetParent(go.transform, false);
            view.stationIcon.transform.localPosition = new Vector3(0.36f, 0.3f, -0.1f);
            view.stationIcon.transform.localScale = new Vector3(0.16f, 0.16f, 1);
            view.stationIcon.GetComponent<MeshRenderer>().sharedMaterial = BoardView.SolidMaterial(Color.white);
            view.stationIcon.SetActive(false);

            view.devastatedIcon = MakeText(go.transform, "Devastated", new Vector3(0, 0.04f, -0.1f), 0.4f, new Color(1f, 0.3f, 0.25f), font, TextAnchor.MiddleCenter).gameObject;
            view.devastatedIcon.GetComponent<TextMesh>().text = "X";
            view.devastatedIcon.SetActive(false);

            view.Apply();
            return view;
        }

        private static TextMesh MakeText(Transform parent, string name, Vector3 pos, float size, Color color, Font font, TextAnchor anchor)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            go.transform.localPosition = pos;
            var tm = go.AddComponent<TextMesh>();
            tm.font = font;
            tm.fontSize = 64;
            tm.characterSize = size / 6.4f;
            tm.anchor = anchor;
            tm.alignment = TextAlignment.Center;
            tm.color = color;
            var mr = go.GetComponent<MeshRenderer>();
            if (font != null) mr.sharedMaterial = font.material;
            mr.sortingOrder = 10;
            return tm;
        }

        public void Refresh(TerritoryState ts, Color ownerColor, bool animate)
        {
            devastatedIcon.SetActive(ts.Devastated);
            stationIcon.SetActive(ts.SpaceStation);
            if (ts.Devastated)
            {
                unitsText.text = ""; cmdText.text = "";
                SetFill(new Color(0.18f, 0.05f, 0.05f, 0.9f), animate);
                borderColor = new Color(0.7f, 0.2f, 0.2f);
                Apply();
                return;
            }
            unitsText.text = ts.Owner >= 0 ? ts.Units.ToString() : "";
            var sb = new System.Text.StringBuilder();
            for (int c = 0; c < MapGraph.NumCommanders; c++) if (ts.Commanders[c]) sb.Append("LDNXS"[c]);
            cmdText.text = sb.ToString();
            var target = ts.Owner >= 0 ? new Color(ownerColor.r, ownerColor.g, ownerColor.b, 0.55f) : new Color(0.08f, 0.11f, 0.2f, 0.85f);
            SetFill(target, animate);
        }

        private void SetFill(Color target, bool animate)
        {
            if (!animate || !gameObject.activeInHierarchy) { fillColor = target; Apply(); return; }
            if (colorRoutine != null) StopCoroutine(colorRoutine);
            Color from = fillColor;
            colorRoutine = StartCoroutine(Tween.Run(0.6f, Ease.InOutCubic, u => { fillColor = Color.Lerp(from, target, u); Apply(); }));
        }

        public void SetHighlight(HighlightMode m)
        {
            mode = m;
            Apply();
        }

        public HighlightMode Highlight => mode;

        private void Update()
        {
            float target = mode == HighlightMode.None ? 0 : 1;
            hover = Mathf.MoveTowards(hover, target, Time.deltaTime * 6f);
            pulse = mode == HighlightMode.None ? 0 : 0.5f + 0.5f * Mathf.Sin(Time.time * 5f);
            if (mode != HighlightMode.None || hover > 0) Apply();
        }

        public Color HighlightColor()
        {
            switch (mode)
            {
                case HighlightMode.Hover: return new Color(0.5f, 0.95f, 1f);
                case HighlightMode.Selected: return Color.white;
                case HighlightMode.Option: return new Color(1f, 0.85f, 0.2f);
                case HighlightMode.Target: return new Color(1f, 0.55f, 0.15f);
                case HighlightMode.FortifyTarget: return new Color(0.4f, 0.7f, 1f);
                case HighlightMode.Attacking: return new Color(1f, 0.55f, 0.15f);
                case HighlightMode.Defending: return new Color(1f, 0.25f, 0.25f);
                default: return borderColor;
            }
        }

        private void Apply()
        {
            if (meshRenderer == null) return;
            Color border = Color.Lerp(borderColor, HighlightColor(), hover);
            block.SetColor(FillId, fillColor);
            block.SetColor(BorderId, border);
            block.SetFloat(HoverId, hover + selectedGlow);
            block.SetFloat(PulseId, pulse);
            meshRenderer.SetPropertyBlock(block);
        }

        /// <summary>Brief red/black flash used for combat losses.</summary>
        public IEnumerator Flash(Color color, float duration = 0.35f)
        {
            Color original = fillColor;
            yield return Tween.Run(duration, Ease.OutQuad, u =>
            {
                float k = Mathf.Sin(u * Mathf.PI * 3) * 0.5f + 0.5f;
                fillColor = Color.Lerp(original, color, k * (1 - u));
                Apply();
            });
            fillColor = original;
            Apply();
        }
    }
}
