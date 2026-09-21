#pragma once
#include <string>
#include <vector>

#include "risk2210/Types.h"

namespace risk2210 {

/// When a command card may be played.
enum class CardTiming {
    BeforeFirstInvasion,  // on your turn, after purchases, before your first invasion is declared
    OnInvasionDeclared,   // reactive: after any player declares an invasion
    Scoring               // at the end of year 5, during final scoring
};

/// The engine-implemented effect a card carries. Several cards share a kind
/// but differ in the territory type they target (e.g. the three Reinforcements
/// cards, or the three Scatter Bombs).
enum class CardKind {
    Reinforcements,   // place 3 MODs, one each on 3 different territories of `target` type you occupy
    AssembleMods,     // place 3 MODs on one territory of `target` type you occupy
    StealthMods,      // reactive: +3 defending MODs in the defending territory of `target` type
    ColonyInfluence,  // scoring: +1 score (requires the deck's commander in play)
    EnergyCrisis,     // collect 1 energy from each opponent
    CeaseFire,        // reactive: cancel the declared invasion
    Redeployment,     // move up to 3 MODs from one territory to any other territory you occupy
    EnergyExtraction, // gain 1 energy per water territory you occupy (max 4)
    ScatterBomb,      // flip 3 territory cards of `target` type; destroy half of opponents' units there (round up)
    TheMother,        // draw a land card; destroy every unit there and in adjacent land territories, then devastate it
    Armageddon,       // every territory on the board loses half its units (round down)
    InvadeEarth,      // this turn you may invade one randomly drawn land territory from any lunar territory
    ScoutForces       // draw a land card; when you next occupy it, place 5 MODs there
};

struct CardDef {
    int id = -1;
    std::string name;
    Commander deck = Commander::Land;
    int cost = 0;
    CardTiming timing = CardTiming::BeforeFirstInvasion;
    CardKind kind = CardKind::Reinforcements;
    TerrType target = TerrType::Land;
    std::string text;
};

/// Static catalogue of every card definition plus per-deck compositions.
class Cards {
public:
    static const std::vector<CardDef>& all();
    static const CardDef& def(int id) { return all().at(id); }
    /// Card definition ids making up a fresh (unshuffled) deck for a commander.
    static std::vector<int> deckFor(Commander c);
};

}  // namespace risk2210
