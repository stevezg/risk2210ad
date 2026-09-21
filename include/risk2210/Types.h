#pragma once
#include <array>
#include <string>

namespace risk2210 {

enum class TerrType { Land, Water, Moon };

enum class Commander { Land = 0, Diplomat, Naval, Nuclear, Space };
constexpr int kNumCommanders = 5;

enum class Phase {
    Setup,
    Bidding,
    Deploy,
    Purchase,
    PlayCards,
    Invade,
    Fortify,
    GameOver
};

constexpr int kCommanderCost = 3;
constexpr int kSpaceStationCost = 5;
constexpr int kMaxSpaceStations = 4;
constexpr int kCardCost = 1;
constexpr int kMaxCardsPerTurn = 4;
constexpr int kNumYears = 5;
constexpr int kStartingEnergy = 3;
constexpr int kDevastationMarkers = 4;

inline const char* toString(TerrType t) {
    switch (t) {
        case TerrType::Land: return "Land";
        case TerrType::Water: return "Water";
        case TerrType::Moon: return "Moon";
    }
    return "?";
}

inline const char* toString(Commander c) {
    switch (c) {
        case Commander::Land: return "Land";
        case Commander::Diplomat: return "Diplomat";
        case Commander::Naval: return "Naval";
        case Commander::Nuclear: return "Nuclear";
        case Commander::Space: return "Space";
    }
    return "?";
}

inline const char* toString(Phase p) {
    switch (p) {
        case Phase::Setup: return "Setup";
        case Phase::Bidding: return "Bidding";
        case Phase::Deploy: return "Deploy";
        case Phase::Purchase: return "Purchase";
        case Phase::PlayCards: return "PlayCards";
        case Phase::Invade: return "Invade";
        case Phase::Fortify: return "Fortify";
        case Phase::GameOver: return "GameOver";
    }
    return "?";
}

/// Outcome of an engine action. `ok == false` means the action was rejected
/// and the game state is unchanged; `error` explains why.
struct Result {
    bool ok = true;
    std::string error;
    static Result success() { return {}; }
    static Result fail(std::string msg) { return {false, std::move(msg)}; }
    explicit operator bool() const { return ok; }
};

}  // namespace risk2210
