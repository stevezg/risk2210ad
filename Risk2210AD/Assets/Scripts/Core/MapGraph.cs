using System;
using System.Collections.Generic;
using UnityEngine;

namespace Risk2210.Core
{
    public enum TerritoryType { Land, Water, Moon }

    public enum CommanderType { Land = 0, Diplomat = 1, Naval = 2, Nuclear = 3, Space = 4 }

    public enum EdgeKind { Land, Water, Moon }

    /// <summary>A node of the map graph: one territory of the board.</summary>
    public sealed class TerritoryNode
    {
        public int Id;
        public string Name;
        public TerritoryType Type;
        public int RegionId;
        public bool LunarLandingSite;   // Sea of Crisis, Bay of Dew, Tycho
        public Vector2 Position;        // world-space layout position (board units)
        public readonly List<int> Neighbors = new List<int>();
    }

    /// <summary>An edge of the map graph: a border between two territories.</summary>
    public sealed class BorderEdge
    {
        public int A;
        public int B;
        public EdgeKind Kind;
        public bool WrapsAround;        // Pacific links that leave one board edge and re-enter on the other
    }

    /// <summary>A continent, water colony or lunar colony with its control bonus.</summary>
    public sealed class Region
    {
        public int Id;
        public string Name;
        public TerritoryType Type;
        public int Bonus;
        public readonly List<int> Territories = new List<int>();
    }

    /// <summary>The Risk 2210 A.D. board as a graph: 42 land, 13 water and 14 lunar territories.</summary>
    public sealed class MapGraph
    {
        public const int NumCommanders = 5;

        public readonly List<TerritoryNode> Territories = new List<TerritoryNode>();
        public readonly List<BorderEdge> Edges = new List<BorderEdge>();
        public readonly List<Region> Regions = new List<Region>();

        public int Count => Territories.Count;
        public TerritoryNode this[int id] => Territories[id];

        public int Find(string name)
        {
            for (int i = 0; i < Territories.Count; i++)
                if (Territories[i].Name == name) return i;
            return -1;
        }

        public bool AreAdjacent(int a, int b) => Territories[a].Neighbors.Contains(b);

        public List<int> TerritoriesOfType(TerritoryType type)
        {
            var list = new List<int>();
            foreach (var t in Territories) if (t.Type == type) list.Add(t.Id);
            return list;
        }

        public Rect Bounds(TerritoryType? type = null)
        {
            float minx = float.MaxValue, miny = float.MaxValue, maxx = float.MinValue, maxy = float.MinValue;
            foreach (var t in Territories)
            {
                if (type.HasValue && t.Type != type.Value) continue;
                minx = Mathf.Min(minx, t.Position.x); miny = Mathf.Min(miny, t.Position.y);
                maxx = Mathf.Max(maxx, t.Position.x); maxy = Mathf.Max(maxy, t.Position.y);
            }
            return Rect.MinMaxRect(minx, miny, maxx, maxy);
        }

        /// <summary>Breadth-first shortest path (list of territory ids) or an empty list.</summary>
        public List<int> ShortestPath(int from, int to, Func<int, bool> passable = null)
        {
            var prev = new int[Count];
            for (int i = 0; i < Count; i++) prev[i] = -2;
            var q = new Queue<int>();
            q.Enqueue(from); prev[from] = -1;
            while (q.Count > 0)
            {
                int cur = q.Dequeue();
                if (cur == to) break;
                foreach (int n in Territories[cur].Neighbors)
                {
                    if (prev[n] != -2) continue;
                    if (passable != null && !passable(n) && n != to) continue;
                    prev[n] = cur; q.Enqueue(n);
                }
            }
            var path = new List<int>();
            if (prev[to] == -2) return path;
            for (int c = to; c != -1; c = prev[c]) path.Add(c);
            path.Reverse();
            return path;
        }

        // ------------------------------------------------------------------
        // Construction
        // ------------------------------------------------------------------

        private int AddRegion(string name, TerritoryType type, int bonus)
        {
            Regions.Add(new Region { Id = Regions.Count, Name = name, Type = type, Bonus = bonus });
            return Regions.Count - 1;
        }

        private void AddTerritory(string name, TerritoryType type, int region, float x, float y)
        {
            var t = new TerritoryNode { Id = Territories.Count, Name = name, Type = type, RegionId = region, Position = new Vector2(x, y) };
            Territories.Add(t);
            Regions[region].Territories.Add(t.Id);
        }

        private void Link(string a, string b, bool wraps = false)
        {
            int ia = Find(a), ib = Find(b);
            if (ia < 0 || ib < 0) throw new InvalidOperationException("Unknown territory in link: " + a + " / " + b);
            if (AreAdjacent(ia, ib)) return;
            Territories[ia].Neighbors.Add(ib);
            Territories[ib].Neighbors.Add(ia);
            var ta = Territories[ia].Type; var tb = Territories[ib].Type;
            var kind = (ta == TerritoryType.Water || tb == TerritoryType.Water) ? EdgeKind.Water
                     : (ta == TerritoryType.Moon ? EdgeKind.Moon : EdgeKind.Land);
            Edges.Add(new BorderEdge { A = ia, B = ib, Kind = kind, WrapsAround = wraps });
        }

        /// <summary>
        /// Builds the standard board. Layout positions are in board units (x to the right, y up);
        /// Earth spans roughly x 0..20, y 0..-12 and the Moon sits below it.
        /// </summary>
        public static MapGraph CreateStandard()
        {
            var m = new MapGraph();
            const float S = 1f / 50f;   // raw layout table is in pixels; convert to board units, flip y

            // ---- Continents ----------------------------------------------------
            int na = m.AddRegion("North America", TerritoryType.Land, 5);
            foreach (var p in new[] {
                ("Aleutian Empire", 75f, 95f), ("Nunavut", 160f, 70f), ("Exiled States of America", 310f, 55f), ("Alberta", 140f, 140f),
                ("Canada", 215f, 150f), ("Republique du Quebec", 300f, 110f), ("Continental Biospheres", 140f, 205f),
                ("American Republic", 250f, 225f), ("Mexitlopoctli", 175f, 290f) })
                m.AddTerritory(p.Item1, TerritoryType.Land, na, p.Item2 * S, -p.Item3 * S);

            int sa = m.AddRegion("South America", TerritoryType.Land, 2);
            foreach (var p in new[] { ("Nuevo Timoto", 235f, 355f), ("Andean Nations", 215f, 425f), ("Amazon Desert", 290f, 415f), ("Argentina", 245f, 500f) })
                m.AddTerritory(p.Item1, TerritoryType.Land, sa, p.Item2 * S, -p.Item3 * S);

            int eu = m.AddRegion("Europe", TerritoryType.Land, 5);
            foreach (var p in new[] { ("Iceland GRC", 400f, 110f), ("New Avalon", 405f, 175f), ("Jotenheim", 475f, 90f), ("Warsaw Republic", 495f, 165f),
                ("Ukrayina", 575f, 140f), ("Andorra", 415f, 240f), ("Imperial Balkania", 505f, 230f) })
                m.AddTerritory(p.Item1, TerritoryType.Land, eu, p.Item2 * S, -p.Item3 * S);

            int af = m.AddRegion("Africa", TerritoryType.Land, 3);
            foreach (var p in new[] { ("Saharan Empire", 445f, 330f), ("Egypt", 515f, 300f), ("Ministry of Djibouti", 565f, 375f),
                ("Zaire Military Zone", 500f, 425f), ("Lesotho", 510f, 505f), ("Madagascar", 585f, 495f) })
                m.AddTerritory(p.Item1, TerritoryType.Land, af, p.Item2 * S, -p.Item3 * S);

            int asia = m.AddRegion("Asia", TerritoryType.Land, 7);
            foreach (var p in new[] { ("Middle East", 605f, 275f), ("Afghanistan", 645f, 205f), ("Enclave of the Bear", 645f, 135f), ("Siberia", 705f, 80f),
                ("Sakha", 785f, 60f), ("Pevek", 865f, 75f), ("Alden", 765f, 135f), ("Khan Industrial State", 785f, 195f), ("Japan", 885f, 195f),
                ("Hong Kong", 735f, 255f), ("United Indiastan", 665f, 305f), ("Angkhor Wat", 745f, 325f) })
                m.AddTerritory(p.Item1, TerritoryType.Land, asia, p.Item2 * S, -p.Item3 * S);

            int au = m.AddRegion("Australia", TerritoryType.Land, 2);
            foreach (var p in new[] { ("Java Cartel", 785f, 415f), ("New Guinea", 875f, 405f), ("Aboriginal League", 795f, 505f), ("Australian Testing Ground", 900f, 525f) })
                m.AddTerritory(p.Item1, TerritoryType.Land, au, p.Item2 * S, -p.Item3 * S);

            // ---- Water colonies ------------------------------------------------
            int usp = m.AddRegion("US Pacific", TerritoryType.Water, 2);
            foreach (var p in new[] { ("Poseidon", 45f, 205f), ("Hawaiian Preserve", 55f, 285f), ("New Atlantis", 65f, 365f) })
                m.AddTerritory(p.Item1, TerritoryType.Water, usp, p.Item2 * S, -p.Item3 * S);
            int asp = m.AddRegion("Asia Pacific", TerritoryType.Water, 1);
            foreach (var p in new[] { ("Sung Tzu", 940f, 345f), ("Neo Tokyo", 940f, 275f) })
                m.AddTerritory(p.Item1, TerritoryType.Water, asp, p.Item2 * S, -p.Item3 * S);
            int nat = m.AddRegion("Northern Atlantic", TerritoryType.Water, 2);
            foreach (var p in new[] { ("Western Ireland", 350f, 215f), ("New York City", 320f, 265f), ("Nova Brasilia", 335f, 335f) })
                m.AddTerritory(p.Item1, TerritoryType.Water, nat, p.Item2 * S, -p.Item3 * S);
            int sat = m.AddRegion("Southern Atlantic", TerritoryType.Water, 1);
            foreach (var p in new[] { ("Neo Paulo", 335f, 480f), ("The Ivory Reef", 405f, 465f) })
                m.AddTerritory(p.Item1, TerritoryType.Water, sat, p.Item2 * S, -p.Item3 * S);
            int ind = m.AddRegion("Indian", TerritoryType.Water, 2);
            foreach (var p in new[] { ("Akara", 695f, 515f), ("South Ceylon", 640f, 385f), ("Microcorp", 640f, 455f) })
                m.AddTerritory(p.Item1, TerritoryType.Water, ind, p.Item2 * S, -p.Item3 * S);

            // ---- Lunar colonies -------------------------------------------------
            int cre = m.AddRegion("Cresinion", TerritoryType.Moon, 2);
            foreach (var p in new[] { ("Harpalus", 90f, 700f), ("Sea of Rains", 185f, 705f), ("Ocean of Storms", 195f, 800f), ("Bay of Dew", 90f, 790f) })
                m.AddTerritory(p.Item1, TerritoryType.Moon, cre, p.Item2 * S, -p.Item3 * S);
            int del = m.AddRegion("Delphot", TerritoryType.Moon, 2);
            foreach (var p in new[] { ("Aristotle", 320f, 680f), ("Sea of Serenity", 345f, 750f), ("Sea of Crisis", 445f, 715f), ("Sea of Nectar", 425f, 805f) })
                m.AddTerritory(p.Item1, TerritoryType.Moon, del, p.Item2 * S, -p.Item3 * S);
            int saj = m.AddRegion("Sajon", TerritoryType.Moon, 3);
            foreach (var p in new[] { ("Rhaeticus", 575f, 735f), ("Byrgius", 575f, 835f), ("Sea of Clouds", 675f, 775f), ("Straight Wall", 755f, 710f),
                ("Marsh of Diseases", 735f, 845f), ("Tycho", 850f, 800f) })
                m.AddTerritory(p.Item1, TerritoryType.Moon, saj, p.Item2 * S, -p.Item3 * S);

            foreach (var n in new[] { "Sea of Crisis", "Bay of Dew", "Tycho" }) m.Territories[m.Find(n)].LunarLandingSite = true;

            // ---- Land borders (classic topology with 2210 renames; the East Africa–Middle East
            //      and Greenland–Ontario bridges are absent on the 2210 board) ---------------
            string[,] land = {
                {"Aleutian Empire","Nunavut"},{"Aleutian Empire","Alberta"},{"Aleutian Empire","Pevek"},
                {"Nunavut","Alberta"},{"Nunavut","Canada"},{"Nunavut","Exiled States of America"},
                {"Exiled States of America","Republique du Quebec"},{"Exiled States of America","Iceland GRC"},
                {"Alberta","Canada"},{"Alberta","Continental Biospheres"},
                {"Canada","Republique du Quebec"},{"Canada","Continental Biospheres"},{"Canada","American Republic"},
                {"Republique du Quebec","American Republic"},
                {"Continental Biospheres","American Republic"},{"Continental Biospheres","Mexitlopoctli"},
                {"American Republic","Mexitlopoctli"},{"Mexitlopoctli","Nuevo Timoto"},
                {"Nuevo Timoto","Andean Nations"},{"Nuevo Timoto","Amazon Desert"},
                {"Andean Nations","Amazon Desert"},{"Andean Nations","Argentina"},
                {"Amazon Desert","Argentina"},{"Amazon Desert","Saharan Empire"},
                {"Iceland GRC","New Avalon"},{"Iceland GRC","Jotenheim"},
                {"New Avalon","Jotenheim"},{"New Avalon","Warsaw Republic"},{"New Avalon","Andorra"},
                {"Jotenheim","Warsaw Republic"},{"Jotenheim","Ukrayina"},
                {"Warsaw Republic","Andorra"},{"Warsaw Republic","Imperial Balkania"},{"Warsaw Republic","Ukrayina"},
                {"Ukrayina","Imperial Balkania"},{"Ukrayina","Enclave of the Bear"},{"Ukrayina","Afghanistan"},{"Ukrayina","Middle East"},
                {"Andorra","Imperial Balkania"},{"Andorra","Saharan Empire"},
                {"Imperial Balkania","Middle East"},{"Imperial Balkania","Egypt"},
                {"Saharan Empire","Egypt"},{"Saharan Empire","Ministry of Djibouti"},{"Saharan Empire","Zaire Military Zone"},
                {"Egypt","Middle East"},{"Egypt","Ministry of Djibouti"},
                {"Ministry of Djibouti","Zaire Military Zone"},{"Ministry of Djibouti","Lesotho"},{"Ministry of Djibouti","Madagascar"},
                {"Zaire Military Zone","Lesotho"},{"Lesotho","Madagascar"},
                {"Middle East","Afghanistan"},{"Middle East","United Indiastan"},
                {"Afghanistan","Enclave of the Bear"},{"Afghanistan","Hong Kong"},{"Afghanistan","United Indiastan"},
                {"Enclave of the Bear","Siberia"},{"Enclave of the Bear","Hong Kong"},
                {"Siberia","Sakha"},{"Siberia","Alden"},{"Siberia","Khan Industrial State"},{"Siberia","Hong Kong"},
                {"Sakha","Pevek"},{"Sakha","Alden"},
                {"Pevek","Alden"},{"Pevek","Khan Industrial State"},{"Pevek","Japan"},
                {"Alden","Khan Industrial State"},
                {"Khan Industrial State","Japan"},{"Khan Industrial State","Hong Kong"},
                {"Hong Kong","Angkhor Wat"},{"Hong Kong","United Indiastan"},
                {"United Indiastan","Angkhor Wat"},{"Angkhor Wat","Java Cartel"},
                {"Java Cartel","New Guinea"},{"Java Cartel","Aboriginal League"},
                {"New Guinea","Aboriginal League"},{"New Guinea","Australian Testing Ground"},
                {"Aboriginal League","Australian Testing Ground"},
            };
            for (int i = 0; i < land.GetLength(0); i++) m.Link(land[i, 0], land[i, 1]);

            // ---- Water lines (from the printed board) ---------------------------
            string[,] water = {
                {"Poseidon","Aleutian Empire"},{"Poseidon","Hawaiian Preserve"},
                {"Hawaiian Preserve","Mexitlopoctli"},{"Hawaiian Preserve","New Atlantis"},
                {"New Atlantis","Nuevo Timoto"},
                {"Neo Tokyo","Japan"},{"Neo Tokyo","Hong Kong"},{"Neo Tokyo","Sung Tzu"},
                {"Sung Tzu","Java Cartel"},
                {"Western Ireland","New Avalon"},{"Western Ireland","New York City"},
                {"New York City","American Republic"},{"New York City","Nova Brasilia"},
                {"Nova Brasilia","Nuevo Timoto"},{"Nova Brasilia","Saharan Empire"},
                {"Neo Paulo","Amazon Desert"},{"Neo Paulo","The Ivory Reef"},
                {"The Ivory Reef","Saharan Empire"},
                {"South Ceylon","United Indiastan"},{"South Ceylon","Microcorp"},
                {"Microcorp","Madagascar"},{"Microcorp","Akara"},
                {"Akara","Aboriginal League"},
            };
            for (int i = 0; i < water.GetLength(0); i++) m.Link(water[i, 0], water[i, 1]);
            m.Link("Hawaiian Preserve", "Neo Tokyo", wraps: true);
            m.Link("New Atlantis", "Sung Tzu", wraps: true);

            // ---- Lunar borders ---------------------------------------------------
            string[,] moon = {
                {"Harpalus","Bay of Dew"},{"Harpalus","Sea of Rains"},
                {"Sea of Rains","Bay of Dew"},{"Sea of Rains","Ocean of Storms"},{"Sea of Rains","Sea of Serenity"},{"Sea of Rains","Aristotle"},
                {"Ocean of Storms","Bay of Dew"},{"Ocean of Storms","Rhaeticus"},{"Ocean of Storms","Byrgius"},
                {"Aristotle","Sea of Serenity"},
                {"Sea of Serenity","Sea of Crisis"},{"Sea of Serenity","Sea of Nectar"},{"Sea of Serenity","Rhaeticus"},
                {"Sea of Crisis","Sea of Nectar"},
                {"Sea of Nectar","Rhaeticus"},{"Sea of Nectar","Straight Wall"},
                {"Rhaeticus","Sea of Clouds"},{"Rhaeticus","Byrgius"},
                {"Byrgius","Sea of Clouds"},{"Byrgius","Marsh of Diseases"},
                {"Sea of Clouds","Straight Wall"},{"Sea of Clouds","Marsh of Diseases"},{"Sea of Clouds","Tycho"},
                {"Straight Wall","Tycho"},{"Marsh of Diseases","Tycho"},
            };
            for (int i = 0; i < moon.GetLength(0); i++) m.Link(moon[i, 0], moon[i, 1]);

            return m;
        }
    }
}
