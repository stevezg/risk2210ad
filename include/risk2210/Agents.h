#pragma once
#include <iosfwd>
#include <random>

#include "risk2210/Agent.h"
#include "risk2210/Game.h"

namespace risk2210 {

/// Simple heuristic AI: attacks when it has local superiority, hires
/// commanders, buys and plays cards, fortifies toward the front.
class RandomAgent : public Agent {
public:
    explicit RandomAgent(unsigned seed = std::random_device{}()) : rng_(seed) {}

    int chooseClaim(const Game& g, int player, const std::vector<int>& freeTerritories) override;
    int chooseInitialPlacement(const Game& g, int player) override;
    InitialSetup chooseInitialSetup(const Game& g, int player) override;
    int bid(const Game& g, int player) override;
    int chooseTurnOrder(const Game& g, int player, const std::vector<int>& availableMarkers) override;
    void deployPhase(Game& g, int player) override;
    void purchasePhase(Game& g, int player) override;
    void cardPhase(Game& g, int player) override;
    void invadePhase(Game& g, int player) override;
    void fortifyPhase(Game& g, int player) override;
    int reactiveCard(const Game& g, int player, const Invasion& inv) override;
    int chooseTerritory(const Game& g, int player, const std::vector<int>& options, const std::string& prompt) override;

private:
    bool isBorder(const Game& g, int player, int t) const;
    std::vector<int> borderTerritories(const Game& g, int player) const;
    template <class T>
    const T& pick(const std::vector<T>& v) {
        return v[std::uniform_int_distribution<size_t>(0, v.size() - 1)(rng_)];
    }
    std::mt19937 rng_;
};

/// Interactive text-mode player reading commands from an input stream.
class HumanCliAgent : public Agent {
public:
    HumanCliAgent(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

    int chooseClaim(const Game& g, int player, const std::vector<int>& freeTerritories) override;
    int chooseInitialPlacement(const Game& g, int player) override;
    InitialSetup chooseInitialSetup(const Game& g, int player) override;
    int bid(const Game& g, int player) override;
    int chooseTurnOrder(const Game& g, int player, const std::vector<int>& availableMarkers) override;
    void deployPhase(Game& g, int player) override;
    void purchasePhase(Game& g, int player) override;
    void cardPhase(Game& g, int player) override;
    void invadePhase(Game& g, int player) override;
    void fortifyPhase(Game& g, int player) override;
    int chooseDefenseDice(const Game& g, int player, const Invasion& inv, int maxDice) override;
    int reactiveCard(const Game& g, int player, const Invasion& inv) override;
    Commander chooseBonusDeck(const Game& g, int player, const std::vector<Commander>& options) override;
    int chooseTerritory(const Game& g, int player, const std::vector<int>& options, const std::string& prompt) override;

private:
    void printTerritory(const Game& g, int t) const;
    void printOwned(const Game& g, int player) const;
    void printHand(const Game& g, int player) const;
    void printStatus(const Game& g, int player) const;
    /// Reads a territory by id or (case-insensitive) name; -1 on EOF / "done".
    int readTerritory(const Game& g, const std::string& prompt);
    int readInt(const std::string& prompt, int lo, int hi);
    bool readLine(std::string& line);

    std::istream& in_;
    std::ostream& out_;
};

}  // namespace risk2210
