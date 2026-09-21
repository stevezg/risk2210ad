using System.Collections.Generic;
using Risk2210.Core;
using UnityEngine;

namespace Risk2210.View
{
    /// <summary>
    /// Generates the 2D polygon for each territory (used both as the visible landform and as the
    /// PolygonCollider2D hit-box). Land and lunar cells are Voronoi regions between neighbouring nodes
    /// of the same board, clipped to a rounded radius so coastlines stay organic; water colonies are
    /// the circular platforms of the physical board.
    ///
    /// Drop-in replacement: if a sprite named after the territory exists under
    /// Resources/Textures/Territories, <see cref="BoardView"/> uses it for the fill instead.
    /// </summary>
    public static class VectorLandforms
    {
        private const int CircleSegments = 28;

        public static List<Vector2> Polygon(MapGraph map, int id)
        {
            var node = map[id];
            if (node.Type == TerritoryType.Water) return Circle(node.Position, 0.42f);

            float nearest = float.MaxValue;
            foreach (var o in map.Territories)
                if (o.Id != id && o.Type == node.Type) nearest = Mathf.Min(nearest, Vector2.Distance(o.Position, node.Position));
            float radius = Mathf.Clamp(nearest * 0.68f, 0.55f, 1.5f);
            var poly = Circle(node.Position, radius);

            foreach (var o in map.Territories)
            {
                if (o.Id == id || o.Type != node.Type) continue;
                Vector2 d = o.Position - node.Position;
                float dist = d.magnitude;
                if (dist > radius * 2.2f) continue;
                Vector2 n = d / dist;
                Vector2 mid = node.Position + n * (dist * 0.5f - 0.09f);   // small channel between cells
                poly = ClipHalfPlane(poly, mid, n);
                if (poly.Count < 3) break;
            }
            return poly;
        }

        public static List<Vector2> Circle(Vector2 c, float r)
        {
            var list = new List<Vector2>(CircleSegments);
            for (int i = 0; i < CircleSegments; i++)
            {
                float a = i / (float)CircleSegments * Mathf.PI * 2;
                list.Add(c + new Vector2(Mathf.Cos(a), Mathf.Sin(a)) * r);
            }
            return list;
        }

        /// <summary>Sutherland–Hodgman clip keeping the side where dot(p - point, normal) &lt;= 0.</summary>
        private static List<Vector2> ClipHalfPlane(List<Vector2> poly, Vector2 point, Vector2 normal)
        {
            var output = new List<Vector2>(poly.Count + 2);
            for (int i = 0; i < poly.Count; i++)
            {
                Vector2 cur = poly[i], prev = poly[(i + poly.Count - 1) % poly.Count];
                float dc = Vector2.Dot(cur - point, normal), dp = Vector2.Dot(prev - point, normal);
                bool inC = dc <= 0, inP = dp <= 0;
                if (inC)
                {
                    if (!inP) output.Add(Vector2.Lerp(prev, cur, dp / (dp - dc)));
                    output.Add(cur);
                }
                else if (inP) output.Add(Vector2.Lerp(prev, cur, dp / (dp - dc)));
            }
            return output;
        }

        /// <summary>
        /// Fan-triangulated mesh in local space (origin = centroid). uv.x is 0 on the rim and 1 at the centre so
        /// the neon-border shader can glow along the edge.
        /// </summary>
        public static Mesh BuildMesh(List<Vector2> poly, out Vector2 centroid)
        {
            centroid = Vector2.zero;
            foreach (var p in poly) centroid += p;
            centroid /= poly.Count;

            int n = poly.Count;
            var verts = new Vector3[n + 1];
            var uvs = new Vector2[n + 1];
            var cols = new Color[n + 1];
            verts[0] = Vector3.zero; uvs[0] = new Vector2(1, 0); cols[0] = Color.white;
            for (int i = 0; i < n; i++)
            {
                verts[i + 1] = poly[i] - centroid;
                uvs[i + 1] = new Vector2(0, 0);
                cols[i + 1] = Color.white;
            }
            var tris = new int[n * 3];
            for (int i = 0; i < n; i++)
            {
                tris[i * 3] = 0;
                tris[i * 3 + 1] = i + 1;
                tris[i * 3 + 2] = (i + 1) % n + 1;
            }
            // polygons are CCW in board space; Unity wants clockwise for a front face toward -z camera
            for (int i = 0; i < n; i++) { int t = tris[i * 3 + 1]; tris[i * 3 + 1] = tris[i * 3 + 2]; tris[i * 3 + 2] = t; }

            var mesh = new Mesh { name = "Territory" };
            mesh.vertices = verts; mesh.uv = uvs; mesh.colors = cols; mesh.triangles = tris;
            mesh.RecalculateBounds();
            return mesh;
        }
    }
}
