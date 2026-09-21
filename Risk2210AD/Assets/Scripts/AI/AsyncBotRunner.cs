using System.Collections.Generic;
using System.Threading.Tasks;
using Risk2210.Core;
using UnityEngine;

namespace Risk2210.AI
{
    /// <summary>
    /// Drives every bot player. When the engine is waiting on a bot, a snapshot of the state is handed to
    /// <see cref="BotBrain.Decide"/> on a thread-pool task; the resulting command is applied on the main
    /// thread once the animation layer is idle, so bot turns never stall rendering and stay watchable.
    /// </summary>
    public sealed class AsyncBotRunner : MonoBehaviour
    {
        public float PaceSeconds = 0.35f;
        public System.Func<bool> IsViewBusy = () => false;

        private GameDirector director;
        private readonly Dictionary<int, BotBrain> brains = new Dictionary<int, BotBrain>();
        private Task<GameCommand> pending;
        private int pendingPlayer = -1;
        private float cooldown;

        public void Bind(GameDirector d, int seed)
        {
            director = d;
            brains.Clear();
            for (int p = 0; p < d.State.Players.Count; p++)
                if (d.State.Players[p].IsBot) brains[p] = new BotBrain(seed * 31 + p);
        }

        private void Update()
        {
            if (director == null || director.State.Phase == PhaseId.GameOver) return;
            if (cooldown > 0) { cooldown -= Time.deltaTime; return; }

            if (pending != null)
            {
                if (!pending.IsCompleted) return;
                var cmd = pending.Result;
                pending = null;
                if (cmd != null && director.State.ExpectedActor == pendingPlayer)
                {
                    var r = director.Submit(cmd);
                    if (!r.Ok) Debug.LogWarning($"Bot {director.Name(pendingPlayer)} command {cmd.GetType().Name} rejected: {r.Error}");
                    cooldown = PaceSeconds;
                }
                return;
            }

            if (IsViewBusy()) return;
            int actor = director.State.ExpectedActor;
            if (actor < 0 || !brains.ContainsKey(actor)) return;
            var snapshot = director.State.Clone();
            var brain = brains[actor];
            pendingPlayer = actor;
            pending = Task.Run(() => brain.Decide(snapshot, actor));
        }
    }
}
