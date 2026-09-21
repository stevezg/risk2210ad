#pragma once
#include <array>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "risk2210/Agent.h"
#include "risk2210/Cards.h"
#include "risk2210/Map.h"
#include "risk2210/Types.h"

namespace risk2210 {

struct TerritoryState {
    int owner = -1;
    int mods = 0;
    std::array<bool, kNumCommanders> commanders{};
    bool spaceStation = false;
    bool devastated = false;

    int commanderCount() const {
        int n = 0;
        for (bool b : commanders) n += b ? 1 : 0;
        return n;
    }
    int units() const { return mods + commanderCount(); }
    bool hasCommander(Commander c) const { return commanders[static_cast<int>(c)]; }
};

struct PlayerState {
    std::string name;
    int energy = kStartingEnergy;
    int pool = 0;           // MODs waiting to be deployed
    std::vector<int> hand;  // card definition ids
    bool eliminated = false;
    bool neutral = false;   // 2-player game "neutral" army: defends but never acts
    int scoutTerritory = -1;
    int finalScore = 0;

    // per-turn bookkeeping (reset at the start of each turn)
    int cardsBoughtThisTurn = 0;
    int contestedCaptures = 0;
    bool bonusClaimed = false;
    bool invasionDeclaredThisTurn = false;
    bool fortifiedThisTurn = false;
    int invadeEarthTarget = -1;
};

struct Invasion {
    bool active = false;
    int attacker = -1;
    int defender = -1;  // -1 when the target territory was empty
    int from = -1;
    int to = -1;
    bool contested = false;
    bool attackedOnce = false;
    bool captured = false;
    int lastDice = 0;
    std::array<bool, kNumCommanders> mustMoveIn{};  // commanders that rolled d8 in the capturing battle
};

struct BattleResult {
    std::vector<int> attackRolls;   // sorted descending
    std::vector<int> defendRolls;   // sorted descending
    int attackD8 = 0;               // how many of the attacker's dice were 8-sided
    int defendD8 = 0;
    int attackerLost = 0;
    int defenderLost = 0;
    bool captured = false;
};

/// Complete Risk 2210 A.D. rules engine. Construct it, attach one Agent per
/// player, and call run() — or drive the action API directly for a custom loop.
class Game {
public:
    Game(std::vector<std::string> playerNames, unsigned seed = std::random_device{}());

    void setAgent(int player, Agent* agent) { agents_.at(player) = agent; }
    void setLogger(std::function<void(const std::string&)> fn) { logger_ = std::move(fn); }

    /// Plays a complete game (setup, 5 years, final scoring).
    void run();
    // Building blocks of run(), exposed for custom drivers.
    void setup();
    void startYear();
    void takeTurn(int player);
    void finalScoring();

    // ---- queries ---------------------------------------------------------
    const Map& map() const { return map_; }
    int numPlayers() const { return numPlayers_; }  // real players (excludes neutral)
    bool hasNeutral() const { return static_cast<int>(players_.size()) > numPlayers_; }
    int neutralPlayer() const { return hasNeutral() ? numPlayers_ : -1; }
    const PlayerState& player(int p) const { return players_.at(p); }
    const TerritoryState& territory(int t) const { return terrs_.at(t); }
    const std::vector<TerritoryState>& territories() const { return terrs_; }
    int year() const { return year_; }
    Phase phase() const { return phase_; }
    int currentPlayer() const { return current_; }
    const Invasion& invasion() const { return invasion_; }
    const std::vector<int>& turnOrder() const { return turnOrder_; }
    const std::vector<std::string>& log() const { return log_; }
    int deckSize(Commander c) const { return static_cast<int>(decks_[static_cast<int>(c)].size()); }

    bool isActive(int p) const { return p >= 0 && p < numPlayers_ && !players_[p].eliminated; }
    bool commanderInPlay(int p, Commander c) const;
    int countTerritories(int p) const;
    int countUnits(int p) const;
    int countSpaceStations(int p) const;
    bool controlsRegion(int p, int region) const;
    int regionBonus(int p) const;
    /// MODs (and energy) a player receives at the start of a turn, before space-station MODs.
    int income(int p) const;
    int score(int p) const;
    std::vector<int> ownedTerritories(int p) const;
    std::vector<int> ownedTerritories(int p, TerrType type) const;

    /// Whether `from` may currently invade `to`. `why` receives the reason on failure.
    bool canInvade(int p, int from, int to, std::string* why = nullptr) const;
    /// Attacker's 8-sided dice count for the given number of dice.
    int attackD8Count(int from, int to, int nDice, std::array<bool, kNumCommanders>* which = nullptr) const;
    int defendD8Count(int to, int nDice) const;
    /// Whether a fortify path of friendly territories connects `from` and `to`.
    bool fortifyPathExists(int p, int from, int to) const;

    // ---- actions (validated; return Result::fail without mutating on error) --
    Result deployMods(int p, int territory, int n);
    Result hireCommander(int p, Commander c, int territory);
    Result buildSpaceStation(int p, int territory);
    Result buyCards(int p, const std::vector<Commander>& decks);
    Result playCard(int p, int handIndex);
    Result declareInvasion(int p, int from, int to);
    Result attack(int p, int nDice, BattleResult* out = nullptr);
    Result moveIn(int p, int n);
    Result endInvasion(int p);
    Result fortify(int p, int from, int to, int mods, std::array<bool, kNumCommanders> commanders = {});

private:
    void logf(const std::string& s);
    int rollDie(int sides) { return std::uniform_int_distribution<int>(1, sides)(rng_); }
    int drawTerritoryCard(TerrType type);
    int drawCommandCard(Commander c);  // -1 if the deck is empty
    void giveTurnOrderBidding();
    void startTurn(int p);
    void resetInvasion() { invasion_ = Invasion{}; }
    /// Removes n units from a territory (MODs first, then commanders). Returns units actually removed.
    int destroyUnits(int t, int n);
    void devastate(int t);
    void checkElimination(int p);
    void applyCard(int p, const CardDef& card);
    Result applyReactiveCard(int p, int handIndex);
    Agent& agent(int p) { return *agents_.at(p); }

    Map map_;
    std::vector<TerritoryState> terrs_;
    std::vector<PlayerState> players_;
    std::vector<Agent*> agents_;
    int numPlayers_ = 0;
    std::mt19937 rng_;

    std::array<std::vector<int>, kNumCommanders> decks_;
    std::array<std::vector<int>, 3> terrDecks_;  // land, water, moon
    std::array<size_t, 3> terrDeckPos_{};

    int year_ = 0;
    Phase phase_ = Phase::Setup;
    int current_ = -1;
    std::vector<int> turnOrder_;
    Invasion invasion_;
    std::vector<std::string> log_;
    std::function<void(const std::string&)> logger_;
};

}  // namespace risk2210
