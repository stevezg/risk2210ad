using System.Collections;
using System.Collections.Generic;
using Risk2210.Core;
using UnityEngine;

namespace Risk2210.View
{
    /// <summary>
    /// Consumes engine events in order and turns them into animations: transport orbs along borders,
    /// combat dice overlays, loss flashes, conquest shockwaves and colour fades. Bots wait while it is busy.
    /// </summary>
    public sealed class AnimationDirector : MonoBehaviour
    {
        public BoardView Board;
        public TacticalCamera Cam;
        public CombatOverlay Combat;
        public GameState State;
        public bool FastForward;

        private readonly Queue<GameEvent> queue = new Queue<GameEvent>();
        private bool running;
        private Material orbMaterial, ringMaterial;

        public bool IsBusy => running || queue.Count > 0;

        public void Enqueue(GameEvent ev) { queue.Enqueue(ev); }

        private void Update()
        {
            if (!running && queue.Count > 0) StartCoroutine(Process(queue.Dequeue()));
        }

        private IEnumerator Process(GameEvent ev)
        {
            running = true;
            if (FastForward) { Board.RefreshAll(State, false); running = false; yield break; }
            switch (ev.Type)
            {
                case GameEventType.ModsDeployed:
                    if (State.Phase == PhaseId.Setup) StartCoroutine(Pulse(Board.WorldPos(ev.To), BoardView.PlayerColor(ev.Player), 0.4f, 0.2f));
                    else yield return Pulse(Board.WorldPos(ev.To), BoardView.PlayerColor(ev.Player), 0.5f, 0.25f);
                    break;
                case GameEventType.UnitsMoved:
                case GameEventType.Fortified:
                    yield return Orb(ev.From, ev.To, BoardView.PlayerColor(ev.Player), ev.Amount);
                    break;
                case GameEventType.InvasionDeclared:
                    Focus(ev.From, ev.To);
                    Board[ev.From].SetHighlight(HighlightMode.Attacking);
                    Board[ev.To].SetHighlight(HighlightMode.Defending);
                    yield return Tween.Wait(0.25f);
                    break;
                case GameEventType.InvasionCancelled:
                    Board.ClearHighlights(HighlightMode.Attacking);
                    Board.ClearHighlights(HighlightMode.Defending);
                    break;
                case GameEventType.Battle:
                    if (Combat != null) yield return Combat.Play(ev, Board, Cam);
                    if (ev.Amount > 0) StartCoroutine(Board[ev.From].Flash(new Color(0.1f, 0.02f, 0.02f)));
                    if (ev.Amount2 > 0) StartCoroutine(Board[ev.To].Flash(new Color(1f, 0.15f, 0.1f)));
                    if (ev.Amount + ev.Amount2 > 0) Cam.Shake(0.05f * (ev.Amount + ev.Amount2));
                    yield return Tween.Wait(0.3f);
                    break;
                case GameEventType.TerritoryCaptured:
                    Board.ClearHighlights(HighlightMode.Attacking);
                    Board.ClearHighlights(HighlightMode.Defending);
                    yield return Shockwave(Board.WorldPos(ev.To), BoardView.PlayerColor(ev.Player));
                    break;
                case GameEventType.UnitsDestroyed:
                    if (ev.To >= 0) yield return Shockwave(Board.WorldPos(ev.To), new Color(1f, 0.3f, 0.2f), 0.45f, 1.2f);
                    break;
                case GameEventType.TerritoryDevastated:
                    Cam.Shake(0.35f);
                    yield return Shockwave(Board.WorldPos(ev.To), new Color(1f, 0.2f, 0.1f), 0.9f, 3f);
                    break;
                case GameEventType.TurnStarted:
                    Board.ClearHighlights();
                    Cam.Home();
                    break;
                case GameEventType.YearStarted:
                    Cam.Home();
                    break;
            }
            Board.RefreshAll(State, true);
            running = false;
        }

        private void Focus(int a, int b)
        {
            Vector3 pa = Board.WorldPos(a), pb = Board.WorldPos(b);
            if (Vector3.Distance(pa, pb) > 12f) { Cam.FocusOn(pb, 5f); return; }
            Cam.FocusOn((pa + pb) * 0.5f, Mathf.Max(3.5f, Vector3.Distance(pa, pb) * 0.9f));
        }

        // ------------------------------------------------------------------
        // Primitive effects
        // ------------------------------------------------------------------

        private Material OrbMaterial() => orbMaterial ??= BoardView.SolidMaterial(Color.white);

        public IEnumerator Orb(int from, int to, Color color, int count)
        {
            Vector3 a = Board.WorldPos(from), b = Board.WorldPos(to);
            a.z = b.z = -0.5f;
            var go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            Destroy(go.GetComponent<Collider>());
            go.name = "TransportOrb";
            go.transform.localScale = Vector3.one * 0.28f;
            var mr = go.GetComponent<MeshRenderer>();
            mr.material = new Material(OrbMaterial()) { color = color * 1.6f };
            var trail = go.AddComponent<TrailRenderer>();
            trail.time = 0.35f; trail.startWidth = 0.22f; trail.endWidth = 0.0f;
            trail.material = BoardView.SolidMaterial(color);
            trail.startColor = color; trail.endColor = new Color(color.r, color.g, color.b, 0);
            float duration = Mathf.Clamp(Vector3.Distance(a, b) * 0.22f, 0.35f, 1.1f);
            yield return Tween.Run(duration, Ease.InOutSine, u =>
            {
                Vector3 p = Vector3.Lerp(a, b, u);
                p.y += Mathf.Sin(u * Mathf.PI) * 0.25f;
                go.transform.position = p;
            });
            Destroy(go, 0.4f);
            yield return Pulse(b, color, 0.5f, 0.2f);
        }

        public IEnumerator Pulse(Vector3 at, Color color, float radius, float duration)
        {
            var ring = MakeRing(at, color);
            yield return Tween.Run(duration, Ease.OutQuad, u =>
            {
                ring.transform.localScale = Vector3.one * Mathf.Lerp(0.2f, radius, u);
                ring.startColor = ring.endColor = new Color(color.r, color.g, color.b, 1 - u);
            });
            Destroy(ring.gameObject);
        }

        public IEnumerator Shockwave(Vector3 at, Color color, float duration = 0.7f, float radius = 2.2f)
        {
            var ring = MakeRing(at, color);
            var ring2 = MakeRing(at, Color.white);
            yield return Tween.Run(duration, Ease.OutQuad, u =>
            {
                ring.transform.localScale = Vector3.one * Mathf.Lerp(0.3f, radius, u);
                ring2.transform.localScale = Vector3.one * Mathf.Lerp(0.2f, radius * 0.6f, u);
                ring.startColor = ring.endColor = new Color(color.r, color.g, color.b, 1 - u);
                ring2.startColor = ring2.endColor = new Color(1, 1, 1, 0.6f * (1 - u));
                ring.startWidth = ring.endWidth = 0.08f * (1 - u) + 0.02f;
            });
            Destroy(ring.gameObject);
            Destroy(ring2.gameObject);
        }

        private LineRenderer MakeRing(Vector3 at, Color color)
        {
            var go = new GameObject("Ring");
            go.transform.position = new Vector3(at.x, at.y, -0.6f);
            var lr = go.AddComponent<LineRenderer>();
            lr.sharedMaterial = ringMaterial ??= BoardView.SolidMaterial(Color.white);
            lr.useWorldSpace = false;
            lr.loop = true;
            const int n = 48;
            lr.positionCount = n;
            for (int i = 0; i < n; i++)
            {
                float a = i / (float)n * Mathf.PI * 2;
                lr.SetPosition(i, new Vector3(Mathf.Cos(a), Mathf.Sin(a), 0));
            }
            lr.startWidth = lr.endWidth = 0.06f;
            lr.startColor = lr.endColor = color;
            return lr;
        }
    }
}
