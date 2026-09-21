using UnityEngine;

namespace Risk2210.View
{
    /// <summary>A large quad behind the board running the tactical grid shader.</summary>
    public static class GridBackground
    {
        public static GameObject Create(Transform parent, Rect area)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Quad);
            Object.Destroy(go.GetComponent<Collider>());
            go.name = "GridBackground";
            go.transform.SetParent(parent, false);
            go.transform.position = new Vector3(area.center.x, area.center.y, 1f);
            go.transform.localScale = new Vector3(area.width, area.height, 1);
            var shader = Shader.Find("Risk2210/Grid");
            var mr = go.GetComponent<MeshRenderer>();
            mr.sharedMaterial = new Material(shader != null ? shader : Shader.Find("Sprites/Default"));
            mr.sharedMaterial.SetVector("_Area", new Vector4(area.xMin, area.yMin, area.width, area.height));
            return go;
        }
    }
}
