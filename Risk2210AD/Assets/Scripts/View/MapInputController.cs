using System;
using UnityEngine;

namespace Risk2210.View
{
    /// <summary>Hover / click detection over the territory hit-boxes. Feeds the HUD and the camera.</summary>
    public sealed class MapInputController : MonoBehaviour
    {
        public BoardView Board;
        public TacticalCamera Cam;
        public Func<bool> IsPointerOverUi = () => false;
        public event Action<int> Hovered;          // -1 when nothing
        public event Action<int> Clicked;
        public int HoveredTerritory { get; private set; } = -1;

        private Vector3 pressPos;

        private void Update()
        {
            if (Board == null || Cam == null) return;
            bool overUi = IsPointerOverUi();
            int hit = -1;
            if (!overUi)
            {
                Vector3 w = Cam.Camera.ScreenToWorldPoint(Input.mousePosition);
                var col = Physics2D.OverlapPoint(new Vector2(w.x, w.y));
                if (col != null)
                {
                    var tv = col.GetComponent<TerritoryView>();
                    if (tv != null) hit = tv.Id;
                }
            }
            if (hit != HoveredTerritory)
            {
                HoveredTerritory = hit;
                Hovered?.Invoke(hit);
            }
            Cam.HandleInput(overUi, hit >= 0);
            if (!overUi && Input.GetMouseButtonDown(0)) pressPos = Input.mousePosition;
            if (!overUi && Input.GetMouseButtonUp(0) && hit >= 0 && (Input.mousePosition - pressPos).magnitude < 12f)
                Clicked?.Invoke(hit);
        }
    }
}
