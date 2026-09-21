#include "risk2210/Agents.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace risk2210 {

// ============================================================================
// RandomAgent
// ============================================================================

bool RandomAgent::isBorder(const Game& g, int player, int t) const {
    for (int n : g.map().territory(t).adjacent) {
        const auto& ts = g.territory(n);
        if (!ts.devastated && ts.owner != player) return true;
    }
    return false;
}

std::vector<int> RandomAgent::borderTerritories(const Game& g, int player) const {
    std::vector<int> out;
    for (int t : g.ownedTerritories(player))
        if (isBorder(g, player, t)) out.push_back(t);
    if (out.empty()) out = g.ownedTerritories(player);
    return out;
}

int RandomAgent::chooseClaim(const Game& g, int player, const std::vector<int>& freeTerritories) {
    std::vector<int> adjacentToMine;
    for (int t : freeTerritories)
        for (int n : g.map().territory(t).adjacent)
            if (g.territory(n).owner == player) {
                adjacentToMine.push_back(t);
                break;
            }
    if (!adjacentToMine.empty() && std::uniform_int_distribution<int>(0, 3)(rng_) != 0) return pick(adjacentToMine);
    return pick(freeTerritories);
}

int RandomAgent::chooseInitialPlacement(const Game& g, int player) { return pick(borderTerritories(g, player)); }

InitialSetup RandomAgent::chooseInitialSetup(const Game& g, int player) {
    auto owned = g.ownedTerritories(player);
    int best = owned.front();
    for (int t : owned)
        if (g.territory(t).mods > g.territory(best).mods) best = t;
    InitialSetup s;
    s.spaceStation = best;
    s.landCommander = best;
    s.diplomat = pick(borderTerritories(g, player));
    return s;
}

int RandomAgent::bid(const Game& g, int player) {
    int e = g.player(player).energy;
    if (e <= 3) return 0;
    return std::uniform_int_distribution<int>(0, std::min(3, e / 3))(rng_);
}

int RandomAgent::chooseTurnOrder(const Game&, int, const std::vector<int>& availableMarkers) {
    return availableMarkers.front();
}

void RandomAgent::deployPhase(Game& g, int player) {
    auto border = borderTerritories(g, player);
    while (g.player(player).pool > 0) {
        int t = pick(border);
        int n = std::uniform_int_distribution<int>(1, g.player(player).pool)(rng_);
        if (!g.deployMods(player, t, n)) g.deployMods(player, g.ownedTerritories(player).front(), g.player(player).pool);
    }
}

void RandomAgent::purchasePhase(Game& g, int player) {
    const Commander priority[] = {Commander::Land, Commander::Naval, Commander::Nuclear, Commander::Space, Commander::Diplomat};
    auto border = borderTerritories(g, player);
    for (Commander c : priority) {
        if (g.player(player).energy < kCommanderCost + 1) break;
        if (!g.commanderInPlay(player, c)) g.hireCommander(player, c, pick(border));
    }
    if (g.player(player).energy >= kSpaceStationCost + 2 && g.countSpaceStations(player) < kMaxSpaceStations &&
        std::uniform_int_distribution<int>(0, 2)(rng_) == 0) {
        auto land = g.ownedTerritories(player, TerrType::Land);
        std::vector<int> noStation;
        for (int t : land)
            if (!g.territory(t).spaceStation) noStation.push_back(t);
        if (!noStation.empty()) g.buildSpaceStation(player, pick(noStation));
    }
    std::vector<Commander> inPlay;
    for (int c = 0; c < kNumCommanders; ++c)
        if (g.commanderInPlay(player, static_cast<Commander>(c)) && g.deckSize(static_cast<Commander>(c)) > 0)
            inPlay.push_back(static_cast<Commander>(c));
    if (inPlay.empty()) return;
    int spend = std::min({kMaxCardsPerTurn, g.player(player).energy - 1, 6 - static_cast<int>(g.player(player).hand.size())});
    std::vector<Commander> want;
    for (int i = 0; i < spend; ++i) want.push_back(pick(inPlay));
    // Do not request more cards than a deck holds.
    std::array<int, kNumCommanders> count{};
    std::vector<Commander> filtered;
    for (Commander c : want)
        if (++count[static_cast<int>(c)] <= g.deckSize(c)) filtered.push_back(c);
    if (!filtered.empty()) g.buyCards(player, filtered);
}

void RandomAgent::cardPhase(Game& g, int player) {
    bool played = true;
    while (played) {
        played = false;
        const auto& hand = g.player(player).hand;
        for (size_t i = 0; i < hand.size(); ++i) {
            const CardDef& c = Cards::def(hand[i]);
            if (c.timing != CardTiming::BeforeFirstInvasion) continue;
            if (c.kind == CardKind::Armageddon && g.score(player) >= 10) continue;  // don't nuke a lead
            if (g.player(player).energy - c.cost < 1) continue;
            if (g.playCard(player, static_cast<int>(i))) {
                played = true;
                break;
            }
        }
    }
}

void RandomAgent::invadePhase(Game& g, int player) {
    for (int invasions = 0; invasions < 12; ++invasions) {
        struct Option {
            int from, to;
        };
        std::vector<Option> opts;
        for (int from : g.ownedTerritories(player)) {
            const auto& f = g.territory(from);
            if (f.units() < 2) continue;
            std::vector<int> targets = g.map().territory(from).adjacent;
            if (f.spaceStation)
                for (int t = 0; t < g.map().size(); ++t)
                    if (g.map().territory(t).lunarLandingSite) targets.push_back(t);
            if (g.player(player).invadeEarthTarget >= 0) targets.push_back(g.player(player).invadeEarthTarget);
            for (int to : targets) {
                if (!g.canInvade(player, from, to)) continue;
                const auto& d = g.territory(to);
                if (d.units() == 0 || f.units() >= d.units() + 2) opts.push_back({from, to});
            }
        }
        if (opts.empty()) return;
        Option o = pick(opts);
        if (!g.declareInvasion(player, o.from, o.to)) continue;
        while (g.invasion().active && !g.invasion().captured) {
            int units = g.territory(o.from).units();
            if (units < 2) break;
            if (units <= g.territory(o.to).units() && g.invasion().attackedOnce) break;
            g.attack(player, std::min(3, units - 1));
        }
        if (g.invasion().active) {
            if (g.invasion().captured) {
                int maxMove = g.territory(o.from).units() - 1;
                int keepBehind = isBorder(g, player, o.from) ? std::min(maxMove, 2) : 0;
                g.moveIn(player, std::max(1, maxMove - keepBehind));
            } else {
                g.endInvasion(player);
            }
        }
    }
}

void RandomAgent::fortifyPhase(Game& g, int player) {
    int bestFrom = -1;
    for (int t : g.ownedTerritories(player))
        if (!isBorder(g, player, t) && g.territory(t).mods >= 2 && (bestFrom < 0 || g.territory(t).mods > g.territory(bestFrom).mods))
            bestFrom = t;
    if (bestFrom < 0) return;
    std::vector<int> dests;
    for (int t : borderTerritories(g, player))
        if (t != bestFrom && g.fortifyPathExists(player, bestFrom, t)) dests.push_back(t);
    if (dests.empty()) return;
    g.fortify(player, bestFrom, pick(dests), g.territory(bestFrom).mods - 1);
}

int RandomAgent::reactiveCard(const Game& g, int player, const Invasion& inv) {
    if (inv.defender != player) return -1;
    const auto& hand = g.player(player).hand;
    TerrType tt = g.map().territory(inv.to).type;
    int attackers = g.territory(inv.from).units();
    for (size_t i = 0; i < hand.size(); ++i) {
        const CardDef& c = Cards::def(hand[i]);
        if (c.timing != CardTiming::OnInvasionDeclared || !g.commanderInPlay(player, c.deck)) continue;
        if (c.kind == CardKind::StealthMods && c.target == tt) return static_cast<int>(i);
        if (c.kind == CardKind::CeaseFire && attackers >= 6 && g.player(player).energy >= c.cost + 1) return static_cast<int>(i);
    }
    return -1;
}

int RandomAgent::chooseTerritory(const Game& g, int player, const std::vector<int>& options, const std::string&) {
    std::vector<int> border;
    for (int t : options)
        if (g.territory(t).owner == player && isBorder(g, player, t)) border.push_back(t);
    return border.empty() ? pick(options) : pick(border);
}

// ============================================================================
// HumanCliAgent
// ============================================================================

namespace {
std::string lower(std::string s) {
    for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}
std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
}  // namespace

bool HumanCliAgent::readLine(std::string& line) {
    if (!std::getline(in_, line)) return false;
    line = trim(line);
    return true;
}

void HumanCliAgent::printTerritory(const Game& g, int t) const {
    const auto& info = g.map().territory(t);
    const auto& ts = g.territory(t);
    out_ << "  [" << t << "] " << info.name << " (" << toString(info.type) << ", " << g.map().region(info.region).name << ")";
    if (ts.devastated) {
        out_ << " DEVASTATED\n";
        return;
    }
    if (ts.owner < 0) out_ << " empty";
    else out_ << " " << g.player(ts.owner).name << " " << ts.mods << " MODs";
    for (int c = 0; c < kNumCommanders; ++c)
        if (ts.commanders[c]) out_ << " +" << toString(static_cast<Commander>(c));
    if (ts.spaceStation) out_ << " [Space Station]";
    out_ << "\n";
}

void HumanCliAgent::printOwned(const Game& g, int player) const {
    out_ << g.player(player).name << "'s territories:\n";
    for (int t : g.ownedTerritories(player)) printTerritory(g, t);
}

void HumanCliAgent::printHand(const Game& g, int player) const {
    const auto& hand = g.player(player).hand;
    out_ << "Hand (" << hand.size() << " cards):\n";
    for (size_t i = 0; i < hand.size(); ++i) {
        const CardDef& c = Cards::def(hand[i]);
        out_ << "  (" << i << ") " << c.name << " [" << toString(c.deck) << ", cost " << c.cost << "] " << c.text << "\n";
    }
}

void HumanCliAgent::printStatus(const Game& g, int player) const {
    const auto& ps = g.player(player);
    out_ << "\n" << ps.name << " | year " << g.year() << " | energy " << ps.energy << " | territories "
         << g.countTerritories(player) << " | score " << g.score(player) << " | commanders:";
    for (int c = 0; c < kNumCommanders; ++c)
        if (g.commanderInPlay(player, static_cast<Commander>(c))) out_ << " " << toString(static_cast<Commander>(c));
    out_ << " | stations " << g.countSpaceStations(player) << "\n";
}

int HumanCliAgent::readTerritory(const Game& g, const std::string& prompt) {
    for (;;) {
        out_ << prompt << " (id or name, 'list' for your territories, 'map' for all, 'done' to stop): ";
        std::string line;
        if (!readLine(line)) return -1;
        std::string l = lower(line);
        if (l == "done" || l.empty()) return -1;
        if (l == "list") {
            printOwned(g, g.currentPlayer() >= 0 ? g.currentPlayer() : 0);
            continue;
        }
        if (l == "map") {
            for (int t = 0; t < g.map().size(); ++t) printTerritory(g, t);
            continue;
        }
        if (std::all_of(l.begin(), l.end(), ::isdigit)) {
            int id = std::stoi(l);
            if (id >= 0 && id < g.map().size()) return id;
        }
        for (int t = 0; t < g.map().size(); ++t)
            if (lower(g.map().territory(t).name) == l) return t;
        out_ << "Unknown territory.\n";
    }
}

int HumanCliAgent::readInt(const std::string& prompt, int lo, int hi) {
    for (;;) {
        out_ << prompt << " [" << lo << "-" << hi << "]: ";
        std::string line;
        if (!readLine(line)) return lo;
        try {
            int v = std::stoi(line);
            if (v >= lo && v <= hi) return v;
        } catch (...) {
        }
        out_ << "Enter a number between " << lo << " and " << hi << ".\n";
    }
}

int HumanCliAgent::chooseClaim(const Game& g, int player, const std::vector<int>& freeTerritories) {
    out_ << "\n" << g.player(player).name << ", claim a territory. Free:\n";
    for (int t : freeTerritories) out_ << "  [" << t << "] " << g.map().territory(t).name << "\n";
    for (;;) {
        int t = readTerritory(g, "Claim");
        if (std::find(freeTerritories.begin(), freeTerritories.end(), t) != freeTerritories.end()) return t;
        if (!in_) return freeTerritories.front();
        out_ << "That territory is not available.\n";
    }
}

int HumanCliAgent::chooseInitialPlacement(const Game& g, int player) {
    out_ << "\n" << g.player(player).name << ", " << g.player(player).pool << " MODs left to place.\n";
    for (;;) {
        int t = readTerritory(g, "Place 1 MOD on");
        if (t >= 0 && g.territory(t).owner == player) return t;
        if (!in_) return g.ownedTerritories(player).front();
        out_ << "You must choose a territory you control.\n";
    }
}

InitialSetup HumanCliAgent::chooseInitialSetup(const Game& g, int player) {
    printOwned(g, player);
    InitialSetup s;
    auto ask = [&](const char* what) {
        for (;;) {
            int t = readTerritory(g, std::string("Place your ") + what + " in");
            if (t >= 0 && g.territory(t).owner == player) return t;
            if (!in_) return g.ownedTerritories(player).front();
            out_ << "You must choose a territory you control.\n";
        }
    };
    s.spaceStation = ask("Space Station");
    s.landCommander = ask("Land Commander");
    s.diplomat = ask("Diplomat");
    return s;
}

int HumanCliAgent::bid(const Game& g, int player) {
    printStatus(g, player);
    return readInt("Bid energy for turn order", 0, g.player(player).energy);
}

int HumanCliAgent::chooseTurnOrder(const Game& g, int player, const std::vector<int>& availableMarkers) {
    out_ << g.player(player).name << ", available turn markers:";
    for (int m : availableMarkers) out_ << " #" << (m + 1);
    out_ << "\n";
    for (;;) {
        int m = readInt("Choose marker", 1, static_cast<int>(availableMarkers.size()) + 5) - 1;
        if (std::find(availableMarkers.begin(), availableMarkers.end(), m) != availableMarkers.end()) return m;
        if (!in_) return availableMarkers.front();
    }
}

void HumanCliAgent::deployPhase(Game& g, int player) {
    printStatus(g, player);
    printOwned(g, player);
    while (g.player(player).pool > 0) {
        out_ << g.player(player).pool << " MODs to deploy.\n";
        int t = readTerritory(g, "Deploy to");
        if (t < 0) {
            if (!in_) return;
            continue;
        }
        int n = readInt("How many", 1, g.player(player).pool);
        Result r = g.deployMods(player, t, n);
        if (!r) out_ << r.error << "\n";
    }
}

void HumanCliAgent::purchasePhase(Game& g, int player) {
    for (;;) {
        printStatus(g, player);
        out_ << "Purchase: (c)ommander 3E, (s)pace station 5E, (b)uy cards 1E each, (l)ist, (d)one: ";
        std::string line;
        if (!readLine(line)) return;
        std::string l = lower(line);
        if (l == "d" || l == "done" || l.empty()) return;
        if (l == "l") {
            printOwned(g, player);
            continue;
        }
        if (l == "c") {
            out_ << "Commanders: 0=Land 1=Diplomat 2=Naval 3=Nuclear 4=Space\n";
            Commander c = static_cast<Commander>(readInt("Which commander", 0, 4));
            int t = readTerritory(g, "Place in");
            if (t < 0) continue;
            Result r = g.hireCommander(player, c, t);
            if (!r) out_ << r.error << "\n";
        } else if (l == "s") {
            int t = readTerritory(g, "Build Space Station in");
            if (t < 0) continue;
            Result r = g.buildSpaceStation(player, t);
            if (!r) out_ << r.error << "\n";
        } else if (l == "b") {
            out_ << "Decks: 0=Land 1=Diplomat 2=Naval 3=Nuclear 4=Space (sizes:";
            for (int c = 0; c < kNumCommanders; ++c) out_ << " " << g.deckSize(static_cast<Commander>(c));
            out_ << ")\nEnter deck numbers separated by spaces (up to " << (kMaxCardsPerTurn - g.player(player).cardsBoughtThisTurn) << "): ";
            std::string ln;
            if (!readLine(ln)) return;
            std::istringstream is(ln);
            std::vector<Commander> decks;
            int d;
            while (is >> d)
                if (d >= 0 && d < kNumCommanders) decks.push_back(static_cast<Commander>(d));
            Result r = g.buyCards(player, decks);
            if (!r) out_ << r.error << "\n";
            else printHand(g, player);
        }
    }
}

void HumanCliAgent::cardPhase(Game& g, int player) {
    for (;;) {
        if (g.player(player).hand.empty()) return;
        printHand(g, player);
        out_ << "Play a card by number, or 'done': ";
        std::string line;
        if (!readLine(line)) return;
        std::string l = lower(line);
        if (l == "done" || l.empty()) return;
        try {
            Result r = g.playCard(player, std::stoi(l));
            if (!r) out_ << r.error << "\n";
        } catch (...) {
        }
    }
}

void HumanCliAgent::invadePhase(Game& g, int player) {
    for (;;) {
        printStatus(g, player);
        out_ << "Invade: (i)nvade, (p)lay card, (l)ist, (m)ap, (d)one: ";
        std::string line;
        if (!readLine(line)) return;
        std::string l = lower(line);
        if (l == "d" || l == "done" || l.empty()) return;
        if (l == "l") {
            printOwned(g, player);
            continue;
        }
        if (l == "m") {
            for (int t = 0; t < g.map().size(); ++t) printTerritory(g, t);
            continue;
        }
        if (l == "p") {
            cardPhase(g, player);
            continue;
        }
        if (l != "i") continue;
        int from = readTerritory(g, "Attack from");
        if (from < 0) continue;
        int to = readTerritory(g, "Attack");
        if (to < 0) continue;
        Result r = g.declareInvasion(player, from, to);
        if (!r) {
            out_ << r.error << "\n";
            continue;
        }
        while (g.invasion().active && !g.invasion().captured) {
            printTerritory(g, from);
            printTerritory(g, to);
            int maxDice = std::min(3, g.territory(from).units() - 1);
            if (maxDice < 1) {
                g.endInvasion(player);
                break;
            }
            int n = readInt("Roll how many dice (0 to retreat)", 0, maxDice);
            if (n == 0) {
                Result e = g.endInvasion(player);
                if (!e) out_ << e.error << "\n";
                else break;
                continue;
            }
            BattleResult br;
            g.attack(player, n, &br);
            out_ << "  You:";
            for (int d : br.attackRolls) out_ << " " << d;
            out_ << "  Defender:";
            for (int d : br.defendRolls) out_ << " " << d;
            out_ << "  => you lose " << br.attackerLost << ", they lose " << br.defenderLost << "\n";
        }
        if (g.invasion().active && g.invasion().captured) {
            int maxMove = g.territory(from).units() - 1;
            int minMove = std::max(1, std::min(g.invasion().lastDice, maxMove));
            out_ << "Captured " << g.map().territory(to).name << "!\n";
            int n = readInt("Move in how many units", minMove, maxMove);
            Result m = g.moveIn(player, n);
            if (!m) out_ << m.error << "\n";
        }
    }
}

void HumanCliAgent::fortifyPhase(Game& g, int player) {
    printStatus(g, player);
    out_ << "Fortify (one move). Press enter at the prompt to skip.\n";
    int from = readTerritory(g, "Fortify from");
    if (from < 0) return;
    int to = readTerritory(g, "Fortify to");
    if (to < 0) return;
    const auto& f = g.territory(from);
    int n = readInt("How many MODs", 0, std::max(0, std::min(f.mods, f.units() - 1)));
    std::array<bool, kNumCommanders> cmds{};
    for (int c = 0; c < kNumCommanders; ++c)
        if (f.commanders[c]) {
            out_ << "Move the " << toString(static_cast<Commander>(c)) << " Commander too? (y/n): ";
            std::string line;
            if (readLine(line) && lower(line) == "y") cmds[c] = true;
        }
    Result r = g.fortify(player, from, to, n, cmds);
    if (!r) out_ << r.error << "\n";
}

int HumanCliAgent::chooseDefenseDice(const Game& g, int player, const Invasion& inv, int maxDice) {
    if (maxDice <= 1) return maxDice;
    out_ << "\n" << g.player(player).name << ": " << g.player(inv.attacker).name << " attacks "
         << g.map().territory(inv.to).name << " (" << g.territory(inv.to).units() << " units) from "
         << g.map().territory(inv.from).name << " (" << g.territory(inv.from).units() << " units)\n";
    return readInt("Defend with how many dice", 1, maxDice);
}

int HumanCliAgent::reactiveCard(const Game& g, int player, const Invasion& inv) {
    const auto& hand = g.player(player).hand;
    std::vector<int> playable;
    for (size_t i = 0; i < hand.size(); ++i)
        if (Cards::def(hand[i]).timing == CardTiming::OnInvasionDeclared) playable.push_back(static_cast<int>(i));
    if (playable.empty()) return -1;
    out_ << "\n" << g.player(player).name << ": " << g.player(inv.attacker).name << " declared an invasion of "
         << g.map().territory(inv.to).name << " from " << g.map().territory(inv.from).name << ".\n";
    printHand(g, player);
    out_ << "Play a reactive card by number, or 'pass': ";
    std::string line;
    if (!readLine(line) || lower(line) == "pass" || line.empty()) return -1;
    try {
        return std::stoi(line);
    } catch (...) {
        return -1;
    }
}

Commander HumanCliAgent::chooseBonusDeck(const Game&, int, const std::vector<Commander>& options) {
    out_ << "3-territory bonus! Choose a deck:";
    for (size_t i = 0; i < options.size(); ++i) out_ << " " << i << "=" << toString(options[i]);
    out_ << "\n";
    return options[readInt("Deck", 0, static_cast<int>(options.size()) - 1)];
}

int HumanCliAgent::chooseTerritory(const Game& g, int, const std::vector<int>& options, const std::string& prompt) {
    out_ << prompt << ". Options:\n";
    for (int t : options) printTerritory(g, t);
    for (;;) {
        int t = readTerritory(g, prompt);
        if (std::find(options.begin(), options.end(), t) != options.end()) return t;
        if (t < 0 || !in_) return options.front();
        out_ << "Not an option.\n";
    }
}

}  // namespace risk2210
