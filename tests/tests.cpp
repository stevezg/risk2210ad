// Self-play invariant tests for the Risk 2210 A.D. engine.
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "risk2210/Agents.h"
#include "risk2210/Game.h"

using namespace risk2210;

static int failures = 0;
#define CHECK(cond, msg)                                                                      \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n";  \
            ++failures;                                                                      \
        }                                                                                    \
    } while (0)

static void testMap() {
    Map m = Map::standard();
    CHECK(m.territoriesOfType(TerrType::Land).size() == 42, "42 land territories");
    CHECK(m.territoriesOfType(TerrType::Water).size() == 13, "13 water territories");
    CHECK(m.territoriesOfType(TerrType::Moon).size() == 14, "14 lunar territories");
    CHECK(m.regions().size() == 6 + 5 + 3, "14 regions");
    int landing = 0;
    for (const auto& t : m.territories()) {
        CHECK(!t.adjacent.empty(), "territory " + t.name + " has neighbours");
        for (int n : t.adjacent) CHECK(m.adjacent(n, t.id), "adjacency symmetric for " + t.name);
        if (t.lunarLandingSite) ++landing;
    }
    CHECK(landing == 3, "three lunar landing sites");
    CHECK(m.adjacent(m.find("Egypt"), m.find("Middle East")), "Egypt-Middle East link");
    CHECK(!m.adjacent(m.find("Ministry of Djibouti"), m.find("Middle East")), "no Djibouti-Middle East link");
    CHECK(m.adjacent(m.find("Aleutian Empire"), m.find("Pevek")), "Bering strait link");
}

static void checkInvariants(const Game& g, const std::string& ctx) {
    int stations[6] = {0};
    std::set<std::pair<int, int>> commandersSeen;
    for (int t = 0; t < g.map().size(); ++t) {
        const auto& ts = g.territory(t);
        CHECK(ts.mods >= 0, ctx + ": negative MODs");
        if (ts.devastated) {
            CHECK(ts.owner == -1 && ts.units() == 0 && !ts.spaceStation, ctx + ": devastated territory occupied");
            continue;
        }
        if (ts.units() > 0) CHECK(ts.owner >= 0, ctx + ": units on unowned territory " + g.map().territory(t).name);
        if (ts.owner == -1) CHECK(ts.units() == 0 && !ts.spaceStation, ctx + ": empty territory has stuff");
        if (ts.spaceStation) {
            CHECK(g.map().territory(t).type == TerrType::Land, ctx + ": station off land");
            ++stations[ts.owner];
        }
        for (int c = 0; c < kNumCommanders; ++c)
            if (ts.commanders[c]) {
                bool fresh = commandersSeen.insert({ts.owner, c}).second;
                CHECK(fresh, ctx + ": duplicate commander for a player");
            }
    }
    for (int p = 0; p < g.numPlayers(); ++p) {
        CHECK(g.player(p).energy >= 0, ctx + ": negative energy");
        CHECK(stations[p] <= kMaxSpaceStations, ctx + ": more than 4 space stations");
        if (g.player(p).eliminated) CHECK(g.countUnits(p) == 0, ctx + ": eliminated player has units");
    }
}

static void testSelfPlay() {
    int games = 0;
    for (int players = 2; players <= 5; ++players) {
        for (unsigned seed = 1; seed <= 40; ++seed) {
            std::vector<std::string> names;
            for (int p = 0; p < players; ++p) names.push_back("P" + std::to_string(p));
            Game g(names, seed * 1000 + players);
            std::vector<std::unique_ptr<RandomAgent>> agents;
            for (int p = 0; p < players; ++p) {
                agents.push_back(std::make_unique<RandomAgent>(seed + p));
                g.setAgent(p, agents.back().get());
            }
            std::string ctx = "players=" + std::to_string(players) + " seed=" + std::to_string(seed);
            g.setup();
            checkInvariants(g, ctx + " setup");
            int devastated = 0;
            for (const auto& t : g.territories()) devastated += t.devastated ? 1 : 0;
            CHECK(devastated == kDevastationMarkers, ctx + ": four devastation markers");
            for (int p = 0; p < players; ++p) {
                CHECK(g.countSpaceStations(p) == 1, ctx + ": one starting station");
                CHECK(g.commanderInPlay(p, Commander::Land) && g.commanderInPlay(p, Commander::Diplomat), ctx + ": starting commanders");
                CHECK(g.player(p).pool == 0, ctx + ": all starting MODs placed");
            }
            for (int y = 0; y < kNumYears; ++y) {
                g.startYear();
                CHECK(static_cast<int>(g.turnOrder().size()) <= players, ctx + ": turn order size");
                for (int p : g.turnOrder()) {
                    if (!g.isActive(p)) continue;
                    g.takeTurn(p);
                    CHECK(!g.invasion().active, ctx + ": invasion left open");
                    CHECK(g.player(p).pool == 0, ctx + ": MODs left undeployed");
                    checkInvariants(g, ctx + " year " + std::to_string(g.year()));
                }
            }
            g.finalScoring();
            CHECK(g.year() == kNumYears, ctx + ": five years played");
            CHECK(g.phase() == Phase::GameOver, ctx + ": game over");
            for (int p = 0; p < players; ++p)
                CHECK(g.player(p).finalScore >= g.countTerritories(p), ctx + ": score at least territory count");
            ++games;
        }
    }
    std::cout << "self-play: " << games << " games completed\n";
}

static void testCombatRules() {
    Game g({"A", "B", "C"}, 7);
    RandomAgent a(1), b(2), c(3);
    g.setAgent(0, &a);
    g.setAgent(1, &b);
    g.setAgent(2, &c);
    g.setup();
    const Map& m = g.map();
    // Water requires a Naval Commander; the Moon requires a Space Commander + station + landing site.
    int water = m.find("Poseidon");
    int aleutian = m.find("Aleutian Empire");
    std::string why;
    if (g.territory(aleutian).owner == 0 && g.territory(aleutian).units() >= 2) {
        bool ok = g.canInvade(0, aleutian, water, &why);
        CHECK(!ok && why.find("Naval") != std::string::npos, "water invasion needs Naval Commander");
    }
    CHECK(!g.canInvade(0, aleutian, m.find("Tycho"), &why), "moon invasion blocked without Space Commander");
    CHECK(g.income(0) >= 3, "minimum income of 3");
}

int main() {
    testMap();
    testCombatRules();
    testSelfPlay();
    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all tests passed\n";
    return 0;
}
