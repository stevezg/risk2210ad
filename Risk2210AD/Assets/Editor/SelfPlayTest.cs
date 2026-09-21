using System;
using System.Collections.Generic;
using Risk2210.AI;
using Risk2210.Core;
using UnityEditor;
using UnityEngine;

namespace Risk2210.EditorTools
{
    /// <summary>
    /// Headless engine fuzzing: bots play complete games synchronously and every turn is checked
    /// against board invariants. Run from the menu or `-executeMethod Risk2210.EditorTools.SelfPlayTest.Run`.
    /// </summary>
    public static class SelfPlayTest
    {
        private static int failures;

        private static void Check(bool ok, string msg)
        {
            if (ok) return;
            failures++;
            Debug.LogError("SELFPLAY FAIL: " + msg);
        }

        [MenuItem("Risk 2210/Run Self-Play Test")]
        public static void Run()
        {
            failures = 0;
            int games = 0, commands = 0;
            var sw = System.Diagnostics.Stopwatch.StartNew();
            for (int players = 2; players <= 5; players++)
            {
                for (int seed = 1; seed <= 25; seed++)
                {
                    string ctx = $"players={players} seed={seed}";
                    var names = new List<string>(); var bots = new List<bool>();
                    for (int p = 0; p < players; p++) { names.Add("P" + p); bots.Add(true); }
                    var d = new GameDirector(names, bots, seed * 1000 + players);
                    var brains = new Dictionary<int, BotBrain>();
                    for (int p = 0; p < players; p++) brains[p] = new BotBrain(seed + p);
                    d.Start();
                    int steps = 0;
                    while (d.State.Phase != PhaseId.GameOver && steps < 20000)
                    {
                        int actor = d.State.ExpectedActor;
                        Check(actor >= 0, ctx + ": engine waiting on nobody in phase " + d.State.Phase);
                        if (actor < 0) break;
                        var cmd = brains[actor].Decide(d.State.Clone(), actor);
                        Check(cmd != null, ctx + ": bot produced no command in " + d.State.Phase + " prompt=" + (d.State.Prompt?.Kind.ToString() ?? "none"));
                        if (cmd == null) break;
                        var r = d.Submit(cmd);
                        Check(r.Ok, ctx + ": " + cmd.GetType().Name + " rejected: " + r.Error + " (phase " + d.State.Phase + ")");
                        if (!r.Ok) break;
                        commands++;
                        steps++;
                        if (steps % 25 == 0) Invariants(d.State, ctx);
                    }
                    Check(d.State.Phase == PhaseId.GameOver, ctx + ": game did not finish (" + steps + " steps, phase " + d.State.Phase + ")");
                    Check(d.State.Year == Rules.NumYears, ctx + ": five years played");
                    Invariants(d.State, ctx + " final");
                    games++;
                }
            }
            string summary = $"Self-play: {games} games, {commands} commands, {failures} failures in {sw.Elapsed.TotalSeconds:F1}s";
            if (failures > 0) Debug.LogError(summary); else Debug.Log(summary);
            if (Application.isBatchMode) EditorApplication.Exit(failures > 0 ? 1 : 0);
        }

        private static void Invariants(GameState s, string ctx)
        {
            var stations = new int[6];
            var seen = new HashSet<(int, int)>();
            for (int t = 0; t < s.Map.Count; t++)
            {
                var ts = s.Territories[t];
                Check(ts.Mods >= 0, ctx + ": negative MODs");
                if (ts.Devastated) { Check(ts.Owner == -1 && ts.Units == 0 && !ts.SpaceStation, ctx + ": devastated territory occupied"); continue; }
                if (ts.Units > 0) Check(ts.Owner >= 0, ctx + ": units on unowned " + s.Map[t].Name);
                if (ts.Owner == -1) Check(ts.Units == 0 && !ts.SpaceStation, ctx + ": empty territory has pieces");
                if (ts.SpaceStation) { Check(s.Map[t].Type == TerritoryType.Land, ctx + ": station off land"); if (ts.Owner >= 0) stations[Math.Min(ts.Owner, 5)]++; }
                for (int c = 0; c < MapGraph.NumCommanders; c++)
                    if (ts.Commanders[c]) Check(seen.Add((ts.Owner, c)), ctx + ": duplicate commander");
            }
            for (int p = 0; p < s.NumPlayers; p++)
            {
                Check(s.Players[p].Energy >= 0, ctx + ": negative energy");
                Check(stations[p] <= Rules.MaxSpaceStations, ctx + ": more than 4 stations");
                if (s.Players[p].Eliminated) Check(s.CountUnits(p) == 0, ctx + ": eliminated player has units");
            }
        }
    }
}
