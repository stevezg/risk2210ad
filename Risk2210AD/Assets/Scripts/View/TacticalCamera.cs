using UnityEngine;

namespace Risk2210.View
{
    /// <summary>Orthographic board camera: clamped zoom, bounded drag-to-pan, smooth focus and screenshake.</summary>
    [RequireComponent(typeof(Camera))]
    public sealed class TacticalCamera : MonoBehaviour
    {
        public float MinSize = 2.5f;
        public float MaxSize = 9.5f;
        public Rect Bounds = new Rect(-2, -19, 24, 21);
        public float SmoothTime = 0.25f;

        private Camera cam;
        private Vector3 targetPos;
        private float targetSize;
        private Vector3 velocity;
        private float sizeVelocity;
        private bool dragging;
        private Vector3 dragOrigin;
        private Vector3 dragCamOrigin;
        private float shake;
        private Vector3 shakeOffset;

        public Camera Camera => cam;
        public bool IsDragging => dragging;

        private void Awake()
        {
            cam = GetComponent<Camera>();
            cam.orthographic = true;
            targetPos = transform.position;
            targetSize = cam.orthographicSize;
        }

        public void SnapTo(Vector2 center, float size)
        {
            targetPos = new Vector3(center.x, center.y, transform.position.z);
            targetSize = Mathf.Clamp(size, MinSize, MaxSize);
            transform.position = targetPos;
            cam.orthographicSize = targetSize;
        }

        public void FocusOn(Vector2 center, float size)
        {
            targetPos = new Vector3(center.x, center.y, transform.position.z);
            targetSize = Mathf.Clamp(size, MinSize, MaxSize);
        }

        public void FrameRect(Rect r)
        {
            float size = Mathf.Max(r.height / 2f, r.width / 2f / cam.aspect) * 1.05f;
            FocusOn(r.center, size);
        }

        public void Shake(float amount) => shake = Mathf.Max(shake, amount);

        public void HandleInput(bool pointerOverUi, bool pointerOverTerritory)
        {
            if (!pointerOverUi)
            {
                float scroll = Input.mouseScrollDelta.y;
                if (Mathf.Abs(scroll) > 0.01f)
                {
                    Vector3 before = cam.ScreenToWorldPoint(Input.mousePosition);
                    targetSize = Mathf.Clamp(targetSize * (1f - scroll * 0.1f), MinSize, MaxSize);
                    float ratio = targetSize / cam.orthographicSize;
                    Vector3 after = before + (targetPos - before) * ratio;
                    targetPos = new Vector3(after.x, after.y, targetPos.z);
                }
                bool panPressed = Input.GetMouseButtonDown(1) || Input.GetMouseButtonDown(2) || (Input.GetMouseButtonDown(0) && !pointerOverTerritory);
                if (panPressed) { dragging = true; dragOrigin = cam.ScreenToWorldPoint(Input.mousePosition); dragCamOrigin = targetPos; }
            }
            if (dragging)
            {
                bool held = Input.GetMouseButton(0) || Input.GetMouseButton(1) || Input.GetMouseButton(2);
                if (!held) dragging = false;
                else
                {
                    Vector3 now = cam.ScreenToWorldPoint(Input.mousePosition);
                    Vector3 delta = (dragOrigin - now);
                    targetPos = dragCamOrigin + new Vector3(delta.x, delta.y, 0);
                    dragCamOrigin = targetPos; dragOrigin = cam.ScreenToWorldPoint(Input.mousePosition);
                }
            }
            float speed = targetSize * 1.2f * Time.deltaTime;
            if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) targetPos.y += speed;
            if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) targetPos.y -= speed;
            if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) targetPos.x -= speed;
            if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) targetPos.x += speed;
        }

        private void LateUpdate()
        {
            targetPos.x = Mathf.Clamp(targetPos.x, Bounds.xMin, Bounds.xMax);
            targetPos.y = Mathf.Clamp(targetPos.y, Bounds.yMin, Bounds.yMax);
            Vector3 smoothed = Vector3.SmoothDamp(transform.position - shakeOffset, targetPos, ref velocity, SmoothTime);
            cam.orthographicSize = Mathf.SmoothDamp(cam.orthographicSize, targetSize, ref sizeVelocity, SmoothTime);
            if (shake > 0.001f)
            {
                shakeOffset = (Vector3)(Random.insideUnitCircle * shake);
                shake = Mathf.Lerp(shake, 0, Time.deltaTime * 9f);
            }
            else shakeOffset = Vector3.zero;
            transform.position = smoothed + shakeOffset;
        }
    }
}
