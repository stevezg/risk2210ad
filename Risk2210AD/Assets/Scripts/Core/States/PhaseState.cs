namespace Risk2210.Core.States
{
    /// <summary>One state of the turn state machine. Handles the commands legal in that phase.</summary>
    public abstract class PhaseState
    {
        protected GameDirector D;
        protected GameState S => D.State;
        protected MapGraph Map => D.Map;

        public abstract PhaseId Id { get; }
        public void Bind(GameDirector director) => D = director;
        public virtual void Enter() { }
        public virtual void Exit() { }
        public abstract CommandResult Handle(GameCommand cmd);

        protected CommandResult Fail(string msg) => CommandResult.Fail(msg);
        protected CommandResult Ok => CommandResult.Success;
        protected bool ValidTerritory(int t) => t >= 0 && t < Map.Count;
    }
}
