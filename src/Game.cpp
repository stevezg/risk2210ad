#include "risk2210/Game.h"

#include <algorithm>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace risk2210 {

namespace {
int typeIndex(TerrType t) { return static_cast<int>(t); }
int ci(Commander c) { return static_cast<int>(c); }
}  // namespace

// ============================================================================
// Construction / logging
// ============================================================================

Game::Game(std::vector<std::string> playerNames, unsigned seed) : map_(Map::standard()), rng_(seed) {
    numPlayers_ = static_cast<int>(playerNames.size());
    if (numPlayers_ < 2 || numPlayers_ > 5) throw std::invalid_argument("Risk 2210 supports 2-5 players");
    for (auto& n : playerNames) {
        PlayerState ps;
        ps.name = std::move(n);
        players_.push_back(ps);
    }
    if (numPlayers_ == 2) {
        PlayerState neutral;
        neutral.name = "Neutral";
        neutral.neutral = true;
        neutral.energy = 0;
        players_.push_back(neutral);
    }
    agents_.assign(players_.size(), nullptr);
    terrs_.assign(map_.size(), TerritoryState{});

    for (int c = 0; c < kNumCommanders; ++c) {
        decks_[c] = Cards::deckFor(static_cast<Commander>(c));
        std::shuffle(decks_[c].begin(), decks_[c].end(), rng_);
    }
    for (TerrType t : {TerrType::Land, TerrType::Water, TerrType::Moon}) {
        terrDecks_[typeIndex(t)] = map_.territoriesOfType(t);
        std::shuffle(terrDecks_[typeIndex(t)].begin(), terrDecks_[typeIndex(t)].end(), rng_);
    }
}

void Game::logf(const std::string& s) {
    log_.push_back(s);
    if (logger_) logger_(s);
}

int Game::drawTerritoryCard(TerrType type) {
    auto& deck = terrDecks_[typeIndex(type)];
    auto& pos = terrDeckPos_[typeIndex(type)];
    if (pos >= deck.size()) {
        std::shuffle(deck.begin(), deck.end(), rng_);
        pos = 0;
    }
    return deck[pos++];
}

int Game::drawCommandCard(Commander c) {
    auto& deck = decks_[ci(c)];
    if (deck.empty()) return -1;
    int id = deck.back();
    deck.pop_back();
    return id;
}

// ============================================================================
// Queries
// ============================================================================

bool Game::commanderInPlay(int p, Commander c) const {
    for (const auto& t : terrs_)
        if (t.owner == p && t.hasCommander(c)) return true;
    return false;
}

int Game::countTerritories(int p) const {
    int n = 0;
    for (const auto& t : terrs_) n += (t.owner == p) ? 1 : 0;
    return n;
}

int Game::countUnits(int p) const {
    int n = 0;
    for (const auto& t : terrs_)
        if (t.owner == p) n += t.units();
    return n;
}

int Game::countSpaceStations(int p) const {
    int n = 0;
    for (const auto& t : terrs_) n += (t.owner == p && t.spaceStation) ? 1 : 0;
    return n;
}

bool Game::controlsRegion(int p, int region) const {
    bool any = false;
    for (int t : map_.region(region).territories) {
        if (terrs_[t].devastated) continue;
        if (terrs_[t].owner != p) return false;
        any = true;
    }
    return any;
}

int Game::regionBonus(int p) const {
    int bonus = 0;
    for (size_t r = 0; r < map_.regions().size(); ++r)
        if (controlsRegion(p, static_cast<int>(r))) bonus += map_.region(static_cast<int>(r)).bonus;
    return bonus;
}

int Game::income(int p) const {
    int base = std::max(3, countTerritories(p) / 3);
    return base + regionBonus(p);
}

int Game::score(int p) const { return countTerritories(p) + regionBonus(p); }

std::vector<int> Game::ownedTerritories(int p) const {
    std::vector<int> out;
    for (int t = 0; t < map_.size(); ++t)
        if (terrs_[t].owner == p) out.push_back(t);
    return out;
}

std::vector<int> Game::ownedTerritories(int p, TerrType type) const {
    std::vector<int> out;
    for (int t = 0; t < map_.size(); ++t)
        if (terrs_[t].owner == p && map_.territory(t).type == type) out.push_back(t);
    return out;
}

bool Game::canInvade(int p, int from, int to, std::string* why) const {
    auto fail = [&](const char* msg) {
        if (why) *why = msg;
        return false;
    };
    if (from < 0 || from >= map_.size() || to < 0 || to >= map_.size()) return fail("no such territory");
    if (from == to) return fail("cannot invade yourself");
    const auto& f = terrs_[from];
    const auto& d = terrs_[to];
    if (f.owner != p) return fail("you do not control the attacking territory");
    if (d.owner == p) return fail("you already control the target");
    if (f.devastated || d.devastated) return fail("devastated territories are impassable");
    if (f.units() < 2) return fail("need at least 2 units to invade");

    const TerrType ft = map_.territory(from).type;
    const TerrType tt = map_.territory(to).type;
    if ((ft == TerrType::Water || tt == TerrType::Water) && !commanderInPlay(p, Commander::Naval))
        return fail("a Naval Commander must be in play to invade into or out of water");
    if ((ft == TerrType::Moon || tt == TerrType::Moon) && !commanderInPlay(p, Commander::Space))
        return fail("a Space Commander must be in play to invade into or out of the Moon");

    if (ft == TerrType::Moon && tt != TerrType::Moon) {
        if (players_[p].invadeEarthTarget != to)
            return fail("Earth can only be invaded from the Moon with the Invade Earth card");
        return true;
    }
    if (ft != TerrType::Moon && tt == TerrType::Moon) {
        if (!f.spaceStation) return fail("lunar invasions must launch from a Space Station");
        if (!map_.territory(to).lunarLandingSite)
            return fail("from Earth you may only invade a lunar landing site (Sea of Crisis, Bay of Dew, Tycho)");
        return true;
    }
    if (!map_.adjacent(from, to)) return fail("territories are not adjacent");
    return true;
}

int Game::attackD8Count(int from, int to, int nDice, std::array<bool, kNumCommanders>* which) const {
    const auto& f = terrs_[from];
    const TerrType ft = map_.territory(from).type;
    const TerrType tt = map_.territory(to).type;
    auto qualifies = [&](Commander c) {
        switch (c) {
            case Commander::Nuclear: return true;
            case Commander::Land: return ft == TerrType::Land || tt == TerrType::Land;
            case Commander::Naval: return ft == TerrType::Water || tt == TerrType::Water;
            case Commander::Space: return ft == TerrType::Moon || tt == TerrType::Moon;
            case Commander::Diplomat: return false;
        }
        return false;
    };
    int n = 0;
    if (which) which->fill(false);
    for (int c = 0; c < kNumCommanders && n < nDice; ++c) {
        if (f.commanders[c] && qualifies(static_cast<Commander>(c))) {
            ++n;
            if (which) (*which)[c] = true;
        }
    }
    return n;
}

int Game::defendD8Count(int to, int nDice) const {
    const auto& d = terrs_[to];
    if (d.spaceStation) return nDice;
    return std::min(nDice, d.commanderCount());
}

bool Game::fortifyPathExists(int p, int from, int to) const {
    if (terrs_[from].owner != p || terrs_[to].owner != p) return false;
    std::vector<bool> seen(map_.size(), false);
    std::queue<int> q;
    q.push(from);
    seen[from] = true;
    // Earth<->Moon links exist between any friendly Space Station and any friendly landing site.
    std::vector<int> stations, sites;
    for (int t = 0; t < map_.size(); ++t) {
        if (terrs_[t].owner != p) continue;
        if (terrs_[t].spaceStation) stations.push_back(t);
        if (map_.territory(t).lunarLandingSite) sites.push_back(t);
    }
    auto visit = [&](int n) {
        if (terrs_[n].owner == p && !terrs_[n].devastated && !seen[n]) {
            seen[n] = true;
            q.push(n);
        }
    };
    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        if (cur == to) return true;
        for (int n : map_.territory(cur).adjacent) visit(n);
        if (terrs_[cur].spaceStation)
            for (int s : sites) visit(s);
        if (map_.territory(cur).lunarLandingSite)
            for (int s : stations) visit(s);
    }
    return false;
}

// ============================================================================
// Setup
// ============================================================================

void Game::setup() {
    for (int p = 0; p < static_cast<int>(players_.size()); ++p)
        if (!players_[p].neutral && !agents_[p]) throw std::runtime_error("player " + players_[p].name + " has no agent");

    phase_ = Phase::Setup;
    logf("=== Setup ===");

    // Devastated lands: four random land territories become impassable.
    for (int i = 0; i < kDevastationMarkers; ++i) {
        int t = drawTerritoryCard(TerrType::Land);
        while (terrs_[t].devastated) t = drawTerritoryCard(TerrType::Land);
        devastate(t);
        logf("Devastation marker placed on " + map_.territory(t).name);
    }

    int startMods = 0;
    switch (numPlayers_) {
        case 2: startMods = 30; break;
        case 3: startMods = 35; break;
        case 4: startMods = 30; break;
        default: startMods = 25; break;
    }
    for (int p = 0; p < numPlayers_; ++p) {
        players_[p].pool = startMods;
        players_[p].energy = kStartingEnergy;
    }

    // Two-player game: neutral army holds 16 land, 6 water and 6 lunar territories with 3 MODs each.
    if (hasNeutral()) {
        const int n = neutralPlayer();
        auto placeNeutral = [&](TerrType type, int count) {
            int placed = 0;
            while (placed < count) {
                int t = drawTerritoryCard(type);
                if (terrs_[t].devastated || terrs_[t].owner != -1) continue;
                terrs_[t].owner = n;
                terrs_[t].mods = 3;
                ++placed;
            }
        };
        placeNeutral(TerrType::Land, 16);
        placeNeutral(TerrType::Water, 6);
        placeNeutral(TerrType::Moon, 6);
        logf("Neutral armies placed on 16 land, 6 water and 6 lunar territories");
    }

    // Claim land territories one MOD at a time.
    std::vector<int> free;
    for (int t : map_.territoriesOfType(TerrType::Land))
        if (!terrs_[t].devastated && terrs_[t].owner == -1) free.push_back(t);
    int p = 0;
    while (!free.empty()) {
        int choice = agent(p).chooseClaim(*this, p, free);
        auto it = std::find(free.begin(), free.end(), choice);
        if (it == free.end()) it = free.begin();
        terrs_[*it].owner = p;
        terrs_[*it].mods = 1;
        players_[p].pool -= 1;
        free.erase(it);
        p = (p + 1) % numPlayers_;
    }

    // Place remaining MODs.
    bool any = true;
    while (any) {
        any = false;
        for (int q = 0; q < numPlayers_; ++q) {
            if (players_[q].pool <= 0) continue;
            any = true;
            int t = agent(q).chooseInitialPlacement(*this, q);
            if (t < 0 || t >= map_.size() || terrs_[t].owner != q) t = ownedTerritories(q).front();
            terrs_[t].mods += 1;
            players_[q].pool -= 1;
        }
    }

    // Starting Space Station, Land Commander and Diplomat.
    for (int q = 0; q < numPlayers_; ++q) {
        InitialSetup s = agent(q).chooseInitialSetup(*this, q);
        auto valid = [&](int t) { return t >= 0 && t < map_.size() && terrs_[t].owner == q; };
        int fallback = ownedTerritories(q).front();
        if (!valid(s.spaceStation)) s.spaceStation = fallback;
        if (!valid(s.landCommander)) s.landCommander = fallback;
        if (!valid(s.diplomat)) s.diplomat = fallback;
        terrs_[s.spaceStation].spaceStation = true;
        terrs_[s.landCommander].commanders[ci(Commander::Land)] = true;
        terrs_[s.diplomat].commanders[ci(Commander::Diplomat)] = true;
        logf(players_[q].name + " starts with a Space Station in " + map_.territory(s.spaceStation).name +
             ", Land Commander in " + map_.territory(s.landCommander).name + ", Diplomat in " +
             map_.territory(s.diplomat).name);
    }
}

// ============================================================================
// Year / turn structure
// ============================================================================

void Game::giveTurnOrderBidding() {
    phase_ = Phase::Bidding;
    struct Bid {
        int player;
        int amount;
        int tiebreak;
    };
    std::vector<Bid> bids;
    for (int p = 0; p < numPlayers_; ++p) {
        if (!isActive(p)) continue;
        int b = std::clamp(agent(p).bid(*this, p), 0, players_[p].energy);
        players_[p].energy -= b;
        bids.push_back({p, b, rollDie(6)});
    }
    std::stable_sort(bids.begin(), bids.end(), [](const Bid& a, const Bid& b) {
        if (a.amount != b.amount) return a.amount > b.amount;
        return a.tiebreak > b.tiebreak;
    });
    std::vector<int> markers;
    for (size_t i = 0; i < bids.size(); ++i) markers.push_back(static_cast<int>(i));
    turnOrder_.assign(bids.size(), -1);
    for (const auto& b : bids) {
        int m = agent(b.player).chooseTurnOrder(*this, b.player, markers);
        auto it = std::find(markers.begin(), markers.end(), m);
        if (it == markers.end()) it = markers.begin();
        turnOrder_[*it] = b.player;
        markers.erase(it);
        std::ostringstream os;
        os << players_[b.player].name << " bid " << b.amount << " energy and takes turn marker #" << (*it + 1);
        logf(os.str());
    }
}

void Game::startYear() {
    ++year_;
    logf("=== Year " + std::to_string(year_) + " (" + std::to_string(2205 + year_) + " A.D.) ===");
    giveTurnOrderBidding();
}

void Game::run() {
    setup();
    for (int y = 0; y < kNumYears; ++y) {
        startYear();
        for (int p : turnOrder_)
            if (isActive(p)) takeTurn(p);
    }
    finalScoring();
}

void Game::startTurn(int p) {
    auto& ps = players_[p];
    ps.cardsBoughtThisTurn = 0;
    ps.contestedCaptures = 0;
    ps.bonusClaimed = false;
    ps.invasionDeclaredThisTurn = false;
    ps.fortifiedThisTurn = false;
    ps.invadeEarthTarget = -1;
    resetInvasion();

    int inc = income(p);
    ps.pool += inc;
    ps.energy += inc;
    int stationMods = 0;
    for (auto& t : terrs_)
        if (t.owner == p && t.spaceStation) {
            t.mods += 1;
            ++stationMods;
        }
    std::ostringstream os;
    os << "--- " << ps.name << "'s turn: " << countTerritories(p) << " territories, +" << inc << " MODs, +" << inc
       << " energy";
    if (stationMods) os << ", +" << stationMods << " MOD(s) at Space Stations";
    os << " (energy now " << ps.energy << ")";
    logf(os.str());
}

void Game::takeTurn(int p) {
    current_ = p;
    startTurn(p);
    Agent& a = agent(p);

    phase_ = Phase::Deploy;
    a.deployPhase(*this, p);
    if (players_[p].pool > 0) {  // engine safety: all MODs must be placed
        auto owned = ownedTerritories(p);
        if (!owned.empty()) deployMods(p, owned.front(), players_[p].pool);
    }

    phase_ = Phase::Purchase;
    a.purchasePhase(*this, p);

    phase_ = Phase::PlayCards;
    a.cardPhase(*this, p);

    phase_ = Phase::Invade;
    a.invadePhase(*this, p);
    if (invasion_.active) {
        if (invasion_.captured) {
            int maxMove = terrs_[invasion_.from].units() - 1;
            moveIn(p, std::max(1, std::min(invasion_.lastDice, maxMove)));
        } else {
            invasion_.attackedOnce = true;  // forced end of turn overrides the "attack once" rule
            endInvasion(p);
        }
    }

    phase_ = Phase::Fortify;
    if (isActive(p)) a.fortifyPhase(*this, p);
    current_ = -1;
}

void Game::finalScoring() {
    phase_ = Phase::GameOver;
    logf("=== Final scoring ===");
    for (int p = 0; p < numPlayers_; ++p) {
        auto& ps = players_[p];
        int s = score(p);
        int influence = 0;
        if (!ps.eliminated) {
            std::vector<int> keep;
            for (int id : ps.hand) {
                const CardDef& c = Cards::def(id);
                if (c.timing == CardTiming::Scoring && c.kind == CardKind::ColonyInfluence && commanderInPlay(p, c.deck))
                    ++influence;
                else
                    keep.push_back(id);
            }
            ps.hand = keep;
        }
        ps.finalScore = s + influence;
        std::ostringstream os;
        os << ps.name << ": " << countTerritories(p) << " territories + " << regionBonus(p) << " bonus + " << influence
           << " influence = " << ps.finalScore << " (energy " << ps.energy << ", units " << countUnits(p) << ")";
        logf(os.str());
    }
    std::vector<int> ranking;
    for (int p = 0; p < numPlayers_; ++p) ranking.push_back(p);
    std::sort(ranking.begin(), ranking.end(), [&](int a, int b) {
        if (players_[a].finalScore != players_[b].finalScore) return players_[a].finalScore > players_[b].finalScore;
        if (players_[a].energy != players_[b].energy) return players_[a].energy > players_[b].energy;
        return countUnits(a) > countUnits(b);
    });
    int w = ranking.front();
    logf("Winner: " + players_[w].name + " is elected the new world leader!");
}

// ============================================================================
// Board mutation helpers
// ============================================================================

int Game::destroyUnits(int t, int n) {
    auto& ts = terrs_[t];
    int removed = 0;
    int m = std::min(n, ts.mods);
    ts.mods -= m;
    removed += m;
    for (int c = 0; c < kNumCommanders && removed < n; ++c) {
        if (ts.commanders[c]) {
            ts.commanders[c] = false;
            ++removed;
        }
    }
    if (ts.units() == 0 && !ts.spaceStation) ts.owner = -1;
    return removed;
}

void Game::devastate(int t) {
    auto& ts = terrs_[t];
    ts = TerritoryState{};
    ts.devastated = true;
}

void Game::checkElimination(int p) {
    if (p < 0 || p >= numPlayers_ || players_[p].eliminated) return;
    if (countUnits(p) > 0) return;
    players_[p].eliminated = true;
    players_[p].hand.clear();
    for (auto& t : terrs_)
        if (t.owner == p) {
            t.spaceStation = false;
            t.owner = -1;
        }
    logf(players_[p].name + " has been eliminated!");
}

// ============================================================================
// Actions
// ============================================================================

Result Game::deployMods(int p, int territory, int n) {
    if (phase_ != Phase::Deploy || current_ != p) return Result::fail("not your deploy phase");
    if (territory < 0 || territory >= map_.size()) return Result::fail("no such territory");
    if (terrs_[territory].owner != p) return Result::fail("you do not control that territory");
    if (n <= 0 || n > players_[p].pool) return Result::fail("invalid MOD count");
    terrs_[territory].mods += n;
    players_[p].pool -= n;
    logf(players_[p].name + " deploys " + std::to_string(n) + " MOD(s) to " + map_.territory(territory).name);
    return Result::success();
}

Result Game::hireCommander(int p, Commander c, int territory) {
    if (phase_ != Phase::Purchase || current_ != p) return Result::fail("not your purchase phase");
    if (commanderInPlay(p, c)) return Result::fail("that commander is already in play");
    if (players_[p].energy < kCommanderCost) return Result::fail("not enough energy");
    if (territory < 0 || territory >= map_.size() || terrs_[territory].owner != p)
        return Result::fail("you do not control that territory");
    players_[p].energy -= kCommanderCost;
    terrs_[territory].commanders[ci(c)] = true;
    logf(players_[p].name + " hires a " + std::string(toString(c)) + " Commander in " + map_.territory(territory).name);
    return Result::success();
}

Result Game::buildSpaceStation(int p, int territory) {
    if (phase_ != Phase::Purchase || current_ != p) return Result::fail("not your purchase phase");
    if (countSpaceStations(p) >= kMaxSpaceStations) return Result::fail("you already control 4 Space Stations");
    if (players_[p].energy < kSpaceStationCost) return Result::fail("not enough energy");
    if (territory < 0 || territory >= map_.size() || terrs_[territory].owner != p)
        return Result::fail("you do not control that territory");
    if (map_.territory(territory).type != TerrType::Land) return Result::fail("Space Stations may only be built on land");
    if (terrs_[territory].spaceStation) return Result::fail("that territory already has a Space Station");
    players_[p].energy -= kSpaceStationCost;
    terrs_[territory].spaceStation = true;
    logf(players_[p].name + " builds a Space Station in " + map_.territory(territory).name);
    return Result::success();
}

Result Game::buyCards(int p, const std::vector<Commander>& decks) {
    if (phase_ != Phase::Purchase || current_ != p) return Result::fail("not your purchase phase");
    auto& ps = players_[p];
    if (decks.empty()) return Result::fail("no decks chosen");
    if (ps.cardsBoughtThisTurn + static_cast<int>(decks.size()) > kMaxCardsPerTurn)
        return Result::fail("you may buy at most 4 cards per turn");
    if (ps.energy < kCardCost * static_cast<int>(decks.size())) return Result::fail("not enough energy");
    std::array<int, kNumCommanders> want{};
    for (Commander c : decks) {
        if (!commanderInPlay(p, c)) return Result::fail(std::string(toString(c)) + " Commander is not in play");
        if (++want[ci(c)] > deckSize(c)) return Result::fail(std::string(toString(c)) + " deck is exhausted");
    }
    for (Commander c : decks) {
        int id = drawCommandCard(c);
        ps.hand.push_back(id);
        ps.energy -= kCardCost;
        ++ps.cardsBoughtThisTurn;
    }
    logf(ps.name + " buys " + std::to_string(decks.size()) + " command card(s)");
    return Result::success();
}

Result Game::playCard(int p, int handIndex) {
    if (current_ != p) return Result::fail("not your turn");
    if (phase_ != Phase::PlayCards && phase_ != Phase::Invade) return Result::fail("cards cannot be played now");
    auto& ps = players_[p];
    if (handIndex < 0 || handIndex >= static_cast<int>(ps.hand.size())) return Result::fail("no such card");
    const CardDef& card = Cards::def(ps.hand[handIndex]);
    if (card.timing != CardTiming::BeforeFirstInvasion) return Result::fail("that card cannot be played now");
    if (ps.invasionDeclaredThisTurn) return Result::fail("you have already declared an invasion this turn");
    if (!commanderInPlay(p, card.deck)) return Result::fail(std::string(toString(card.deck)) + " Commander is not in play");
    if (ps.energy < card.cost) return Result::fail("not enough energy");
    ps.energy -= card.cost;
    ps.hand.erase(ps.hand.begin() + handIndex);
    logf(ps.name + " plays " + card.name);
    applyCard(p, card);
    return Result::success();
}

void Game::applyCard(int p, const CardDef& card) {
    auto& ps = players_[p];
    Agent& a = agent(p);
    switch (card.kind) {
        case CardKind::Reinforcements: {
            auto opts = ownedTerritories(p, card.target);
            int placements = std::min<int>(3, static_cast<int>(opts.size()));
            for (int i = 0; i < placements; ++i) {
                int t = a.chooseTerritory(*this, p, opts, "Place 1 reinforcement MOD");
                if (std::find(opts.begin(), opts.end(), t) == opts.end()) t = opts.front();
                terrs_[t].mods += 1;
                opts.erase(std::find(opts.begin(), opts.end(), t));
            }
            break;
        }
        case CardKind::AssembleMods: {
            auto opts = ownedTerritories(p, card.target);
            if (opts.empty()) break;
            int t = a.chooseTerritory(*this, p, opts, "Place 3 MODs");
            if (std::find(opts.begin(), opts.end(), t) == opts.end()) t = opts.front();
            terrs_[t].mods += 3;
            break;
        }
        case CardKind::EnergyCrisis: {
            int got = 0;
            for (int q = 0; q < numPlayers_; ++q) {
                if (q == p || !isActive(q) || players_[q].energy <= 0) continue;
                players_[q].energy -= 1;
                ++got;
            }
            ps.energy += got;
            logf(ps.name + " collects " + std::to_string(got) + " energy");
            break;
        }
        case CardKind::Redeployment: {
            std::vector<int> sources;
            for (int t : ownedTerritories(p))
                if (terrs_[t].mods >= 1 && terrs_[t].units() >= 2) sources.push_back(t);
            if (sources.empty()) break;
            int from = a.chooseTerritory(*this, p, sources, "Redeploy MODs from");
            if (std::find(sources.begin(), sources.end(), from) == sources.end()) from = sources.front();
            auto dests = ownedTerritories(p);
            dests.erase(std::find(dests.begin(), dests.end(), from));
            if (dests.empty()) break;
            int to = a.chooseTerritory(*this, p, dests, "Redeploy MODs to");
            if (std::find(dests.begin(), dests.end(), to) == dests.end()) to = dests.front();
            int n = std::min({3, terrs_[from].mods, terrs_[from].units() - 1});
            terrs_[from].mods -= n;
            terrs_[to].mods += n;
            logf(ps.name + " redeploys " + std::to_string(n) + " MOD(s) from " + map_.territory(from).name + " to " +
                 map_.territory(to).name);
            break;
        }
        case CardKind::EnergyExtraction: {
            int n = std::min<int>(4, static_cast<int>(ownedTerritories(p, TerrType::Water).size()));
            ps.energy += n;
            logf(ps.name + " extracts " + std::to_string(n) + " energy");
            break;
        }
        case CardKind::ScatterBomb: {
            for (int i = 0; i < 3; ++i) {
                int t = drawTerritoryCard(card.target);
                auto& ts = terrs_[t];
                if (ts.owner == -1 || ts.owner == p || ts.devastated) continue;
                int owner = ts.owner;
                int n = (ts.units() + 1) / 2;
                destroyUnits(t, n);
                logf("Scatter bomb hits " + map_.territory(t).name + ": " + std::to_string(n) + " unit(s) destroyed");
                checkElimination(owner);
            }
            break;
        }
        case CardKind::TheMother: {
            int t = drawTerritoryCard(TerrType::Land);
            while (terrs_[t].devastated) t = drawTerritoryCard(TerrType::Land);
            std::vector<int> affected;
            for (int n : map_.territory(t).adjacent)
                if (map_.territory(n).type == TerrType::Land && !terrs_[n].devastated) {
                    int owner = terrs_[n].owner;
                    destroyUnits(n, terrs_[n].units());
                    if (owner >= 0) affected.push_back(owner);
                }
            int owner = terrs_[t].owner;
            if (owner >= 0) affected.push_back(owner);
            devastate(t);
            logf("The Mother detonates on " + map_.territory(t).name + " — it is now devastated");
            for (int q : affected) checkElimination(q);
            break;
        }
        case CardKind::Armageddon: {
            for (int t = 0; t < map_.size(); ++t) destroyUnits(t, terrs_[t].units() / 2);
            logf("Armageddon: every territory loses half its units");
            for (int q = 0; q < numPlayers_; ++q) checkElimination(q);
            break;
        }
        case CardKind::InvadeEarth: {
            int t = drawTerritoryCard(TerrType::Land);
            while (terrs_[t].devastated) t = drawTerritoryCard(TerrType::Land);
            ps.invadeEarthTarget = t;
            logf(ps.name + " may invade " + map_.territory(t).name + " from the Moon this turn");
            break;
        }
        case CardKind::ScoutForces: {
            int t = drawTerritoryCard(TerrType::Land);
            while (terrs_[t].devastated) t = drawTerritoryCard(TerrType::Land);
            if (terrs_[t].owner == p) {
                terrs_[t].mods += 5;
                logf(ps.name + " places 5 scout MODs on " + map_.territory(t).name);
            } else {
                ps.scoutTerritory = t;
                logf(ps.name + " has scouts waiting in " + map_.territory(t).name);
            }
            break;
        }
        case CardKind::StealthMods:
        case CardKind::CeaseFire:
        case CardKind::ColonyInfluence:
            break;  // handled elsewhere (reactive / scoring)
    }
}

Result Game::applyReactiveCard(int p, int handIndex) {
    auto& ps = players_[p];
    if (handIndex < 0 || handIndex >= static_cast<int>(ps.hand.size())) return Result::fail("no such card");
    const CardDef& card = Cards::def(ps.hand[handIndex]);
    if (card.timing != CardTiming::OnInvasionDeclared) return Result::fail("not a reactive card");
    if (!commanderInPlay(p, card.deck)) return Result::fail("commander not in play");
    if (ps.energy < card.cost) return Result::fail("not enough energy");
    const int to = invasion_.to;
    switch (card.kind) {
        case CardKind::StealthMods:
            if (map_.territory(to).type != card.target) return Result::fail("wrong territory type");
            if (invasion_.defender < 0) return Result::fail("target is empty");
            break;
        case CardKind::CeaseFire:
            if (invasion_.defender != p) return Result::fail("only the defender may play Cease Fire");
            break;
        default:
            return Result::fail("unsupported reactive card");
    }
    ps.energy -= card.cost;
    ps.hand.erase(ps.hand.begin() + handIndex);
    logf(ps.name + " plays " + card.name);
    if (card.kind == CardKind::StealthMods) {
        terrs_[to].mods += 3;
    } else {
        invasion_.active = false;
    }
    return Result::success();
}

Result Game::declareInvasion(int p, int from, int to) {
    if (phase_ != Phase::Invade || current_ != p) return Result::fail("not your invade phase");
    if (invasion_.active) return Result::fail("an invasion is already in progress");
    std::string why;
    if (!canInvade(p, from, to, &why)) return Result::fail(why);

    invasion_ = Invasion{};
    invasion_.active = true;
    invasion_.attacker = p;
    invasion_.from = from;
    invasion_.to = to;
    invasion_.defender = terrs_[to].owner;
    invasion_.contested = terrs_[to].units() > 0;
    players_[p].invasionDeclaredThisTurn = true;
    logf(players_[p].name + " invades " + map_.territory(to).name + " from " + map_.territory(from).name);

    // Reactive cards: defender first, then everyone else.
    if (invasion_.contested) {
        std::vector<int> order;
        if (isActive(invasion_.defender)) order.push_back(invasion_.defender);
        for (int q = 0; q < numPlayers_; ++q)
            if (q != invasion_.defender && q != p && isActive(q)) order.push_back(q);
        for (int q : order) {
            for (int guard = 0; guard < 8 && invasion_.active; ++guard) {
                int idx = agent(q).reactiveCard(*this, q, invasion_);
                if (idx < 0) break;
                if (!applyReactiveCard(q, idx)) break;
            }
            if (!invasion_.active) break;
        }
        if (!invasion_.active) {
            resetInvasion();
            return Result::fail("invasion cancelled by Cease Fire");
        }
    }

    // Empty territory: no dice, but at least one unit must move in.
    if (!invasion_.contested) {
        int old = terrs_[to].owner;
        terrs_[to].owner = p;
        if (old >= 0 && old != p) {
            // station-only territory captured without a fight
            if (terrs_[to].spaceStation && countSpaceStations(p) > kMaxSpaceStations) terrs_[to].spaceStation = false;
        }
        invasion_.captured = true;
        invasion_.lastDice = 1;
        invasion_.attackedOnce = true;
    }
    return Result::success();
}

Result Game::attack(int p, int nDice, BattleResult* out) {
    if (!invasion_.active || invasion_.attacker != p) return Result::fail("no invasion in progress");
    if (invasion_.captured) return Result::fail("territory already captured; move units in");
    auto& f = terrs_[invasion_.from];
    auto& d = terrs_[invasion_.to];
    int maxA = std::min(3, f.units() - 1);
    if (maxA < 1) return Result::fail("not enough units to attack");
    if (nDice < 1 || nDice > maxA) return Result::fail("you may roll between 1 and " + std::to_string(maxA) + " dice");

    int maxD = std::min(2, d.units());
    int dDice = maxD;
    if (isActive(invasion_.defender)) dDice = std::clamp(agent(invasion_.defender).chooseDefenseDice(*this, invasion_.defender, invasion_, maxD), 1, maxD);

    std::array<bool, kNumCommanders> which{};
    int a8 = attackD8Count(invasion_.from, invasion_.to, nDice, &which);
    int d8 = defendD8Count(invasion_.to, dDice);

    BattleResult br;
    for (int i = 0; i < nDice; ++i) br.attackRolls.push_back(rollDie(i < a8 ? 8 : 6));
    for (int i = 0; i < dDice; ++i) br.defendRolls.push_back(rollDie(i < d8 ? 8 : 6));
    std::sort(br.attackRolls.rbegin(), br.attackRolls.rend());
    std::sort(br.defendRolls.rbegin(), br.defendRolls.rend());
    br.attackD8 = a8;
    br.defendD8 = d8;
    int pairs = std::min(nDice, dDice);
    for (int i = 0; i < pairs; ++i) {
        if (br.attackRolls[i] > br.defendRolls[i]) ++br.defenderLost;
        else ++br.attackerLost;
    }
    destroyUnits(invasion_.from, br.attackerLost);
    if (f.units() == 0) f.owner = f.spaceStation ? p : -1;  // cannot happen: attacker always keeps 1+ units
    destroyUnits(invasion_.to, br.defenderLost);
    invasion_.attackedOnce = true;
    invasion_.lastDice = nDice;

    {
        std::ostringstream os;
        os << "  " << players_[p].name << " rolls";
        for (int r : br.attackRolls) os << ' ' << r;
        if (a8) os << " (" << a8 << "x d8)";
        os << " vs";
        for (int r : br.defendRolls) os << ' ' << r;
        if (d8) os << " (" << d8 << "x d8)";
        os << " -> attacker -" << br.attackerLost << ", defender -" << br.defenderLost;
        logf(os.str());
    }

    if (d.units() == 0) {
        br.captured = true;
        invasion_.captured = true;
        invasion_.mustMoveIn = which;
        for (int c = 0; c < kNumCommanders; ++c)
            if (!f.commanders[c]) invasion_.mustMoveIn[c] = false;  // died in this very battle
        int defender = invasion_.defender;
        // Captured Space Station changes hands (or is destroyed if the attacker already has 4).
        if (d.spaceStation && countSpaceStations(p) >= kMaxSpaceStations) {
            d.spaceStation = false;
            logf("  the Space Station in " + map_.territory(invasion_.to).name + " is destroyed");
        }
        d.owner = p;
        logf("  " + players_[p].name + " captures " + map_.territory(invasion_.to).name);

        auto& ps = players_[p];
        if (invasion_.contested) ++ps.contestedCaptures;
        if (ps.contestedCaptures >= 3 && !ps.bonusClaimed) {
            ps.bonusClaimed = true;
            ps.energy += 1;
            std::vector<Commander> opts;
            for (int c = 0; c < kNumCommanders; ++c)
                if (commanderInPlay(p, static_cast<Commander>(c)) && deckSize(static_cast<Commander>(c)) > 0)
                    opts.push_back(static_cast<Commander>(c));
            if (!opts.empty()) {
                Commander c = agent(p).chooseBonusDeck(*this, p, opts);
                if (std::find(opts.begin(), opts.end(), c) == opts.end()) c = opts.front();
                ps.hand.push_back(drawCommandCard(c));
            }
            logf("  3-territory bonus: " + ps.name + " gains 1 energy and 1 command card");
        }
        checkElimination(defender);
    }
    if (out) *out = br;
    return Result::success();
}

Result Game::moveIn(int p, int n) {
    if (!invasion_.active || invasion_.attacker != p) return Result::fail("no invasion in progress");
    if (!invasion_.captured) return Result::fail("territory not captured yet");
    auto& f = terrs_[invasion_.from];
    auto& d = terrs_[invasion_.to];
    int maxMove = f.units() - 1;
    int minMove = std::max(1, std::min(invasion_.lastDice, maxMove));
    if (n < minMove) return Result::fail("you must move in at least " + std::to_string(minMove) + " unit(s)");
    if (n > maxMove) return Result::fail("you must leave at least one unit behind");

    int moved = 0;
    for (int c = 0; c < kNumCommanders; ++c)
        if (invasion_.mustMoveIn[c] && f.commanders[c]) {
            f.commanders[c] = false;
            d.commanders[c] = true;
            ++moved;
        }
    n = std::max(n, moved);
    int m = std::min(n - moved, f.mods);
    f.mods -= m;
    d.mods += m;
    moved += m;
    for (int c = 0; c < kNumCommanders && moved < n; ++c)
        if (f.commanders[c]) {
            f.commanders[c] = false;
            d.commanders[c] = true;
            ++moved;
        }
    logf("  " + players_[p].name + " moves " + std::to_string(moved) + " unit(s) into " + map_.territory(invasion_.to).name);

    auto& ps = players_[p];
    if (ps.scoutTerritory == invasion_.to) {
        d.mods += 5;
        ps.scoutTerritory = -1;
        logf("  scout forces arrive: +5 MODs in " + map_.territory(invasion_.to).name);
    }
    resetInvasion();
    return Result::success();
}

Result Game::endInvasion(int p) {
    if (!invasion_.active || invasion_.attacker != p) return Result::fail("no invasion in progress");
    if (invasion_.captured) return Result::fail("you must move units into the captured territory first");
    if (!invasion_.attackedOnce) return Result::fail("you must attack at least once before calling off an invasion");
    resetInvasion();
    return Result::success();
}

Result Game::fortify(int p, int from, int to, int mods, std::array<bool, kNumCommanders> commanders) {
    if (phase_ != Phase::Fortify || current_ != p) return Result::fail("not your fortify phase");
    if (players_[p].fortifiedThisTurn) return Result::fail("you may only fortify once per turn");
    if (from < 0 || from >= map_.size() || to < 0 || to >= map_.size() || from == to) return Result::fail("invalid territories");
    auto& f = terrs_[from];
    auto& d = terrs_[to];
    if (f.owner != p || d.owner != p) return Result::fail("you must control both territories");
    int cmdMoves = 0;
    for (int c = 0; c < kNumCommanders; ++c)
        if (commanders[c]) {
            if (!f.commanders[c]) return Result::fail("that commander is not in the source territory");
            ++cmdMoves;
        }
    if (mods < 0 || mods > f.mods) return Result::fail("invalid MOD count");
    if (mods + cmdMoves <= 0) return Result::fail("nothing to move");
    if (mods + cmdMoves > f.units() - 1) return Result::fail("you must leave at least one unit behind");
    if (!fortifyPathExists(p, from, to)) return Result::fail("no path of friendly territories connects them");
    f.mods -= mods;
    d.mods += mods;
    for (int c = 0; c < kNumCommanders; ++c)
        if (commanders[c]) {
            f.commanders[c] = false;
            d.commanders[c] = true;
        }
    players_[p].fortifiedThisTurn = true;
    logf(players_[p].name + " fortifies " + std::to_string(mods + cmdMoves) + " unit(s) from " + map_.territory(from).name +
         " to " + map_.territory(to).name);
    return Result::success();
}

}  // namespace risk2210
