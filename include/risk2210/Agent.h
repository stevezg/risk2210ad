#pragma once
#include <string>
#include <vector>

#include "risk2210/Types.h"

namespace risk2210 {

class Game;
struct Invasion;

struct InitialSetup {
    int spaceStation = -1;   // land territory for the starting Space Station
    int landCommander = -1;  // territory for the starting Land Commander
    int diplomat = -1;       // territory for the starting Diplomat
};

/// Decision-maker for one player. The engine calls these hooks at the right
/// moments; phase hooks receive a mutable Game and drive it through its action
/// API (deployMods, declareInvasion, attack, ...). Every action is validated by
/// the engine, so an agent can never put the game into an illegal state.
class Agent {
public:
    virtual ~Agent() = default;

    // ---- setup -----------------------------------------------------------
    virtual int chooseClaim(const Game& g, int player, const std::vector<int>& freeTerritories) = 0;
    virtual int chooseInitialPlacement(const Game& g, int player) = 0;
    virtual InitialSetup chooseInitialSetup(const Game& g, int player) = 0;

    // ---- start of year ---------------------------------------------------
    virtual int bid(const Game& g, int player) = 0;
    virtual int chooseTurnOrder(const Game& g, int player, const std::vector<int>& availableMarkers) = 0;

    // ---- turn phases -----------------------------------------------------
    virtual void deployPhase(Game& g, int player) = 0;
    virtual void purchasePhase(Game& g, int player) = 0;
    virtual void cardPhase(Game& g, int player) = 0;
    virtual void invadePhase(Game& g, int player) = 0;
    virtual void fortifyPhase(Game& g, int player) = 0;

    // ---- reactive / mid-action choices ----------------------------------
    /// Number of dice to defend with (engine clamps to what is legal).
    virtual int chooseDefenseDice(const Game& g, int player, const Invasion& inv, int maxDice) {
        (void)g; (void)player; (void)inv;
        return maxDice;
    }
    /// Return an index into your hand to play a reactive card, or -1 to pass.
    /// Called repeatedly until -1 is returned.
    virtual int reactiveCard(const Game& g, int player, const Invasion& inv) {
        (void)g; (void)player; (void)inv;
        return -1;
    }
    /// Which deck to draw the 3-territory bonus card from.
    virtual Commander chooseBonusDeck(const Game& g, int player, const std::vector<Commander>& options) {
        (void)g; (void)player;
        return options.front();
    }
    /// Generic territory choice used by card effects. Must return an element of `options`.
    virtual int chooseTerritory(const Game& g, int player, const std::vector<int>& options, const std::string& prompt) {
        (void)g; (void)player; (void)prompt;
        return options.front();
    }
};

}  // namespace risk2210
