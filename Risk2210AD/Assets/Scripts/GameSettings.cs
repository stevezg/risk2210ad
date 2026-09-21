namespace Risk2210
{
    /// <summary>Choices made on the main menu, carried into the game scene.</summary>
    public static class GameSettings
    {
        public static int Players = 3;
        public static bool Spectate;
        public static int Seed = System.Environment.TickCount;
        public static readonly string[] Names = { "Red", "Blue", "Green", "Yellow", "Violet" };
    }
}
