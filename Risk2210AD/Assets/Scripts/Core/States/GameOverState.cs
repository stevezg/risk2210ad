namespace Risk2210.Core.States
{
    public sealed class GameOverState : PhaseState
    {
        public override PhaseId Id => PhaseId.GameOver;
        public override void Enter() => D.FinalScoring();
        public override CommandResult Handle(GameCommand cmd) => Fail("the game is over");
    }
}
