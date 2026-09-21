using System.Collections.Generic;
using Risk2210.Core;
using UnityEngine;

namespace Risk2210.View
{
    /// <summary>Builds and refreshes every territory view and border line from the map graph.</summary>
    public sealed class BoardView : MonoBehaviour
    {
        public MapGraph Map { get; private set; }
        public IReadOnlyList<TerritoryView> Territories => views;
        public TerritoryView this[int id] => views[id];

        private readonly List<TerritoryView> views = new List<TerritoryView>();
        private readonly List<LineRenderer> lines = new List<LineRenderer>();
        private Material territoryMaterial, lineMaterial, waterLineMaterial, moonLineMaterial;
        private Font font;

        public static readonly Color[] PlayerColors =
        {
            new Color(1f, 0.28f, 0.35f), new Color(0.24f, 0.6f, 1f), new Color(0.3f, 0.9f, 0.47f),
            new Color(1f, 0.82f, 0.24f), new Color(0.78f, 0.47f, 1f), new Color(0.55f, 0.6f, 0.68f)
        };

        public static Color PlayerColor(int p) => p < 0 ? PlayerColors[5] : PlayerColors[p % PlayerColors.Length];

        public static Color RegionColor(int region)
        {
            Color[] c =
            {
                new Color(0.98f, 0.86f, 0.35f), new Color(0.86f, 0.55f, 0.31f), new Color(0.75f, 0.47f, 1f), new Color(0.47f, 0.9f, 0.9f),
                new Color(0.47f, 0.94f, 0.51f), new Color(1f, 0.47f, 0.43f), new Color(0.31f, 0.75f, 1f), new Color(1f, 0.9f, 0.27f),
                new Color(1f, 0.43f, 0.31f), new Color(0.55f, 0.94f, 0.39f), new Color(1f, 0.67f, 0.27f), new Color(0.82f, 0.82f, 0.92f),
                new Color(0.7f, 0.7f, 0.84f), new Color(0.59f, 0.59f, 0.75f)
            };
            return c[region % c.Length];
        }

        private static readonly Dictionary<Color, Material> solidCache = new Dictionary<Color, Material>();
        public static Material SolidMaterial(Color c)
        {
            if (solidCache.TryGetValue(c, out var m)) return m;
            var shader = Shader.Find("Risk2210/NeonLine");
            m = new Material(shader != null ? shader : Shader.Find("Sprites/Default")) { color = c };
            solidCache[c] = m;
            return m;
        }

        public void Build(MapGraph map)
        {
            Map = map;
            font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            territoryMaterial = new Material(Shader.Find("Risk2210/NeonTerritory"));
            lineMaterial = SolidMaterial(new Color(0.4f, 0.48f, 0.65f, 0.8f));
            waterLineMaterial = SolidMaterial(new Color(0.25f, 0.6f, 1f, 0.9f));
            moonLineMaterial = SolidMaterial(new Color(0.6f, 0.6f, 0.85f, 0.8f));

            var linesRoot = new GameObject("Borders").transform;
            linesRoot.SetParent(transform, false);
            foreach (var e in map.Edges)
            {
                var a = map[e.A].Position; var b = map[e.B].Position;
                var mat = e.Kind == EdgeKind.Water ? waterLineMaterial : e.Kind == EdgeKind.Moon ? moonLineMaterial : lineMaterial;
                if (e.WrapsAround)
                {
                    var l = a.x < b.x ? a : b; var r = a.x < b.x ? b : a;
                    lines.Add(MakeLine(linesRoot, l, new Vector2(-1.2f, l.y), mat));
                    lines.Add(MakeLine(linesRoot, r, new Vector2(20.4f, r.y), mat));
                }
                else lines.Add(MakeLine(linesRoot, a, b, mat));
            }

            var terrRoot = new GameObject("Territories").transform;
            terrRoot.SetParent(transform, false);
            foreach (var node in map.Territories)
            {
                var poly = VectorLandforms.Polygon(map, node.Id);
                var sprite = Resources.Load<Sprite>("Textures/Territories/" + node.Name);
                views.Add(TerritoryView.Create(terrRoot, node, poly, territoryMaterial, RegionColor(node.RegionId), font, sprite));
            }

            var moonPlate = new GameObject("MoonPlate");
            moonPlate.transform.SetParent(transform, false);
            var tm = moonPlate.AddComponent<TextMesh>();
            tm.font = font; tm.fontSize = 64; tm.characterSize = 0.07f; tm.text = "THE MOON"; tm.color = new Color(0.6f, 0.6f, 0.85f);
            tm.anchor = TextAnchor.UpperLeft;
            moonPlate.GetComponent<MeshRenderer>().sharedMaterial = font.material;
            moonPlate.transform.position = new Vector3(0.6f, -12.6f, -0.1f);
        }

        private static LineRenderer MakeLine(Transform parent, Vector2 a, Vector2 b, Material mat)
        {
            var go = new GameObject("Border");
            go.transform.SetParent(parent, false);
            var lr = go.AddComponent<LineRenderer>();
            lr.sharedMaterial = mat;
            lr.positionCount = 2;
            lr.SetPosition(0, new Vector3(a.x, a.y, -0.05f));
            lr.SetPosition(1, new Vector3(b.x, b.y, -0.05f));
            lr.startWidth = lr.endWidth = 0.045f;
            lr.useWorldSpace = true;
            lr.numCapVertices = 3;
            return lr;
        }

        public void RefreshAll(GameState s, bool animate)
        {
            for (int t = 0; t < views.Count; t++)
            {
                var ts = s.Territories[t];
                views[t].Refresh(ts, PlayerColor(ts.Owner), animate);
            }
        }

        public void ClearHighlights(HighlightMode only = HighlightMode.None)
        {
            foreach (var v in views)
                if (only == HighlightMode.None || v.Highlight == only) v.SetHighlight(HighlightMode.None);
        }

        public Vector3 WorldPos(int t) => views[t].transform.position;
    }
}
